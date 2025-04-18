/*
 * gnote
 *
 * Copyright (C) 2010-2015,2017,2019-2025 Aurimas Cernius
 * Copyright (C) 2009 Debarshi Ray
 * Copyright (C) 2009 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/* this file might still contain Tomboy code see the  AUTHORS file for details */


#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>
#include <gtkmm/button.h>
#include <gtkmm/fontchooserdialog.h>
#include <gtkmm/linkbutton.h>
#include <gtkmm/notebook.h>
#include <gtkmm/separator.h>

#include "sharp/modulemanager.hpp"
#include "sharp/propertyeditor.hpp"
#include "synchronization/syncserviceaddin.hpp"
#include "iactionmanager.hpp"
#include "addinmanager.hpp"
#include "addinpreferencefactory.hpp"
#include "debug.hpp"
#include "ignote.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferencesdialog.hpp"
#include "preferences.hpp"
#include "utils.hpp"
#include "watchers.hpp"


#define DEFAULT_SYNC_CONFIGURED_CONFLICT_BEHAVIOR 0

#define NEW_PROPERTY_EDITOR_BOOL(property, check) new sharp::PropertyEditorBool([this]()->bool { return m_gnote.preferences().property(); }, \
          [this](bool v) { m_gnote.preferences().property(v); }, check);

namespace gnote {

namespace {

  class SyncService
    : public Glib::Object
  {
  public:
    static Glib::RefPtr<SyncService> create(sync::SyncServiceAddin *service)
      {
        return Glib::make_refptr_for_instance(new SyncService(service));
      }

    sync::SyncServiceAddin & service()
      {
        return *m_service;
      }
  private:
    SyncService(sync::SyncServiceAddin *service)
      : m_service(service)
      {}

    sync::SyncServiceAddin *m_service;
  };

  struct CompareSyncAddinsByName
  {
    bool operator()(sync::SyncServiceAddin *x, sync::SyncServiceAddin *y)
      {
        return x->name() < y->name();
      }
  };

  class AddinInfoDialog
    : public Gtk::Dialog
  {
  public:
    AddinInfoDialog(const AddinInfo & module, Gtk::Dialog &parent);
    void set_addin_id(const Glib::ustring & id)
      { m_id = id; }
    const Glib::ustring & get_addin_id() const
      { return m_id; }
  private:
    void fill(Gtk::Label &);
    AddinInfo m_addin_info;
    Glib::ustring m_id;
  };


  class ColorSchemeItem
    : public Glib::Object
  {
  public:
    static Glib::RefPtr<ColorSchemeItem> create(Glib::ustring &&label, Glib::ustring &&value)
      {
        return Glib::make_refptr_for_instance(new ColorSchemeItem(std::move(label), std::move(value)));
      }

    const Glib::ustring label;
    const Glib::ustring value;
  private:
    ColorSchemeItem(Glib::ustring &&label, Glib::ustring &&value)
      : label(std::move(label))
      , value(std::move(value))
      {}
  };

  Glib::RefPtr<Gtk::Expression<Glib::ustring>> make_color_scheme_label_expr()
  {
    return Gtk::ClosureExpression<Glib::ustring>::create([](const Glib::RefPtr<Glib::ObjectBase> &obj) -> Glib::ustring {
      if(auto item = std::dynamic_pointer_cast<ColorSchemeItem>(obj)) {
        return item->label;
      }

      ERR_OUT("Object is not ColorSchemeItem. This is a bug, please report it!");
      return "";
    });
  }

  guint color_scheme_item_idx(const Glib::RefPtr<Gio::ListStore<ColorSchemeItem>> &model, const Glib::ustring &value)
  {
    const auto count = model->get_n_items();
    for(guint i = 0; i < count; ++i) {
      if(value == model->get_item(i)->value) {
        return i;
      }
    }

    return 0;
  }
}


  PreferencesDialog::PreferencesDialog(IGnote & ignote, NoteManager & note_manager)
    : Gtk::Dialog()
    , m_sync_addin_combo(NULL)
    , m_selected_sync_addin(NULL)
    , m_sync_addin_prefs_container(NULL)
    , m_sync_addin_prefs_widget(NULL)
    , m_reset_sync_addin_button(NULL)
    , m_save_sync_addin_button(NULL)
    , m_rename_behavior_combo(NULL)
    , m_gnote(ignote)
    , m_addin_manager(note_manager.get_addin_manager())
    , m_note_manager(note_manager)
  {
    set_resizable(true);
    set_title(_("Gnote Preferences"));

    get_content_area()->get_parent()->set_margin(5);

    Gtk::Notebook *notebook = Gtk::make_managed<Gtk::Notebook>();
    notebook->set_margin(5);

    notebook->append_page(*make_editing_pane(), _("General"));
    notebook->append_page(*make_links_pane(), _("Links"));
    notebook->append_page(*make_sync_pane(), _("Synchronization"));
    notebook->append_page(*make_addins_pane(), _("Plugins"));

    get_content_area()->append(*notebook);

    Gtk::Button *button = Gtk::make_managed<Gtk::Button>(_("_Close"), true);
    add_action_widget(*button, Gtk::ResponseType::CLOSE);
    set_default_response(Gtk::ResponseType::CLOSE);

    m_gnote.preferences().signal_note_rename_behavior_changed.connect(
        sigc::mem_fun(*this, &PreferencesDialog::on_note_rename_behavior_changed));
    m_gnote.preferences().signal_sync_autosync_timeout_changed
      .connect(sigc::mem_fun(*this, &PreferencesDialog::on_autosync_timeout_setting_changed));
  }

  void PreferencesDialog::enable_addin(bool enable)
  {
    Glib::ustring id = get_selected_addin();
    sharp::DynamicModule * const module = m_addin_manager.get_module(id);
    if(!module) {
      return;
    }
    else {
      set_module_for_selected_addin(module);
    }

    if (module->has_interface(NoteAddin::IFACE_NAME)) {
      if (enable)
        m_addin_manager.add_note_addin_info(std::move(id), module);
      else
        m_addin_manager.erase_note_addin_info(id);
    }
    else {
      ApplicationAddin *addin = m_addin_manager.get_application_addin(id);
      if(addin) {
        enable_app_addin(addin, enable);
      }
      else {
        sync::SyncServiceAddin *sync_addin = m_addin_manager.get_sync_service_addin(id);
        if(sync_addin) {
          enable_sync_addin(sync_addin, enable);
        }
        else {
          ERR_OUT(_("Plugin %s is absent"), id.c_str());
          return;
        }
      }
    }

    module->enabled(enable);
    m_addin_manager.save_addins_prefs();
  }

