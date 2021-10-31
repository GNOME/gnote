/*
 * gnote
 *
 * Copyright (C) 2010-2021 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/i18n.h>
#include <gtkmm/alignment.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/image.h>
#include <gtkmm/separator.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/stock.h>

#include "debug.hpp"
#include "iactionmanager.hpp"
#include "iconmanager.hpp"
#include "ignote.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "recentchanges.hpp"
#include "sharp/string.hpp"

namespace gnote {

  namespace {
    const char *MAIN_MENU_PRIMARY_ICON = "open-menu-symbolic";
    const char *MAIN_MENU_SECONDARY_ICON = "view-more-symbolic";
  }

  NoteRecentChanges::NoteRecentChanges(IGnote & g, NoteManagerBase & m)
    : MainWindow(_("Gnote"))
    , m_gnote(g)
    , m_note_manager(m)
    , m_preferences(g.preferences())
    , m_search_box(nullptr)
    , m_find_next_prev_box(nullptr)
    , m_search_entry(nullptr)
    , m_embedded_widget(nullptr)
    , m_mapped(false)
    , m_entry_changed_timeout(NULL)
    , m_window_menu_embedded(NULL)
    , m_window_menu_default(NULL)
    , m_accel_group(Gtk::AccelGroup::create())
    , m_keybinder(m_accel_group)
  {
    add_accel_group(m_accel_group);
    set_default_size(450,400);
    set_resizable(true);
    if(g.preferences().main_window_maximized()) {
      maximize();
    }

    set_icon_name(IconManager::GNOTE);

    m_search_notes_widget = new SearchNotesWidget(g, m);
    m_search_notes_widget->signal_open_note
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note));
    m_search_notes_widget->signal_open_note_new_window
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note_new_window));
    m_search_notes_widget->notes_widget().signal_key_press_event()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_notes_widget_key_press));

    make_header_bar();
    auto content = manage(new Gtk::Grid);
    content->set_orientation(Gtk::ORIENTATION_VERTICAL);
    int content_y_attach = 0;
    if(use_client_side_decorations(m_preferences)) {
      set_titlebar(*static_cast<Gtk::HeaderBar*>(m_header_bar));
    }
    else {
      content->attach(*m_header_bar, 0, content_y_attach++, 1, 1);
    }
    content->attach(m_embed_box, 0, content_y_attach++, 1, 1);
    m_embed_box.set_hexpand(true);
    m_embed_box.set_vexpand(true);
    m_embed_box.show();
    content->show();

    add(*content);
    signal_delete_event().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_delete));
    signal_key_press_event()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_key_pressed));
    g.signal_quit
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::close_window));// to save size/pos
    m_keybinder.add_accelerator(sigc::mem_fun(*this, &NoteRecentChanges::close_window),
                                GDK_KEY_W, Gdk::CONTROL_MASK, (Gtk::AccelFlags)0);
    m_keybinder.add_accelerator(sigc::mem_fun(*this, &NoteRecentChanges::close_window),
                                GDK_KEY_Q, Gdk::CONTROL_MASK, (Gtk::AccelFlags)0);

    std::map<Glib::ustring, const Glib::VariantType*> actions = g.action_manager().get_main_window_actions();
    for(std::map<Glib::ustring, const Glib::VariantType*>::iterator iter = actions.begin();
        iter != actions.end(); ++iter) {
      MainWindowAction::Ptr action;
      if(iter->second == NULL) {
        add_action(action = MainWindowAction::create(iter->first));
      }
      else if(iter->second == &Glib::Variant<bool>::variant_type()) {
        add_action(action = MainWindowAction::create(iter->first, false));
      }
      else if(iter->second == &Glib::Variant<gint32>::variant_type()) {
        add_action(action = MainWindowAction::create(iter->first, 0));
      }
      else if(iter->second == &Glib::Variant<Glib::ustring>::variant_type()) {
        add_action(action = MainWindowAction::create(iter->first, Glib::ustring("")));
      }
      if(action) {
        action->is_modifying(g.action_manager().is_modifying_main_window_action(iter->first));
      }
    }
    find_action("close-window")->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_close_window));

    embed_widget(*m_search_notes_widget);
  }


  NoteRecentChanges::~NoteRecentChanges()
  {
    if(m_entry_changed_timeout) {
      delete m_entry_changed_timeout;
    }
    if(!m_search_box && m_search_text) {
      delete m_search_text;
    }
    delete m_search_notes_widget;
  }

  void NoteRecentChanges::make_header_bar()
  {
    Gtk::Grid *left_box = manage(new Gtk::Grid);
    left_box->get_style_context()->add_class(GTK_STYLE_CLASS_RAISED);
    left_box->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    left_box->set_valign(Gtk::ALIGN_CENTER);
    m_all_notes_button = manage(new Gtk::Button);
    Gtk::Image *image = manage(new Gtk::Image);
    image->property_icon_name() = "go-previous-symbolic";
    image->property_icon_size() = GTK_ICON_SIZE_MENU;
    m_all_notes_button->set_image(*image);
    m_all_notes_button->set_tooltip_text(_("All Notes"));
    m_all_notes_button->signal_clicked().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_all_notes_button_clicked));
    m_all_notes_button->add_accelerator("activate", m_accel_group, GDK_KEY_comma, Gdk::CONTROL_MASK, (Gtk::AccelFlags) 0);
    m_all_notes_button->show_all();
    left_box->attach(*m_all_notes_button, 0, 0, 1, 1);

    m_new_note_button = manage(new Gtk::Button);
    image = manage(new Gtk::Image);
    image->property_icon_name() = "list-add-symbolic";
    image->property_icon_size() = GTK_ICON_SIZE_MENU;
    m_new_note_button->set_image(*image);
    m_new_note_button->set_tooltip_text(_("Create New Note"));
    m_new_note_button->add_accelerator("activate", m_accel_group, GDK_KEY_N, Gdk::CONTROL_MASK, (Gtk::AccelFlags) 0);
    m_new_note_button->signal_clicked().connect(sigc::mem_fun(*m_search_notes_widget, &SearchNotesWidget::new_note));
    m_new_note_button->show_all();
    left_box->attach(*m_new_note_button, 1, 0, 1, 1);
    left_box->show();

    m_embedded_toolbar.set_margin_start(6);
    m_embedded_toolbar.set_halign(Gtk::ALIGN_START);
    m_embedded_toolbar.set_valign(Gtk::ALIGN_CENTER);
    m_embedded_toolbar.show();

    Gtk::Grid *right_box = manage(new Gtk::Grid);
    right_box->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    right_box->set_column_spacing(5);
    right_box->set_valign(Gtk::ALIGN_CENTER);
    image = manage(new Gtk::Image);
    image->property_icon_name() = "edit-find-symbolic";
    image->property_icon_size() = GTK_ICON_SIZE_MENU;
    m_search_button.set_image(*image);
    m_search_button.signal_toggled().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_search_button_toggled));
    m_search_button.add_accelerator("activate", m_accel_group, GDK_KEY_F, Gdk::CONTROL_MASK, (Gtk::AccelFlags) 0);
    m_search_button.set_tooltip_text(_("Search"));
    m_search_button.show_all();
    right_box->attach(m_search_button, 0, 0, 1, 1);

    m_window_actions_button = manage(new Gtk::Button);
    image = manage(new Gtk::Image);
    image->property_icon_name() = MAIN_MENU_PRIMARY_ICON;
    image->property_icon_size() = GTK_ICON_SIZE_MENU;
    m_window_actions_button->set_image(*image);
    m_window_actions_button->signal_clicked().connect(
      sigc::mem_fun(*this, &NoteRecentChanges::on_show_window_menu));
    m_window_actions_button->add_accelerator(
      "activate", m_accel_group, GDK_KEY_F10, (Gdk::ModifierType) 0, (Gtk::AccelFlags) 0);
    m_window_actions_button->show_all();
    right_box->attach(*m_window_actions_button, 1, 0, 1, 1);
    right_box->show();

    if(use_client_side_decorations(m_preferences)) {
      Gtk::HeaderBar *header_bar = manage(new Gtk::HeaderBar);
      header_bar->set_show_close_button(true);
      header_bar->pack_start(*left_box);
      header_bar->pack_end(*right_box);
      header_bar->pack_end(m_embedded_toolbar);
      m_header_bar = header_bar;
    }
    else {
      Gtk::Grid *header_bar = manage(new Gtk::Grid);
      header_bar->set_margin_start(5);
      header_bar->set_margin_end(5);
      header_bar->set_margin_top(5);
      header_bar->set_margin_bottom(5);
      header_bar->attach(*left_box, 0, 0, 1, 1);
      left_box->set_hexpand(true);
      header_bar->attach(m_embedded_toolbar, 2, 0, 1, 1);
      header_bar->attach(*right_box, 3, 0, 1, 1);
      m_header_bar = header_bar;
    }

    m_header_bar->show();
  }

  void NoteRecentChanges::make_search_box()
  {
    if(m_search_box) {
      return;
    }

    Glib::ustring search_text;
    if(m_search_text) {
      search_text = *m_search_text;
      delete m_search_text;
    }
    m_search_entry = manage(new Gtk::SearchEntry);
    m_search_entry->set_text(search_text);
    m_search_entry->set_activates_default(false);
    m_search_entry->set_size_request(300);
    m_search_entry->signal_key_press_event()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_entry_key_pressed), false);
    m_search_entry->signal_changed()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_entry_changed));
    m_search_entry->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_entry_activated));
    m_search_entry->show();

    m_search_box = manage(new Gtk::Grid);
    m_search_box->set_hexpand(false);
    m_search_box->attach(*m_search_entry, 0, 0, 1, 1);
    m_search_box->set_halign(Gtk::ALIGN_CENTER);

    auto content = dynamic_cast<Gtk::Grid*>(m_embed_box.get_parent());
    if(content) {
      content->attach_next_to(*m_search_box, m_embed_box, Gtk::POS_TOP);
    }
    else {
      ERR_OUT(_("Parent of embed box is not a Gtk::Grid, please report a bug!"));
    }
  }

  void NoteRecentChanges::make_find_next_prev()
  {
    if(m_find_next_prev_box) {
      return;
    }

    m_find_next_prev_box = manage(new Gtk::Grid);
    m_find_next_prev_box->set_margin_start(5);

    Gtk::Button *find_next_button = manage(new Gtk::Button);
    Gtk::Image *image = manage(new Gtk::Image);
    image->property_icon_name() = "go-down-symbolic";
    image->property_icon_size() = GTK_ICON_SIZE_MENU;
    find_next_button->set_image(*image);
    find_next_button->set_always_show_image(true);
    find_next_button->signal_clicked()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_find_next_button_clicked));
    find_next_button->add_accelerator("activate", m_accel_group, GDK_KEY_G, Gdk::CONTROL_MASK, (Gtk::AccelFlags) 0);
    find_next_button->show();
    m_find_next_prev_box->attach(*find_next_button, 0, 0, 1, 1);

    Gtk::Button *find_prev_button = manage(new Gtk::Button);
    image = manage(new Gtk::Image);
    image->property_icon_name() = "go-up-symbolic";
    image->property_icon_size() = GTK_ICON_SIZE_MENU;
    find_prev_button->set_image(*image);
    find_prev_button->set_always_show_image(true);
    find_prev_button->signal_clicked()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_find_prev_button_clicked));
    find_prev_button->add_accelerator("activate", m_accel_group, GDK_KEY_G, Gdk::CONTROL_MASK|Gdk::SHIFT_MASK, (Gtk::AccelFlags) 0);
    find_prev_button->show();
    m_find_next_prev_box->attach(*find_prev_button, 1, 0, 1, 1);

    auto grid = dynamic_cast<Gtk::Grid*>(m_search_entry->get_parent());
    if(grid) {
      grid->attach(*m_find_next_prev_box, 1, 0, 1, 1);
    }
    else {
      ERR_OUT(_("Parent of search entry is not Gtk::Grid, please report a bug!"));
    }
  }

  void NoteRecentChanges::on_search_button_toggled()
  {
    if(m_search_button.get_active()) {
      show_search_bar();
    }
    else {
      m_search_box->hide();
      SearchableItem *searchable_widget = dynamic_cast<SearchableItem*>(currently_foreground());
      if(searchable_widget) {
        searchable_widget->perform_search("");
      }
    }
  }

  void NoteRecentChanges::on_find_next_button_clicked()
  {
    SearchableItem *searchable_widget = dynamic_cast<SearchableItem*>(currently_foreground());
    if(searchable_widget) {
      searchable_widget->goto_next_result();
    }
  }

  void NoteRecentChanges::on_find_prev_button_clicked()
  {
    SearchableItem *searchable_widget = dynamic_cast<SearchableItem*>(currently_foreground());
    if(searchable_widget) {
      searchable_widget->goto_previous_result();
    }
  }

  void NoteRecentChanges::show_search_bar(bool focus)
  {
    make_search_box();
    if(m_search_box->get_visible()) {
      focus = false;
    }
    m_search_box->show();
    if(focus) {
      m_search_entry->grab_focus();
    }
    Glib::ustring text = m_search_entry->get_text();
    update_search_bar(*currently_foreground(), text != "");
  }

  void NoteRecentChanges::update_search_bar(EmbeddableWidget & widget, bool perform_search)
  {
    SearchableItem *searchable_item = dynamic_cast<SearchableItem*>(&widget);
    if(searchable_item) {
      m_search_button.show();
      if(searchable_item->supports_goto_result()) {
        if(m_search_box && m_search_box->get_visible()) {
          make_find_next_prev();
          m_find_next_prev_box->show();
        }
      }
      else {
        if(m_find_next_prev_box) {
          m_find_next_prev_box->hide();
        }
      }
      if(perform_search) {
        searchable_item->perform_search(m_search_button.get_active() ? m_search_entry->get_text() : "");
      }
    }
    else {
      m_search_button.set_active(false);
      m_search_button.hide();
    }
  }

  void NoteRecentChanges::present_search()
  {
    EmbeddableWidget *current = currently_foreground();
    if(m_search_notes_widget == dynamic_cast<SearchNotesWidget*>(current)) {
      return;
    }
    if(current) {
      background_embedded(*current);
    }
    foreground_embedded(*m_search_notes_widget);
  }

  void NoteRecentChanges::present_note(const Note::Ptr & note)
  {
    embed_widget(*note->create_window());
  }


  void NoteRecentChanges::new_note()
  {
    std::vector<Gtk::Widget*> current = m_embed_box.get_children();
    SearchNotesWidget *search_wgt = dynamic_cast<SearchNotesWidget*>(current.size() > 0 ? current[0] : NULL);
    if(search_wgt) {
      search_wgt->new_note();
    }
    else {
      present_note(std::static_pointer_cast<Note>(m_note_manager.create()));
    }
  }


  void NoteRecentChanges::on_open_note(const Note::Ptr & note)
  {
    if(m_preferences.open_notes_in_new_window()) {
      on_open_note_new_window(note);
    }
    else {
      if(!present_active(note)) {
        present_note(note);
      }
    }
  }

  void NoteRecentChanges::on_open_note_new_window(const Note::Ptr & note)
  {
    present_in_new_window(m_gnote, note, m_preferences.enable_close_note_on_escape());
  }

  void NoteRecentChanges::on_delete_note()
  {
    m_search_notes_widget->delete_selected_notes();
  }



  void NoteRecentChanges::close_window()
  {
    Glib::RefPtr<Gdk::Window> win = get_window();
    // background window (for tray to work) might not have GDK window
    if(win) {
      m_preferences.main_window_maximized(win->get_state() & Gdk::WINDOW_STATE_MAXIMIZED);
    }
    std::vector<Gtk::Widget*> current = m_embed_box.get_children();
    for(std::vector<Gtk::Widget*>::iterator iter = current.begin();
        iter != current.end(); ++iter) {
      EmbeddableWidget *widget = dynamic_cast<EmbeddableWidget*>(*iter);
      if(widget) {
        background_embedded(*widget);
      }
    }
    if(m_embedded_widget) {
      m_embedded_widget->unembed();
      m_embedded_widget = nullptr;
    }

    hide();
  }


  bool NoteRecentChanges::is_search()
  {
    return m_search_notes_widget == currently_foreground();
  }


  void NoteRecentChanges::on_close_window(const Glib::VariantBase&)
  {
    close_window();
  }


  bool NoteRecentChanges::on_delete(GdkEventAny *)
  {
    close_window();
    return true;
  }

  bool NoteRecentChanges::on_key_pressed(GdkEventKey *ev)
  {
    guint keyval;
    gdk_event_get_keyval((GdkEvent*)ev, &keyval);
    switch(keyval) {
    case GDK_KEY_Escape:
      if(m_search_button.get_active()) {
        m_search_entry->set_text("");
        m_search_button.set_active(false);
      }
      // Allow Escape to close the window
      else if(close_on_escape()) {
        close_window();
      }
      else if(m_preferences.enable_close_note_on_escape()) {
        EmbeddableWidget *current_item = currently_foreground();
        if(current_item) {
          background_embedded(*current_item);
        }
        foreground_embedded(*m_search_notes_widget);
      }
      break;
    case GDK_KEY_F1:
      utils::show_help("gnote", "", *this);
      break;
    default:
      break;
    }
    return true;
  }


  void NoteRecentChanges::on_show()
  {
    // Select "All Notes" in the notebooks list
    m_search_notes_widget->select_all_notes_notebook();

    EmbeddableWidget *widget = m_embedded_widget ? m_embedded_widget : m_search_notes_widget;
    foreground_embedded(*widget);

    MainWindow::on_show();

    if(widget) {
      int x = 0, y = 0;
      widget->hint_position(x, y);
      if(x && y) {
        move(x, y);
      }
    }
  }

  void NoteRecentChanges::set_search_text(const Glib::ustring & value)
  {
    if(m_search_box) {
      m_search_entry->set_text(value);
    }
    else {
      if(!m_search_text) {
        m_search_text = new Glib::ustring(value);
      }
      else {
        *m_search_text = value;
      }
    }
  }

  void NoteRecentChanges::embed_widget(EmbeddableWidget & widget)
  {
    EmbeddableWidget *current = currently_foreground();
    if(current == &widget) {
      return;
    }
    if(m_embedded_widget) {
      unembed_widget(*m_embedded_widget);
    }
    m_embedded_widget = &widget;
    widget.embed(this);
   if(get_visible()) {
      foreground_embedded(widget);
    }
  }

  void NoteRecentChanges::unembed_widget(EmbeddableWidget & widget)
  {
    bool show_search = false;
    if(&widget == m_embedded_widget) {
      if(is_foreground(widget)) {
        background_embedded(widget);
        show_search = true;
      }
      m_embedded_widget = nullptr;
      widget.unembed();
    }
    if(show_search) {
      foreground_embedded(*m_search_notes_widget);
    }
  }

  void NoteRecentChanges::foreground_embedded(EmbeddableWidget & widget)
  {
    try {
      EmbeddableWidget *current_foreground = currently_foreground();
      if(current_foreground == &widget) {
        return;
      }
      else if(current_foreground) {
        background_embedded(*current_foreground);
      }
      Gtk::Widget &wid = dynamic_cast<Gtk::Widget&>(widget);
      m_embed_box.add(wid);
      wid.show();
      widget.foreground();

      bool maximized = m_preferences.main_window_maximized();
      if(get_realized()) {
        //if window is showing, use actual state
        maximized = get_window()->get_state() & Gdk::WINDOW_STATE_MAXIMIZED;
      }
      int width = 0, height = 0;
      widget.hint_size(width, height);
      if(width && height) {
        set_default_size(width, height);
        if(!maximized && get_visible()) {
          resize(width, height);
        }
      }
      widget.size_internals();
 
      update_toolbar(widget);
      if(&widget == m_search_notes_widget) {
        set_title(_("Gnote"));
      }
      else {
        set_title(widget.get_name());
        m_current_embedded_name_slot = widget.signal_name_changed
          .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_embedded_name_changed));
      }
    }
    catch(std::bad_cast&) {
    }

    try {
      HasActions &has_actions = dynamic_cast<HasActions&>(widget);
      if(m_window_menu_embedded) {
        m_window_menu_embedded = NULL;
      }
      m_signal_popover_widgets_changed_cid = has_actions.signal_popover_widgets_changed
        .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_popover_widgets_changed));
    }
    catch(std::bad_cast&) {
    }
  }

  void NoteRecentChanges::background_embedded(EmbeddableWidget & widget)
  {
    try {
      if(currently_foreground() != &widget) {
        return;
      }
      Gtk::Widget &wid = dynamic_cast<Gtk::Widget&>(widget);
      widget.background();
      m_embed_box.remove(wid);
      m_signal_popover_widgets_changed_cid.disconnect();
      m_current_embedded_name_slot.disconnect();

      if(m_window_menu_embedded) {
        m_window_menu_embedded = NULL;
      }
    }
    catch(std::bad_cast&) {
    }

    auto children = m_embedded_toolbar.get_children();
    for(auto child : children) {
      m_embedded_toolbar.remove(*child);
    }
  }

  bool NoteRecentChanges::contains(EmbeddableWidget & widget)
  {
    if(&widget == m_search_notes_widget) {
      return true;
    }

    return &widget == m_embedded_widget;
  }

  bool NoteRecentChanges::is_foreground(EmbeddableWidget & widget)
  {
    for(Gtk::Widget *wgt : m_embed_box.get_children()) {
      if(dynamic_cast<EmbeddableWidget*>(wgt) == &widget) {
        return true;
      }
    }

    return false;
  }

  void NoteRecentChanges::add_action(const MainWindowAction::Ptr & action)
  {
    m_actions[action->get_name()] = action;
    MainWindow::add_action(action);
  }

  MainWindowAction::Ptr NoteRecentChanges::find_action(const Glib::ustring & name)
  {
    std::map<Glib::ustring, MainWindowAction::Ptr>::iterator iter = m_actions.find(name);
    if(iter != m_actions.end()) {
      return iter->second;
    }
    return MainWindowAction::Ptr();
  }

  void NoteRecentChanges::enabled(bool is_enabled)
  {
    for(auto & iter : m_actions) {
      if(iter.second->is_modifying()) {
        iter.second->set_enabled(is_enabled);
      }
    }
  }

  EmbeddableWidget *NoteRecentChanges::currently_foreground()
  {
    std::vector<Gtk::Widget*> children = m_embed_box.get_children();
    return children.size() ? dynamic_cast<EmbeddableWidget*>(children[0]) : NULL;
  }

  bool NoteRecentChanges::on_map_event(GdkEventAny *evt)
  {
    bool res = MainWindow::on_map_event(evt);
    if(!m_mapped) {
      auto widget = currently_foreground();
      if(widget) {
        widget->set_initial_focus();
      }
    }
    m_mapped = true;
    return res;
  }

  bool NoteRecentChanges::on_entry_key_pressed(GdkEventKey *ev)
  {
    guint keyval;
    gdk_event_get_keyval((GdkEvent*)ev, &keyval);
    switch(keyval) {
    case GDK_KEY_Escape:
      m_search_entry->set_text("");
      m_search_button.set_active(false);
    }

    return false;
  }

  void NoteRecentChanges::on_entry_changed()
  {
    if(!m_search_box || !m_search_box->get_visible()) {
      return;
    }
    if(m_entry_changed_timeout == NULL) {
      m_entry_changed_timeout = new utils::InterruptableTimeout();
      m_entry_changed_timeout->signal_timeout
        .connect(sigc::mem_fun(*this, &NoteRecentChanges::entry_changed_timeout));
    }

    Glib::ustring search_text = get_search_text();
    if(search_text.empty()) {
      SearchableItem *searchable_widget = dynamic_cast<SearchableItem*>(currently_foreground());
      if(searchable_widget) {
        searchable_widget->perform_search(search_text);
      }
    }
    else {
      m_entry_changed_timeout->reset(500);
    }
  }

  void NoteRecentChanges::on_entry_activated()
  {
    if(m_entry_changed_timeout) {
      m_entry_changed_timeout->cancel();
    }

    entry_changed_timeout();
  }

  void NoteRecentChanges::entry_changed_timeout()
  {
    if(!m_search_box || !m_search_box->get_visible()) {
      return;
    }
    Glib::ustring search_text = get_search_text();
    if(search_text.empty()) {
      return;
    }

    SearchableItem *searchable_widget = dynamic_cast<SearchableItem*>(currently_foreground());
    if(searchable_widget) {
      searchable_widget->perform_search(search_text);
    }
  }

  Glib::ustring NoteRecentChanges::get_search_text()
  {
    Glib::ustring text;
    if(m_search_box) {
      text = m_search_entry->get_text();
    }
    else if(m_search_text) {
      text = *m_search_text;
    }
    text = sharp::string_trim(text);
    return text;
  }

  void NoteRecentChanges::update_toolbar(EmbeddableWidget & widget)
  {
    bool search = dynamic_cast<SearchNotesWidget*>(&widget) == m_search_notes_widget;
    m_all_notes_button->set_visible(!search);
    m_new_note_button->set_visible(search);
    dynamic_cast<Gtk::Image*>(m_window_actions_button->get_image())->property_icon_name() = search ? MAIN_MENU_PRIMARY_ICON : MAIN_MENU_SECONDARY_ICON;
    update_search_bar(widget, true);

    try {
      HasEmbeddableToolbar & toolbar_provider = dynamic_cast<HasEmbeddableToolbar&>(widget);
      Gtk::Widget *tool_item = toolbar_provider.embeddable_toolbar();
      if(tool_item) {
        m_embedded_toolbar.add(*tool_item);
      }
    }
    catch(std::bad_cast &) {
    }
  }

  void NoteRecentChanges::on_all_notes_button_clicked()
  {
    close_on_escape(false);  // intentional switch to search, user probably want to work more with this window
    present_search();
  }

  void NoteRecentChanges::on_show_window_menu()
  {
    HasActions *embed_with_actions = dynamic_cast<HasActions*>(currently_foreground());
    if(embed_with_actions) {
      if(m_window_menu_embedded == NULL) {
        m_window_menu_embedded = make_window_menu(m_window_actions_button, std::move(embed_with_actions->get_popover_widgets()));
      }
      m_window_menu_embedded->show_all();
    }
    else {
      if(m_window_menu_default == NULL) {
        m_window_menu_default = make_window_menu(m_window_actions_button, std::vector<PopoverWidget>());
      }
      m_window_menu_default->show_all();
    }
  }

  Gtk::PopoverMenu *NoteRecentChanges::make_window_menu(Gtk::Button *button, std::vector<PopoverWidget> && items)
  {
    std::sort(items.begin(), items.end());
    Gtk::PopoverMenu *menu = manage(new Gtk::PopoverMenu);
    Gtk::Box *menu_box = manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    utils::set_common_popover_widget_props(*menu_box);
    if(items.size() > 0) {
      auto iter = items.begin();
      auto current_section = iter->section;
      for(; iter != items.end() && iter->section != APP_CUSTOM_SECTION; ++iter) {
          if(iter->section != current_section) {
            current_section = iter->section;
            menu_box->add(*manage(new Gtk::Separator));
          }
          menu_box->add(*manage(iter->widget));
      }

      menu->add(*menu_box);
      for(; iter != items.end(); ++iter) {
          PopoverSubmenu *submenu = dynamic_cast<PopoverSubmenu*>(iter->widget);
          if(submenu) {
            menu->add(*manage(iter->widget));
            menu->child_property_submenu(*iter->widget) = submenu->name();
          }
          else {
            ERR_OUT(_("Expected widget to be a sub-menu!"));
          }
      }
    }
    else {
      menu_box->add(*manage(new Gtk::Label(_("No configured actions"))));
      menu->add(*menu_box);
    }

    menu->set_relative_to(*button);
    menu->set_modal(true);
    menu->set_position(Gtk::POS_BOTTOM);
    return menu;
  }

  void NoteRecentChanges::on_embedded_name_changed(const Glib::ustring & name)
  {
    set_title(name);
  }

  void NoteRecentChanges::on_popover_widgets_changed()
  {
    if(m_window_menu_embedded) {
      m_window_menu_embedded = NULL;
    }
  }

  bool NoteRecentChanges::on_notes_widget_key_press(GdkEventKey *ev)
  {
    guint keyval;
    gdk_event_get_keyval((GdkEvent*)ev, &keyval);
    switch(keyval) {
    case GDK_KEY_Escape:
    case GDK_KEY_Delete:
    case GDK_KEY_Tab:
      return false;
    case GDK_KEY_BackSpace:
      if(m_search_button.get_active()) {
        Glib::ustring s = m_search_entry->get_text();
        if(s.size()) {
          m_search_entry->set_text(s.substr(0, s.size() - 1));
        }
      }
      return false;
    default:
      {
        guint32 character = gdk_keyval_to_unicode(keyval);
        if(character) {  // ignore special keys
          if(!m_search_button.get_active()) {
            // first show search box, then activate button
            // because we do not want the box to get selected
            show_search_bar(false);
            m_search_button.activate();
          }
          Glib::ustring s;
          s += character;
          g_signal_emit_by_name(m_search_entry->gobj(), "insert-at-cursor", s.c_str());
          return true;
        }
        return false;
      }
    }
  }

}