  void PreferencesDialog::enable_app_addin(ApplicationAddin *addin, bool enable)
  {
    if(enable) {
      addin->initialize(m_gnote, m_note_manager);
    }
    else {
      addin->shutdown();
    }
  }


  void PreferencesDialog::enable_sync_addin(sync::SyncServiceAddin *addin, bool enable)
  {
    if(enable) {
      addin->initialize(m_gnote, m_gnote.sync_manager());
    }
    else {
      addin->shutdown();
    }
  }
  
  
  // Page 1
  // List of editing options
  Gtk::Widget *PreferencesDialog::make_editing_pane()
  {
      Gtk::Label *label;
      Gtk::CheckButton *check;
      sharp::PropertyEditorBool *font_peditor,* bullet_peditor;

      Gtk::Grid *options_list = Gtk::make_managed<Gtk::Grid>();
      options_list->set_row_spacing(12);
      options_list->set_margin(12);
      int options_list_row = 0;

      // Spell checking...

#if ENABLE_GSPELL
      // TODO I'm not sure there is a proper reason to do that.
      // it is in or NOT. if not, disable the UI.
      if (NoteSpellChecker::gtk_spell_available()) {
        check = manage(make_check_button (
                         _("_Spell check while typing")));
        set_widget_tooltip(*check, _("Misspellings will be underlined in red, with correct spelling "
                                     "suggestions shown in the context menu."));
        options_list->attach(*check, 0, options_list_row++, 1, 1);
        peditor = NEW_PROPERTY_EDITOR_BOOL(enable_spellchecking, *check);
        peditor->setup();
      }
#endif


      // Auto bulleted list
      check = make_check_button(_("Enable auto-_bulleted lists"));
      set_widget_tooltip(*check, _("Start new bulleted list by starting new line with character \"-\"."));
      options_list->attach(*check, 0, options_list_row++, 1, 1);
      bullet_peditor = NEW_PROPERTY_EDITOR_BOOL(enable_auto_bulleted_lists, *check);
      bullet_peditor->setup();

      // Custom font...
      check = make_check_button(_("Use custom _font"));
      check->set_hexpand(true);
      options_list->attach(*check, 0, options_list_row, 1, 1);
      font_peditor = NEW_PROPERTY_EDITOR_BOOL(enable_custom_font, *check);
      font_peditor->setup();

      font_button = manage(make_font_button());
      font_button->set_sensitive(check->get_active());
      font_button->set_hexpand(true);
      options_list->attach(*font_button, 1, options_list_row++, 1, 1);

      font_peditor->add_guard(font_button);

      // Note renaming behavior
      label = make_label(_("When renaming a linked note: "));
      label->set_hexpand(true);
      options_list->attach(*label, 0, options_list_row, 1, 1);
      std::vector<Glib::ustring> rename_opts;
      rename_opts.emplace_back(_("Ask me what to do"));
      rename_opts.emplace_back(_("Never rename links"));
      rename_opts.emplace_back(_("Always rename links"));
      m_rename_behavior_combo = Gtk::make_managed<Gtk::DropDown>(rename_opts);
      guint rename_behavior = m_gnote.preferences().note_rename_behavior();
      if (0 > rename_behavior || 2 < rename_behavior) {
        rename_behavior = 0;
        m_gnote.preferences().note_rename_behavior(rename_behavior);
      }
      m_rename_behavior_combo->set_selected(rename_behavior);
      m_rename_behavior_combo->property_selected().signal_changed().connect(sigc::mem_fun(*this, &PreferencesDialog::on_rename_behavior_changed));
      m_rename_behavior_combo->set_hexpand(true);
      options_list->attach(*m_rename_behavior_combo, 1, options_list_row++, 1, 1);

      // Color scheme
      label = make_label(_("Color Scheme: "));
      options_list->attach(*label, 0, options_list_row, 1, 1);
      auto color_scheme_model = Gio::ListStore<ColorSchemeItem>::create();
      // TRANSLATORS: Option to follow the system color scheme setting
      color_scheme_model->append(ColorSchemeItem::create(_("Follow the System"), ""));
      // TRANSLATORS: Option to use dark variant of the theme
      color_scheme_model->append(ColorSchemeItem::create(_("Dark"), Preferences::COLOR_SCHEME_DARK_VAL));
      // TRANSLATORS: Option to use light variant of the theme
      color_scheme_model->append(ColorSchemeItem::create(_("Light"), Preferences::COLOR_SCHEME_LIGHT_VAL));
      auto color_scheme = Gtk::make_managed<Gtk::DropDown>(color_scheme_model, make_color_scheme_label_expr());
      color_scheme->set_selected(color_scheme_item_idx(color_scheme_model, m_gnote.preferences().color_scheme()));
      color_scheme->property_selected().signal_changed().connect([color_scheme, &gnote=m_gnote] {
        if(auto item = std::dynamic_pointer_cast<ColorSchemeItem>(color_scheme->get_selected_item())) {
          gnote.preferences().color_scheme(item->value);
        }
      });
      options_list->attach(*color_scheme, 1, options_list_row++, 1, 1);

      // New Note Template
      // TRANSLATORS: This is 'New Note' Template, not New 'Note Template'
      label = make_label(_("New Note Template"));
      set_widget_tooltip(*label, _("Use the new note template to specify the text "
                                   "that should be used when creating a new note."));
      label->set_hexpand(true);
      options_list->attach(*label, 0, options_list_row, 1, 1);
      
      Gtk::Button *open_template_button = Gtk::make_managed<Gtk::Button>(_("Open New Note Template"));
      open_template_button->signal_clicked().connect(
        sigc::mem_fun(*this, &PreferencesDialog::open_template_button_clicked));
      options_list->attach(*open_template_button, 1, options_list_row++, 1, 1);

      return options_list;
    }

  Gtk::Button *PreferencesDialog::make_font_button()
  {
    Gtk::Grid *font_box = Gtk::make_managed<Gtk::Grid>();

    font_face = Gtk::make_managed<Gtk::Label>();
    font_face->set_use_markup(true);
    font_face->set_hexpand(true);
    font_box->attach(*font_face, 0, 0, 1, 1);

    Gtk::Separator *sep = Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::VERTICAL);
    font_box->attach(*sep, 1, 0, 1, 1);

    font_size = Gtk::make_managed<Gtk::Label>();
    font_size->set_margin_start(4);
    font_size->set_margin_end(4);
    font_box->attach(*font_size, 2, 0, 1, 1);

    Gtk::Button *button = Gtk::make_managed<Gtk::Button>();
    button->signal_clicked().connect(sigc::mem_fun(*this, &PreferencesDialog::on_font_button_clicked));
    button->set_child(*font_box);

    update_font_button(m_gnote.preferences().custom_font_face());

    return button;
  }

  Gtk::Widget *PreferencesDialog::make_links_pane()
  {
    auto vbox = Gtk::make_managed<Gtk::Grid>();
    vbox->set_row_spacing(12);
    vbox->set_margin(12);

    Gtk::CheckButton *check;
    sharp::PropertyEditorBool *peditor;
    int vbox_row = 0;

    // internal links
    check = make_check_button(_("_Automatically link to notes"));
    set_widget_tooltip(*check, _("Enable this option to create a link when text matches note title."));
    vbox->attach(*check, 0, vbox_row++, 1, 1);
    peditor = NEW_PROPERTY_EDITOR_BOOL(enable_auto_links, *check);
    peditor->setup();

    // URLs
    check = make_check_button(_("Create links for _URLs"));
    set_widget_tooltip(*check, _("Enable this option to create links for URLs. "
                                 "Clicking will open URL with appropriate program."));
    vbox->attach(*check, 0, vbox_row++, 1, 1);
    peditor = NEW_PROPERTY_EDITOR_BOOL(enable_url_links, *check);
    peditor->setup();

    // WikiWords...
    check = make_check_button(_("Highlight _WikiWords"));
    set_widget_tooltip(*check, _("Enable this option to highlight words <b>ThatLookLikeThis</b>. "
                                 "Clicking the word will create a note with that name."));
    vbox->attach(*check, 0, vbox_row++, 1, 1);
    peditor = NEW_PROPERTY_EDITOR_BOOL(enable_wikiwords, *check);
    peditor->setup();

    return vbox;
  }


  Gtk::Widget *PreferencesDialog::make_sync_pane()
  {
    auto vbox = Gtk::make_managed<Gtk::Grid>();
    vbox->set_row_spacing(4);
    vbox->set_margin(8);
    int vbox_row = 0;

    auto hbox = Gtk::make_managed<Gtk::Grid>();
    hbox->set_column_spacing(6);
    int hbox_col = 0;

    auto label = make_label(_("Ser_vice:"));
    hbox->attach(*label, hbox_col++, 0, 1, 1);

    // Populate the store with all the available SyncServiceAddins
    auto sync_addin_store = Gio::ListStore<SyncService>::create();
    std::vector<sync::SyncServiceAddin*> addins = m_addin_manager.get_sync_service_addins();
    std::sort(addins.begin(), addins.end(), CompareSyncAddinsByName());
    for(auto addin : addins) {
      if(addin->initialized()) {
	sync_addin_store->append(SyncService::create(addin));
      }
    }

    auto expr = Gtk::ClosureExpression<Glib::ustring>::create([](const Glib::RefPtr<Glib::ObjectBase> & o) {
      return std::dynamic_pointer_cast<SyncService>(o)->service().name();
    });
    m_sync_addin_combo = Gtk::make_managed<Gtk::DropDown>(sync_addin_store, expr);
    label->set_mnemonic_widget(*m_sync_addin_combo);

    // Read from Preferences which service is configured and select it
    // by default.  Otherwise, just select the first one in the list.
    Glib::ustring addin_id = m_gnote.preferences().sync_selected_service_addin();

    Glib::RefPtr<SyncService> active_sync;
    if(!addin_id.empty()) {
      for(guint i = 0; i < sync_addin_store->get_n_items(); ++i) {
        auto item = sync_addin_store->get_item(i);
        if(item->service().id() == addin_id) {
          active_sync = item;
          m_sync_addin_combo->set_selected(i);
          break;
        }
      }
    }
    if(!active_sync && sync_addin_store->get_n_items() > 0) {
      active_sync = sync_addin_store->get_item(0);
    }

    m_sync_addin_combo->property_selected_item().signal_changed().connect([this]{
      auto item = std::dynamic_pointer_cast<SyncService>(m_sync_addin_combo->get_selected_item());
      sync::SyncServiceAddin *new_addin = item ? &item->service() : nullptr;
      if(new_addin != m_selected_sync_addin) {
        m_selected_sync_addin = new_addin;
        on_sync_addin_combo_changed();
      }
    });

    m_sync_addin_combo->set_hexpand(true);
    hbox->attach(*m_sync_addin_combo, hbox_col++, 0, 1, 1);

    vbox->attach(*hbox, 0, vbox_row++, 1, 1);

    m_sync_addin_prefs_container = Gtk::make_managed<Gtk::Grid>();
    m_sync_addin_prefs_container->set_hexpand(true);
    m_sync_addin_prefs_container->set_vexpand(true);
    vbox->attach(*m_sync_addin_prefs_container, 0, vbox_row++, 1, 1);

    // Autosync preference
    int timeout = m_gnote.preferences().sync_autosync_timeout();
    if(timeout > 0 && timeout < 5) {
      timeout = 5;
      m_gnote.preferences().sync_autosync_timeout(5);
    }
    auto autosyncBox = Gtk::make_managed<Gtk::Grid>();
    autosyncBox->set_column_spacing(5);
    m_autosync_check = make_check_button(_("Automatic background s_ync interval (minutes)"));
    m_autosync_spinner = Gtk::make_managed<Gtk::SpinButton>(1);
    m_autosync_spinner->set_range(5, 1000);
    m_autosync_spinner->set_value(timeout >= 5 ? timeout : 10);
    m_autosync_spinner->set_increments(1, 5);
    m_autosync_spinner->set_hexpand(true);
    m_autosync_check->set_active(timeout >= 5);
    m_autosync_spinner->set_sensitive(timeout >= 5);
    m_autosync_check->signal_toggled()
      .connect(sigc::mem_fun(*this, &PreferencesDialog::on_autosync_check_toggled));
    m_autosync_spinner->signal_value_changed()
      .connect(sigc::mem_fun(*this, &PreferencesDialog::update_timeout_pref));

    autosyncBox->attach(*m_autosync_check, 0, 0, 1, 1);
    autosyncBox->attach(*m_autosync_spinner, 1, 0, 1, 1);
    autosyncBox->set_hexpand(true);
    vbox->attach(*autosyncBox, 0, vbox_row++, 1, 1);

    auto bbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    bbox->set_spacing(4);

    // "Advanced..." button to bring up extra sync config dialog
    auto advancedConfigButton = Gtk::make_managed<Gtk::Button>(_("_Advanced..."), true);
    advancedConfigButton->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_advanced_sync_config_button));
    bbox->append(*advancedConfigButton);

    m_reset_sync_addin_button = Gtk::make_managed<Gtk::Button>(_("Clear"));
    m_reset_sync_addin_button->signal_clicked().connect([this]() {
      on_reset_sync_addin_button(true);
    });

    bool current_sync_configured = active_sync && addin_id == active_sync->service().id() && active_sync->service().is_configured();
    m_reset_sync_addin_button->set_sensitive(current_sync_configured);
    bbox->append(*m_reset_sync_addin_button);

    // TODO: Tabbing should go directly from sync prefs widget to here
    // TODO: Consider connecting to "Enter" pressed in sync prefs widget
    m_save_sync_addin_button = Gtk::make_managed<Gtk::Button>(_("_Save"), true);
    m_save_sync_addin_button->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_save_sync_addin_button));
    m_save_sync_addin_button->set_sensitive(current_sync_configured);
    bbox->append(*m_save_sync_addin_button);

    m_sync_addin_combo->set_sensitive(!current_sync_configured);

    vbox->attach(*bbox, 0, vbox_row++, 1, 1);

    // Get the preferences GUI for the Sync Addin
    if(active_sync) {
      m_selected_sync_addin = &active_sync->service();
      on_sync_addin_combo_changed();
      m_reset_sync_addin_button->set_sensitive(true);
    }
    if (m_sync_addin_prefs_widget == NULL) {
      auto l = make_label(_("Not configurable"));
      m_sync_addin_prefs_widget = l;
      m_sync_addin_prefs_container->attach(*m_sync_addin_prefs_widget, 0, 0, 1, 1);
    }
    if (m_sync_addin_prefs_widget && current_sync_configured) {
      m_sync_addin_prefs_widget->set_sensitive(false);
    }

    return vbox;
  }


  // Page 3
  // Extension Preferences
  Gtk::Widget *PreferencesDialog::make_addins_pane()
  {
    auto vbox = Gtk::make_managed<Gtk::Grid>();
    vbox->set_row_spacing(6);
    vbox->set_margin(6);
    int vbox_row = 0;
    auto l = Gtk::make_managed<Gtk::Label>(_("The following plugins are installed:"), true);
    l->property_xalign() = 0;
    vbox->attach(*l, 0, vbox_row++, 1, 1);

    auto hbox = Gtk::make_managed<Gtk::Grid>();
    hbox->set_column_spacing(6);
    int hbox_col = 0;

    // TreeView of Add-ins
    m_plugin_view = Gtk::make_managed<Gtk::ColumnView>();
    m_plugin_model = sharp::AddinsModel::create(m_plugin_view);

    auto sw = Gtk::make_managed<Gtk::ScrolledWindow>();
    sw->property_hscrollbar_policy() = Gtk::PolicyType::AUTOMATIC;
    sw->property_vscrollbar_policy() = Gtk::PolicyType::AUTOMATIC;
    sw->set_child(*m_plugin_view);
    sw->set_hexpand(true);
    sw->set_vexpand(true);
    hbox->attach(*sw, hbox_col++, 0, 1, 1);

    // Action Buttons (right of TreeView)
    auto button_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    button_box->set_spacing(4);

    enable_addin_button = Gtk::make_managed<Gtk::Button>(_("_Enable"), true);
    enable_addin_button->set_sensitive(false);
    enable_addin_button->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_enable_addin_button));

    disable_addin_button = Gtk::make_managed<Gtk::Button>(_("_Disable"), true);
    disable_addin_button->set_sensitive(false);
    disable_addin_button->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_disable_addin_button));

    addin_prefs_button = Gtk::make_managed<Gtk::Button>(_("_Preferences"), true);
    addin_prefs_button->set_sensitive(false);
    addin_prefs_button->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_addin_prefs_button));

    addin_info_button = Gtk::make_managed<Gtk::Button>(_("In_formation"), true);
    addin_info_button->set_sensitive(false);
    addin_info_button->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_addin_info_button));

    button_box->append(*enable_addin_button);
    button_box->append(*disable_addin_button);
    button_box->append(*addin_prefs_button);
    button_box->append(*addin_info_button);

    hbox->attach(*button_box, hbox_col++, 0, 1, 1);

    vbox->attach(*hbox, 0, vbox_row++, 1, 1);

    m_plugin_model->signal_selection_changed.connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_plugin_view_selection_changed));
    load_addins();

    return vbox;
  }


  Glib::ustring PreferencesDialog::get_selected_addin()
  {
    auto item = m_plugin_model->get_selected_plugin();
    if(item) {
      return item->info.id();
    }
    return "";
  }


  void PreferencesDialog::set_module_for_selected_addin(sharp::DynamicModule * module)
  {
    auto plugin = m_plugin_model->get_selected_plugin();
    if(plugin) {
      plugin->set_module(module);
    }
  }


  void PreferencesDialog::on_plugin_view_selection_changed(const Glib::RefPtr<sharp::Plugin> & plugin)
  {
    update_addin_buttons(plugin);
  }


  /// Set the sensitivity of the buttons based on what is selected
  void PreferencesDialog::update_addin_buttons()
  {
    update_addin_buttons(m_plugin_model->get_selected_plugin());
  }

  void PreferencesDialog::update_addin_buttons(const Glib::RefPtr<sharp::Plugin> & plugin)
  {
    if(plugin) {
      auto id = plugin->info.id();
      bool loaded = m_addin_manager.is_module_loaded(id);
      bool enabled = false;
      if(loaded) {
        const sharp::DynamicModule *module = m_addin_manager.get_module(id);
        enabled = module->is_enabled();
        addin_prefs_button->set_sensitive(
          module->has_interface(AddinPreferenceFactoryBase::IFACE_NAME));
      }
      else {
        addin_prefs_button->set_sensitive(false);
      }
      enable_addin_button->set_sensitive(!enabled);
      disable_addin_button->set_sensitive(enabled);
      addin_info_button->set_sensitive(true);
    }
    else {
      enable_addin_button->set_sensitive(false);
      disable_addin_button->set_sensitive(false);
      addin_prefs_button->set_sensitive(false);
      addin_info_button->set_sensitive(false);
    }
  }


  void PreferencesDialog::load_addins()
  {
    ///// TODO populate
    const AddinInfoMap & addins(m_addin_manager.get_addin_infos());
    for(AddinInfoMap::const_iterator iter = addins.begin();
        iter != addins.end(); ++iter) {
      sharp::DynamicModule *module = NULL;
      if(m_addin_manager.is_module_loaded(iter->first)) {
        module = m_addin_manager.get_module(iter->first);
      }
      m_plugin_model->append(iter->second, module);
    }

    update_addin_buttons();
  }


  void PreferencesDialog::on_enable_addin_button()
  {
    enable_addin(true);
    update_addin_buttons();
    update_sync_services();
  }

  void PreferencesDialog::on_disable_addin_button()
  {
    enable_addin(false);
    update_addin_buttons();
    update_sync_services();
  }


  void PreferencesDialog::on_addin_prefs_button()
  {
    Glib::ustring id = get_selected_addin();
    AddinInfo addin_info = m_addin_manager.get_addin_info(id);
    const sharp::DynamicModule *module = m_addin_manager.get_module(id);
    Gtk::Dialog *dialog;

    if (!module) {
      return;
    }

    auto iter = addin_prefs_dialogs.find(id);
    if (iter == addin_prefs_dialogs.end()) {
      // A preference dialog isn't open already so create a new one
      auto icon = Gtk::make_managed<Gtk::Image>();
      icon->set_from_icon_name("preferences-system");
      auto caption = Gtk::make_managed<Gtk::Label>();
      caption->set_markup(
        Glib::ustring::compose("<span size='large' weight='bold'>%1 %2</span>", 
            addin_info.name(), addin_info.version()));
      caption->property_xalign() = 0;
      caption->set_use_markup(true);
      caption->set_use_underline(false);
      caption->set_hexpand(true);

      Gtk::Widget *pref_widget = m_addin_manager.create_addin_preference_widget(id);

      if (pref_widget == NULL) {
        pref_widget = Gtk::make_managed<Gtk::Label>(_("Not Implemented"));
      }
      
      Gtk::Grid *hbox = Gtk::make_managed<Gtk::Grid>();
      hbox->set_column_spacing(6);
      Gtk::Grid *vbox = Gtk::make_managed<Gtk::Grid>();
      vbox->set_row_spacing(6);
      vbox->set_margin(6);
      hbox->attach(*icon, 0, 0, 1, 1);
      hbox->attach(*caption, 1, 0, 1, 1);
      vbox->attach(*hbox, 0, 0, 1, 1);
      vbox->set_expand(true);

      vbox->attach(*pref_widget, 0, 1, 1, 1);

      dialog = Gtk::make_managed<Gtk::Dialog>(
        // TRANSLATORS: %1 is the placeholder for the addin name.
        Glib::ustring::compose(_("%1 Preferences"), addin_info.name()),
        *this, false);
      dialog->property_destroy_with_parent() = true;
      dialog->add_button(_("_Close"), Gtk::ResponseType::CLOSE);

      dialog->get_content_area()->append(*vbox);
      dialog->signal_response().connect([this, dialog, id](int) {
        addin_pref_dialog_response(id, dialog);
      });

      // Store this dialog off in the dictionary so it can be
      // presented again if the user clicks on the preferences button
      // again before closing the preferences dialog.
//      dialog->set_data(Glib::Quark("AddinId"), module->id());
      addin_prefs_dialogs[id] = dialog;
    } 
    else {
      // It's already opened so just present it again
      dialog = iter->second;
    }

    dialog->present ();
  }


  void PreferencesDialog::addin_pref_dialog_response(const Glib::ustring & addin_id, Gtk::Dialog* dialog)
  {
    dialog->hide();
    addin_prefs_dialogs.erase(addin_id);
  }

  void PreferencesDialog::on_addin_info_button()
  {
    Glib::ustring id = get_selected_addin();
    AddinInfo addin = m_addin_manager.get_addin_info(id);

    Gtk::Dialog* dialog;
    auto iter = addin_info_dialogs.find(addin.id());
    if (iter == addin_info_dialogs.end()) {
      dialog = Gtk::make_managed<AddinInfoDialog>(addin, *this);
      dialog->signal_response().connect(
        sigc::bind(
          sigc::mem_fun(*this, &PreferencesDialog::addin_info_dialog_response),
          dialog));

      // Store this dialog off in a dictionary so it can be presented
      // again if the user clicks on the Info button before closing
      // the original dialog.
      static_cast<AddinInfoDialog*>(dialog)->set_addin_id(id);
      addin_info_dialogs[id] = dialog;
    } 
    else {
      // It's already opened so just present it again
      dialog = iter->second;
    }

    dialog->present ();
  }

  void PreferencesDialog::addin_info_dialog_response(int, Gtk::Dialog* dlg)
  {
    dlg->hide();
    AddinInfoDialog *addin_dialog = static_cast<AddinInfoDialog*>(dlg);
    addin_info_dialogs.erase(addin_dialog->get_addin_id());
  }



  Gtk::Label *PreferencesDialog::make_label(const Glib::ustring & label_text/*, params object[] args*/)
  {
//    if (args.Length > 0)
//      label_text = String.Format (label_text, args);

    auto label = Gtk::make_managed<Gtk::Label>(label_text, true);

    label->set_use_markup(true);
    label->set_justify(Gtk::Justification::LEFT);
    label->set_valign(Gtk::Align::CENTER);

    return label;
  }

  Gtk::CheckButton *PreferencesDialog::make_check_button(const Glib::ustring & label_text)
  {
    Gtk::CheckButton *check = Gtk::make_managed<Gtk::CheckButton>(label_text, true);
    return check;
  }


  void PreferencesDialog::set_widget_tooltip(Gtk::Widget & widget, Glib::ustring label_text)
  {
    widget.set_tooltip_markup(Glib::ustring::compose("<small>%1</small>", label_text));
  }

  void PreferencesDialog::on_font_button_clicked()
  {
    auto font_dialog = Gtk::make_managed<Gtk::FontChooserDialog>(_("Choose Note Font"));

    auto font_name = m_gnote.preferences().custom_font_face();
    font_dialog->set_font(font_name);

    font_dialog->signal_response().connect([this, font_name, font_dialog](int response) {
      if(Gtk::ResponseType::OK == response) {
        auto new_font = font_dialog->get_font();
        if(new_font != font_name) {
          m_gnote.preferences().custom_font_face(new_font);
          update_font_button(new_font);
        }
      }
      font_dialog->hide();
    });
    font_dialog->show();
  }

  void PreferencesDialog::update_font_button(const Glib::ustring & font_desc)
  {
    PangoFontDescription *desc = pango_font_description_from_string(font_desc.c_str());

    // Set the size label
    font_size->set_text(TO_STRING(pango_font_description_get_size(desc) / PANGO_SCALE));

    pango_font_description_unset_fields(desc, PANGO_FONT_MASK_SIZE);

    // Set the font name label
    char * descstr = pango_font_description_to_string(desc);
    font_face->set_markup(Glib::ustring::compose("<span font_desc='%1'>%2</span>",
                              font_desc, Glib::ustring(descstr)));
    g_free(descstr);
    pango_font_description_free(desc);
  }



  void PreferencesDialog::open_template_button_clicked()
  {
    // Open the template note
    m_gnote.open_note(m_note_manager.get_or_create_template_note());
  }



  void  PreferencesDialog::on_note_rename_behavior_changed()
  {
    guint rename_behavior = m_gnote.preferences().note_rename_behavior();
    if(0 > rename_behavior || 2 < rename_behavior) {
      rename_behavior = 0;
      m_gnote.preferences().note_rename_behavior(rename_behavior);
    }
    if(m_rename_behavior_combo->get_selected() != rename_behavior) {
      m_rename_behavior_combo->set_selected(rename_behavior);
    }
  }



  void PreferencesDialog::on_autosync_timeout_setting_changed()
  {
    int timeout = m_gnote.preferences().sync_autosync_timeout();
    if(timeout <= 0 && m_autosync_check->get_active()) {
      m_autosync_check->set_active(false);
    }
    else if(timeout > 0) {
      timeout = (timeout >= 5 && timeout < 1000) ? timeout : 5;
      if(!m_autosync_check->get_active()) {
        m_autosync_check->set_active(true);
      }
      if(static_cast<int>(m_autosync_spinner->get_value()) != timeout) {
        m_autosync_spinner->set_value(timeout);
      }
    }
  }



  void  PreferencesDialog::on_rename_behavior_changed()
  {
    m_gnote.preferences().note_rename_behavior(m_rename_behavior_combo->get_selected());
  }


  void PreferencesDialog::on_advanced_sync_config_button()
  {
    // Get saved behavior
    sync::SyncTitleConflictResolution savedBehavior = sync::CANCEL;
    int dlgBehaviorPref = m_gnote.preferences().sync_configured_conflict_behavior();
    // TODO: Check range of this int
    savedBehavior = static_cast<sync::SyncTitleConflictResolution>(dlgBehaviorPref);

    // Create dialog
    Gtk::Dialog *advancedDlg = Gtk::make_managed<Gtk::Dialog>(_("Other Synchronization Options"), *this, true);
    // Populate dialog
    Gtk::Label *label = Gtk::make_managed<Gtk::Label>(
      _("When a conflict is detected between a local note and a note on the configured synchronization server:"));
    label->set_wrap(true);
    label->set_margin(6);

    auto promptOnConflictRadio = Gtk::make_managed<Gtk::CheckButton>(_("Always ask me what to do"));
    auto renameOnConflictRadio = Gtk::make_managed<Gtk::CheckButton>(_("Rename my local note"));
    renameOnConflictRadio->set_group(*promptOnConflictRadio);
    auto overwriteOnConflictRadio = Gtk::make_managed<Gtk::CheckButton>(_("Replace my local note with the server's update"));
    overwriteOnConflictRadio->set_group(*promptOnConflictRadio);

    auto on_toggle = [this, renameOnConflictRadio, overwriteOnConflictRadio] {
      sync::SyncTitleConflictResolution newBehavior = sync::CANCEL;

      if(renameOnConflictRadio->get_active()) {
        newBehavior = sync::RENAME_EXISTING_NO_UPDATE;
      }
      else if(overwriteOnConflictRadio->get_active()) {
        newBehavior = sync::OVERWRITE_EXISTING;
      }

      m_gnote.preferences().sync_configured_conflict_behavior(static_cast<int>(newBehavior));
    };

    promptOnConflictRadio->signal_toggled().connect(on_toggle);
    renameOnConflictRadio->signal_toggled().connect(on_toggle);
    overwriteOnConflictRadio->signal_toggled().connect(on_toggle);

    switch(savedBehavior) {
    case sync::RENAME_EXISTING_NO_UPDATE:
      renameOnConflictRadio->set_active(true);
      break;
    case sync::OVERWRITE_EXISTING:
      overwriteOnConflictRadio->set_active(true);
      break;
    default:
      promptOnConflictRadio->set_active(true);
      break;
    }

    Gtk::Grid *vbox = Gtk::make_managed<Gtk::Grid>();
    vbox->set_margin(18);

    vbox->attach(*promptOnConflictRadio, 0, 0, 1, 1);
    vbox->attach(*renameOnConflictRadio, 0, 1, 1, 1);
    vbox->attach(*overwriteOnConflictRadio, 0, 2, 1, 1);

    advancedDlg->get_content_area()->append(*label);
    advancedDlg->get_content_area()->append(*vbox);
    advancedDlg->add_button(_("_Close"), Gtk::ResponseType::OK);

    advancedDlg->show();
    advancedDlg->signal_response().connect([advancedDlg](int) { advancedDlg->hide(); });
  }


  void PreferencesDialog::on_sync_addin_combo_changed()
  {
    auto item = std::dynamic_pointer_cast<SyncService>(m_sync_addin_combo->get_selected_item());
    sync::SyncServiceAddin *new_addin = item ? &item->service() : nullptr;

    if(m_sync_addin_prefs_widget != NULL) {
      m_sync_addin_prefs_container->remove(*m_sync_addin_prefs_widget);
      m_sync_addin_prefs_widget = NULL;
    }

    if(new_addin != NULL) {
      m_sync_addin_prefs_widget = new_addin->create_preferences_control(
        *this, sigc::mem_fun(*this, &PreferencesDialog::on_sync_addin_prefs_changed));
      if(m_sync_addin_prefs_widget == NULL) {
        auto l = Gtk::make_managed<Gtk::Label>(_("Not configurable"));
        l->set_halign(Gtk::Align::CENTER);
        l->set_valign(Gtk::Align::CENTER);
        m_sync_addin_prefs_widget = l;
      }

      m_sync_addin_prefs_container->attach(*m_sync_addin_prefs_widget, 0, 0, 1, 1);

      m_reset_sync_addin_button->set_sensitive(false);
      m_save_sync_addin_button->set_sensitive(false);
    }
    else {
      m_selected_sync_addin = NULL;
      m_reset_sync_addin_button->set_sensitive(false);
      m_save_sync_addin_button->set_sensitive(false);
    }
  }


  void PreferencesDialog::on_reset_sync_addin_button(bool signal)
  {
    auto active_sync = std::dynamic_pointer_cast<SyncService>(m_sync_addin_combo->get_selected_item());
    if(!active_sync) {
      return;
    }

    auto after_dialog = [this, active_sync] {
      try {
        active_sync->service().reset_configuration();
      }
      catch(std::exception & e) {
        DBG_OUT("Error calling %s.reset_configuration: %s", active_sync->service().id().c_str(), e.what());
      }

      m_gnote.preferences().sync_selected_service_addin("");

      // Reset conflict handling behavior
      m_gnote.preferences().sync_configured_conflict_behavior(DEFAULT_SYNC_CONFIGURED_CONFLICT_BEHAVIOR);

      m_gnote.sync_manager().reset_client();

      m_sync_addin_combo->set_sensitive(true);
      m_sync_addin_combo->set_selected(GTK_INVALID_LIST_POSITION);
      m_reset_sync_addin_button->set_sensitive(false);
      m_save_sync_addin_button->set_sensitive(true);
      if(m_sync_addin_prefs_widget) {
        m_sync_addin_prefs_widget->set_sensitive(true);
      }
    };

    Gtk::Dialog *dialog;

    // User doesn't get a choice if this is invoked by disabling the addin
    // FIXME: null sender check is lame!
    if(signal) {
      // Prompt the user about what they're about to do since
      // it's not really recommended to switch back and forth
      // between sync services.
      dialog = Gtk::make_managed<utils::HIGMessageDialog>(this, GTK_DIALOG_MODAL, Gtk::MessageType::QUESTION,
        Gtk::ButtonsType::YES_NO, _("Are you sure?"),
        _("Clearing your synchronization settings is not recommended.  "
          "You may be forced to synchronize all of your notes again when you save new settings."));
      dialog->signal_response().connect([dialog, after_dialog](int dialog_response) {
        dialog->hide();
        if(dialog_response == Gtk::ResponseType::YES) {
          after_dialog();
        }
      });
    }
    else { // FIXME: Weird place for this to go.  User should be able to cancel disabling of addin, anyway
      dialog = Gtk::make_managed<utils::HIGMessageDialog>(this, GTK_DIALOG_MODAL, Gtk::MessageType::INFO,
        Gtk::ButtonsType::OK, _("Resetting Synchronization Settings"),
        _("You have disabled the configured synchronization service.  "
          "Your synchronization settings will now be cleared.  "
          "You may be forced to synchronize all of your notes again when you save new settings."));
      dialog->signal_response().connect([dialog, after_dialog](int) {
        dialog->hide();
        after_dialog();
      });
    }

    dialog->show();
  }

  void PreferencesDialog::on_save_sync_addin_button()
  {
    auto active_sync = std::dynamic_pointer_cast<SyncService>(m_sync_addin_combo->get_selected_item());
    if(!active_sync) {
      return;
    }

    bool saved = false;
    Glib::ustring errorMsg;
    try {
      set_cursor("wait");
      set_sensitive(false);
      saved = active_sync->service().save_configuration([this, active_sync](bool saved, const Glib::ustring & err) {
        on_sync_settings_saved(active_sync, saved, err);
      });
    }
    catch(sync::GnoteSyncException & e) {
      errorMsg = e.what();
    }
    catch(std::exception & e) {
      DBG_OUT("Unexpected error calling %s.save_configuration: %s", active_sync->service().id().c_str(), e.what());
    }

    if(!saved) {
      on_sync_settings_saved(active_sync, saved, errorMsg);
    }
  }


  void PreferencesDialog::on_sync_settings_saved(const Glib::RefPtr<Glib::Object> & active_sync_service, bool saved, Glib::ustring errorMsg)
  {
    set_sensitive(true);
    set_cursor("");

    utils::HIGMessageDialog *dialog;
    if(saved) {
      auto active_sync = std::dynamic_pointer_cast<SyncService>(active_sync_service);
      m_gnote.preferences().sync_selected_service_addin(active_sync->service().id());

      m_sync_addin_combo->set_sensitive(false);
      m_sync_addin_prefs_widget->set_sensitive(false);
      m_reset_sync_addin_button->set_sensitive(true);
      m_save_sync_addin_button->set_sensitive(false);

      m_gnote.sync_manager().reset_client();

      // Give the user a visual letting them know that connecting
      // was successful.
      // TODO: Replace Yes/No with Sync/Close
      dialog = Gtk::make_managed<utils::HIGMessageDialog>(this, GTK_DIALOG_MODAL, Gtk::MessageType::INFO, Gtk::ButtonsType::YES_NO,
        _("Connection successful"),
        _("Gnote is ready to synchronize your notes. Would you like to synchronize them now?"));
      dialog->show();
      dialog->signal_response().connect([this, dialog](int dialog_response ) {
        dialog->hide();
        if(dialog_response == Gtk::ResponseType::YES) {
          // TODO: Put this voodoo in a method somewhere
          auto action = m_gnote.action_manager().get_app_action("sync-notes");
          utils::main_context_invoke([action = std::move(action)]() { action->activate(Glib::VariantBase()); });
        }
      });
    }
    else {
      // TODO: Change the SyncServiceAddin API so the call to
      // SaveConfiguration has a way of passing back an exception
      // or other text so it can be displayed to the user.
      m_gnote.preferences().sync_selected_service_addin("");

      m_sync_addin_combo->set_sensitive(true);
      m_sync_addin_prefs_widget->set_sensitive(true);
      m_reset_sync_addin_button->set_sensitive(false);
      m_save_sync_addin_button->set_sensitive(true);

      // Give the user a visual letting them know that connecting
      // was successful.
      if(errorMsg == "") {
        // TRANSLATORS: %1 is the placeholder for the log file path.
        errorMsg = _("Please check your information and try again.  The log file %1 may contain more information about the error.");
        Glib::ustring logPath = Glib::build_filename(Glib::get_home_dir(), "gnote.log");
        errorMsg = Glib::ustring::compose(errorMsg, logPath);
      }
      dialog = Gtk::make_managed<utils::HIGMessageDialog>(this, GTK_DIALOG_MODAL, Gtk::MessageType::WARNING, Gtk::ButtonsType::CLOSE,
        _("Error connecting"), errorMsg);
      dialog->show();
      dialog->signal_response().connect([dialog](int) { dialog->hide(); });
    }
  }


  void PreferencesDialog::on_sync_addin_prefs_changed()
  {
    // Enable/disable the save button based on required fields
    auto active_sync = std::dynamic_pointer_cast<SyncService>(m_sync_addin_combo->get_selected_item());
    if(active_sync) {
      m_save_sync_addin_button->set_sensitive(active_sync->service().are_settings_valid());
    }
  }


  AddinInfoDialog::AddinInfoDialog(const AddinInfo & addin_info,
                                   Gtk::Dialog &parent)
    : Gtk::Dialog(addin_info.name(), parent, false)
    , m_addin_info(addin_info)
  {
    property_destroy_with_parent() = true;
    add_button(_("_Close"), Gtk::ResponseType::CLOSE);
    set_modal(true);
    
    // TODO: Change this icon to be an addin/package icon
    Gtk::Image *icon = Gtk::make_managed<Gtk::Image>();
    icon->set_from_icon_name("dialog-information");
    icon->set_halign(Gtk::Align::START);

    auto info_label = Gtk::make_managed<Gtk::Label>();
    info_label->property_xalign() = 0;
    info_label->property_yalign() = 0;
    info_label->set_use_markup(true);
    info_label->set_use_underline(false);
    info_label->property_wrap() = true;
    info_label->set_max_width_chars(60);
    info_label->set_hexpand(true);
    info_label->set_vexpand(true);

    auto hbox = Gtk::make_managed<Gtk::Grid>();
    hbox->set_column_spacing(6);
    auto vbox = Gtk::make_managed<Gtk::Grid>();
    vbox->set_row_spacing(12);
    hbox->set_margin(12);
    hbox->set_expand(true);
    vbox->set_margin(6);

    hbox->attach(*icon, 0, 0, 1, 1);
    hbox->attach(*vbox, 1, 0, 1, 1);

    vbox->attach(*info_label, 0, 0, 1, 1);

    get_content_area()->append(*hbox);

    fill(*info_label);
  }

  void AddinInfoDialog::fill(Gtk::Label & info_label)
  {
    Glib::ustring sb = "<b><big>" + m_addin_info.name() + "</big></b>\n\n";
    sb += m_addin_info.description() + "\n\n";

    sb += Glib::ustring::compose("<small><b>%1</b>\n%2\n\n",
              _("Version:"), m_addin_info.version());

    sb += Glib::ustring::compose("<b>%1</b>\n%2\n\n",
              _("Author:"), m_addin_info.authors());
    
    Glib::ustring s = m_addin_info.copyright();
    if(s != "") {
      sb += Glib::ustring::compose("<b>%1</b>\n%2\n\n", _("Copyright:"), s);
    }

#if 0 // TODO handle module dependencies
    if (info.Dependencies.Count > 0) {
      sb.Append (string.Format (
                   "<b>{0}</b>\n",
                   Catalog.GetString ("Add-in Dependencies:")));
      foreach (Mono.Addins.Description.Dependency dep in info.Dependencies) {
        sb.Append (dep.Name + "\n");
      }
    }
#endif

    sb += "</small>";

    info_label.set_markup(sb);
  }

  void PreferencesDialog::update_sync_services()
  {
    std::vector<sync::SyncServiceAddin*> new_addins = m_addin_manager.get_sync_service_addins();
    auto remove_iter = new_addins.begin();
    while(remove_iter != new_addins.end()) {
      if(!(*remove_iter)->initialized()) {
        remove_iter = new_addins.erase(remove_iter);
      }
      else {
        ++remove_iter;
      }
    }
    std::sort(new_addins.begin(), new_addins.end(), CompareSyncAddinsByName());

    auto model = Gio::ListStore<SyncService>::create();
    for(auto addin : new_addins) {
      model->append(SyncService::create(addin));
    }

    Glib::ustring current_selection;
    if(auto selected = m_sync_addin_combo->get_selected_item()) {
      current_selection = std::dynamic_pointer_cast<SyncService>(selected)->service().id();
    }

    guint select_item = GTK_INVALID_LIST_POSITION;
    if(!current_selection.empty()) {
      for(guint i = 0; i < model->get_n_items(); ++i) {
        if(model->get_item(i)->service().id() == current_selection) {
          select_item = i;
          break;
        }
      }
    }

    m_sync_addin_combo->set_model(model);
    m_sync_addin_combo->set_selected(select_item);
  }

  void PreferencesDialog::on_autosync_check_toggled()
  {
    m_autosync_spinner->set_sensitive(m_autosync_check->get_active());
    update_timeout_pref();
  }

  void PreferencesDialog::update_timeout_pref()
  {
    m_gnote.preferences().sync_autosync_timeout(
        m_autosync_check->get_active() ? static_cast<int>(m_autosync_spinner->get_value()) : -1);
  }

}

