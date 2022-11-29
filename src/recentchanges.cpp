/*
 * gnote
 *
 * Copyright (C) 2010-2022 Aurimas Cernius
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

    class TabLabel
      : public Gtk::Box
    {
    public:
      TabLabel(const Glib::ustring & label, EmbeddableWidget & widget, bool show_button)
        : m_embed(widget)
        , m_label(label)
      {
        append(m_label);
        if (show_button) {
          m_close_button.set_image_from_icon_name("window-close-symbolic");
          m_close_button.set_has_frame(false);
          m_close_button.set_focus_on_click(false);
          m_close_button.set_tooltip_text(_("Close"));
          append(m_close_button);
          m_close_button.signal_clicked().connect([this] { signal_close(m_embed); });
        }

        widget.signal_name_changed.connect(sigc::mem_fun(*this, &TabLabel::on_embedded_name_changed));
      }

      sigc::signal<void(EmbeddableWidget&)> signal_close;
    private:
      void on_embedded_name_changed(const Glib::ustring & name)
      {
        m_label.set_text(name);
      }

      EmbeddableWidget & m_embed;
      Gtk::Label m_label;
      Gtk::Button m_close_button;
    };
  }

  NoteRecentChanges::NoteRecentChanges(IGnote & g, NoteManagerBase & m)
    : MainWindow(_("Gnote"))
    , m_gnote(g)
    , m_note_manager(m)
    , m_preferences(g.preferences())
    , m_search_box(nullptr)
    , m_find_next_prev_box(nullptr)
    , m_search_entry(nullptr)
    , m_mapped(false)
    , m_entry_changed_timeout(NULL)
    , m_accel_group(Gtk::AccelGroup::create())
    , m_keybinder(m_accel_group)
  {
    add_accel_group(m_accel_group);
    set_resizable(true);
    if(g.preferences().main_window_maximized()) {
      maximize();
    }
    int width = m_gnote.preferences().search_window_width();
    int height = m_gnote.preferences().search_window_height();
    if(width && height) {
      set_default_size(width, height);
    }
    else {
      set_default_size(450,400);
    }

    set_title(_("Gnote"));
    set_icon_name(IconManager::GNOTE);
    register_actions();

    m_search_notes_widget = new SearchNotesWidget(g, m);
    m_search_notes_widget->signal_open_note
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note));
    m_search_notes_widget->signal_open_note_new_window
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note_new_window));
    m_search_notes_widget->notes_widget_key_ctrl()->signal_key_pressed()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_notes_widget_key_press), true);

    make_header_bar();
    auto content = manage(new Gtk::Grid);
    content->set_orientation(Gtk::Orientation::VERTICAL);
    int content_y_attach = 0;
    if(use_client_side_decorations(m_preferences)) {
      set_titlebar(*static_cast<Gtk::HeaderBar*>(m_header_bar));
    }
    else {
      content->attach(*m_header_bar, 0, content_y_attach++, 1, 1);
    }
    content->attach(m_embed_book, 0, content_y_attach++, 1, 1);
    m_embed_book.set_hexpand(true);
    m_embed_book.set_vexpand(true);
    m_embed_book.signal_switch_page().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_current_page_changed), false);
    embed_widget(*m_search_notes_widget);

    set_child(*content);
    signal_close_request().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_close), false);

    auto controller = Gtk::EventControllerKey::create();
    controller->signal_key_pressed().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_key_pressed), true);
    add_controller(controller);

    g.signal_quit.connect(sigc::mem_fun(*this, &NoteRecentChanges::close_window));// to save size/pos

    {
      auto shortcuts = Gtk::ShortcutController::create();
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_W, Gdk::ModifierType::CONTROL_MASK);
      auto action = Gtk::NamedAction::create("win.close-window");
      auto shortcut = Gtk::Shortcut::create(trigger, action);
      shortcuts->add_shortcut(shortcut);
      trigger = Gtk::KeyvalTrigger::create(GDK_KEY_Q, Gdk::ModifierType::CONTROL_MASK);
      shortcut = Gtk::Shortcut::create(trigger, action);
      shortcuts->add_shortcut(shortcut);
      trigger = Gtk::KeyvalTrigger::create(GDK_KEY_question, Gdk::ModifierType::CONTROL_MASK);
      action = Gtk::NamedAction::create("app.help-shortcuts");
      shortcut = Gtk::Shortcut::create(trigger, action);
      shortcuts->add_shortcut(shortcut);
      add_controller(shortcuts);
    }
  }


  NoteRecentChanges::~NoteRecentChanges()
  {
    int page_idx = m_embed_book.get_current_page();
    if(page_idx >= 0) {
      // to avoid page switching, first remove all pages prior to current, then remove from the end
      for(int i = 0; i < page_idx; ++i) {
        m_embed_book.remove_page(0);
      }
      while(m_embed_book.get_n_pages() > 0) {
        m_embed_book.remove_page(-1);
      }
    }
    if(m_entry_changed_timeout) {
      delete m_entry_changed_timeout;
    }
    if(!m_search_box && m_search_text) {
      delete m_search_text;
    }
    delete m_search_notes_widget;
  }

  void NoteRecentChanges::register_actions()
  {
    auto & am = m_gnote.action_manager();
    for(auto & action_def : am.get_main_window_actions()) {
      MainWindowAction::Ptr action;
      if(action_def.second == NULL) {
        add_action(action = MainWindowAction::create(Glib::ustring(action_def.first)));
      }
      else if(action_def.second == &Glib::Variant<bool>::variant_type()) {
        add_action(action = MainWindowAction::create(Glib::ustring(action_def.first), false));
      }
      else if(action_def.second == &Glib::Variant<gint32>::variant_type()) {
        add_action(action = MainWindowAction::create(Glib::ustring(action_def.first), 0));
      }
      else if(action_def.second == &Glib::Variant<Glib::ustring>::variant_type()) {
        add_action(action = MainWindowAction::create(Glib::ustring(action_def.first), Glib::ustring("")));
      }
      if(action) {
        action->is_modifying(am.is_modifying_main_window_action(action_def.first));
      }
    }
    find_action("close-window")->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_close_window));
    am.signal_main_window_search_actions_changed
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::callbacks_changed));

    register_callbacks();
  }

  void NoteRecentChanges::callbacks_changed()
  {
    unregister_callbacks();
    register_callbacks();
  }

  void NoteRecentChanges::register_callbacks()
  {
    auto & manager(m_gnote.action_manager());
    auto cbacks = manager.get_main_window_search_callbacks();
    for(auto & cback : cbacks) {
      auto action = find_action(cback.first);
      if(action) {
        m_action_cids.push_back(action->signal_activate().connect(cback.second));
      }
    }
  }

  void NoteRecentChanges::unregister_callbacks()
  {
    for(auto & cid : m_action_cids) {
      cid.disconnect();
    }
    m_action_cids.clear();
  }

  void NoteRecentChanges::add_shortcut(Gtk::Widget & widget, guint keyval, Gdk::ModifierType modifiers)
  {
    auto controller = Gtk::ShortcutController::create();
    controller->set_scope(Gtk::ShortcutScope::GLOBAL);
    widget.add_controller(controller);
    add_shortcut(*controller, keyval, modifiers);
  }

  void NoteRecentChanges::add_shortcut(Gtk::ShortcutController & controller, guint keyval, Gdk::ModifierType modifiers)
  {
    auto trigger = Gtk::KeyvalTrigger::create(keyval, modifiers);
    auto shortcut = Gtk::Shortcut::create(trigger, Gtk::ActivateAction::get());
    controller.add_shortcut(shortcut);
  }

  void NoteRecentChanges::make_header_bar()
  {
    Gtk::Grid *left_box = manage(new Gtk::Grid);
    left_box->set_orientation(Gtk::Orientation::HORIZONTAL);
    left_box->set_valign(Gtk::Align::CENTER);

    auto new_note_button = manage(new Gtk::Button);
    new_note_button->set_image_from_icon_name("list-add-symbolic");
    new_note_button->set_tooltip_text(_("Create New Note"));
    new_note_button->signal_clicked().connect(sigc::mem_fun(*m_search_notes_widget, &SearchNotesWidget::new_note));
    add_shortcut(*new_note_button, GDK_KEY_N, Gdk::ModifierType::CONTROL_MASK|Gdk::ModifierType::SHIFT_MASK);
    left_box->attach(*new_note_button, 0, 0, 1, 1);

    m_embedded_toolbar.set_margin_start(6);
    m_embedded_toolbar.set_halign(Gtk::Align::START);
    m_embedded_toolbar.set_valign(Gtk::Align::CENTER);

    Gtk::Grid *right_box = manage(new Gtk::Grid);
    right_box->set_orientation(Gtk::Orientation::HORIZONTAL);
    right_box->set_column_spacing(5);
    right_box->set_valign(Gtk::Align::CENTER);
    int right_box_pos = 0;

    m_current_embed_actions_button = manage(new Gtk::Button);
    m_current_embed_actions_button->set_image_from_icon_name(MAIN_MENU_SECONDARY_ICON);
    m_current_embed_actions_button->signal_clicked().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_show_embed_action_menu));
    add_shortcut(*m_current_embed_actions_button, GDK_KEY_F8);
    right_box->attach(*m_current_embed_actions_button, right_box_pos++, 0, 1, 1);

    m_search_button.set_image_from_icon_name("edit-find-symbolic");
    m_search_button.signal_toggled().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_search_button_toggled));
    add_shortcut(m_search_button, GDK_KEY_F, Gdk::ModifierType::CONTROL_MASK);
    m_search_button.set_tooltip_text(_("Search"));
    Gtk::Grid *search_group = manage(new Gtk::Grid);
    search_group->set_column_spacing(5);
    search_group->set_orientation(Gtk::Orientation::HORIZONTAL);
    search_group->set_valign(Gtk::Align::CENTER);
    search_group->attach(m_search_button, 0, 0, 1, 1);
    right_box->attach(*search_group, right_box_pos++, 0, 1, 1);

    m_window_actions_button = manage(new Gtk::Button);
    m_window_actions_button->set_image_from_icon_name(MAIN_MENU_PRIMARY_ICON);
    m_window_actions_button->signal_clicked().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_show_window_menu));
    {
      auto shortcuts = Gtk::ShortcutController::create();
      shortcuts->set_scope(Gtk::ShortcutScope::GLOBAL);
      add_shortcut(*m_window_actions_button, GDK_KEY_F10);
      add_shortcut(*m_window_actions_button, GDK_KEY_comma, Gdk::ModifierType::CONTROL_MASK );
    }
    right_box->attach(*m_window_actions_button, right_box_pos++, 0, 1, 1);

    if(use_client_side_decorations(m_preferences)) {
      Gtk::HeaderBar *header_bar = manage(new Gtk::HeaderBar);
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
    auto key_ctrl = Gtk::EventControllerKey::create();
    key_ctrl->signal_key_pressed().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_entry_key_pressed), false);
    m_search_entry->add_controller(key_ctrl);
    m_search_entry->signal_changed()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_entry_changed));
    m_search_entry->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_entry_activated));

    m_search_box = manage(new Gtk::Grid);
    m_search_box->set_hexpand(false);
    m_search_box->attach(*m_search_entry, 0, 0, 1, 1);
    m_search_box->set_halign(Gtk::Align::CENTER);

    auto content = dynamic_cast<Gtk::Grid*>(m_search_button.get_parent());
    if(content) {
      content->attach_next_to(*m_search_box, m_search_button, Gtk::PositionType::LEFT);
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
    find_next_button->property_icon_name() = "go-down-symbolic";
    find_next_button->signal_clicked()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_find_next_button_clicked));
    find_next_button->add_accelerator("activate", m_accel_group, GDK_KEY_G, Gdk::CONTROL_MASK, (Gtk::AccelFlags) 0);
    m_find_next_prev_box->attach(*find_next_button, 0, 0, 1, 1);

    Gtk::Button *find_prev_button = manage(new Gtk::Button);
    find_prev_button->property_icon_name() = "go-up-symbolic";
    find_prev_button->signal_clicked()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_find_prev_button_clicked));
    find_prev_button->add_accelerator("activate", m_accel_group, GDK_KEY_G, Gdk::CONTROL_MASK|Gdk::SHIFT_MASK, (Gtk::AccelFlags) 0);
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
    if(note && note->has_window()) {
      auto win = note->get_window();
      if(win->host()) {
        win->host()->unembed_widget(*win);
      }
      embed_widget(*win);
      win->set_initial_focus();
    }
    else {
      auto win = note->create_window();
      embed_widget(*win);
      win->set_initial_focus();
    }
  }


  void NoteRecentChanges::new_note()
  {
    auto current = m_embed_book.get_nth_page(0);
    SearchNotesWidget *search_wgt = dynamic_cast<SearchNotesWidget*>(current);
    if(search_wgt) {
      search_wgt->new_note();
    }
    else {
      present_note(std::static_pointer_cast<Note>(m_note_manager.create()));
    }
  }


  void NoteRecentChanges::on_open_note(const Note::Ptr & note)
  {
    if(!present_active(note)) {
      present_note(note);
    }
  }

  void NoteRecentChanges::on_open_note_new_window(const Note::Ptr & note)
  {
    present_in_new_window(m_gnote, note);
  }

  void NoteRecentChanges::on_delete_note()
  {
    m_search_notes_widget->delete_selected_notes();
  }

  bool NoteRecentChanges::present_active(const Note::Ptr & note)
  {
    if(note && note->has_window()) {
      auto note_window = note->get_window();
      auto win = dynamic_cast<NoteRecentChanges*>(note_window->host());
      if(!win) {
        return false;
      }
      if(win == this) {
        int n_pages = m_embed_book.get_n_pages();
        for(int i = 0; i < n_pages; ++i) {
          if(m_embed_book.get_nth_page(i) == note_window) {
            m_embed_book.set_current_page(i);
            break;
          }
        }
      }
      else {
        win->present_active(note);
        win->present();
      }

      return true;
    }

    return false;
  }

  void NoteRecentChanges::close_window()
  {
    m_preferences.main_window_maximized(is_maximized());

    if(m_embed_book.get_n_pages() > 0) {
      EmbeddableWidget *widget = dynamic_cast<EmbeddableWidget*>(m_embed_book.get_nth_page(m_embed_book.get_current_page()));
      if(widget) {
        background_embedded(*widget);
      }

      int n_pages = m_embed_book.get_n_pages();
      for(int i = 0; i < n_pages; ++i) {
        EmbeddableWidget *widget = dynamic_cast<EmbeddableWidget*>(m_embed_book.get_nth_page(i));
        if(widget) {
          widget->unembed();
        }
      }
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


  bool NoteRecentChanges::on_close()
  {
    close_window();
    return false;
  }

  bool NoteRecentChanges::on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state)
  {
    switch(keyval) {
    case GDK_KEY_Escape:
      if(m_search_button.get_active()) {
        m_search_entry->set_text("");
        m_search_button.set_active(false);
      }
      return true;
    case GDK_KEY_F1:
      utils::show_help("gnote", "", *this);
      return true;
    default:
      break;
    }
    return false;
  }

  void NoteRecentChanges::next_tab()
  {
    int next_page = 1 + m_embed_book.get_current_page();
    if(next_page >= m_embed_book.get_n_pages()) {
      next_page = 0;
    }

    m_embed_book.set_current_page(next_page);
  }

  void NoteRecentChanges::previous_tab()
  {
    int prev_page = m_embed_book.get_current_page() - 1;
    if(prev_page < 0) {
      prev_page = m_embed_book.get_n_pages() - 1;
    }

    m_embed_book.set_current_page(prev_page);
  }

  void NoteRecentChanges::on_show()
  {
    // Select "All Notes" in the notebooks list
    m_search_notes_widget->select_all_notes_notebook();
    MainWindow::on_show();
  }

  void NoteRecentChanges::set_search_text(Glib::ustring && value)
  {
    if(m_search_box) {
      m_search_entry->set_text(std::move(value));
    }
    else {
      if(!m_search_text) {
        m_search_text = new Glib::ustring(std::move(value));
      }
      else {
        *m_search_text = std::move(value);
      }
    }
  }

  void NoteRecentChanges::embed_widget(EmbeddableWidget & widget)
  {
    auto win = dynamic_cast<Gtk::Widget*>(&widget);
    if(!win) {
      return;
    }
    win->show();
    widget.embed(this);
    bool allow_close = &widget != m_search_notes_widget;
    auto tab_label = manage(new TabLabel(widget.get_name(), widget, allow_close));
    if (allow_close) {
      tab_label->signal_close.connect(sigc::mem_fun(*this, &NoteRecentChanges::unembed_widget));
    }
    int idx = m_embed_book.append_page(*win, *tab_label);
    m_embed_book.set_current_page(idx);
  }

  void NoteRecentChanges::unembed_widget(EmbeddableWidget & widget)
  {
    int n_pages = m_embed_book.get_n_pages();
    int page_to_remove = -1;
    Gtk::Widget *wid = dynamic_cast<Gtk::Widget*>(&widget);
    for(int i = 0; i < n_pages; ++i) {
      if(m_embed_book.get_nth_page(i) == wid) {
        page_to_remove = i;
        break;
      }
    }
    if(page_to_remove < 0) {
      return;
    }
    if(n_pages > 1 && page_to_remove == m_embed_book.get_current_page()) {
      int new_page = (page_to_remove == n_pages - 1) ? page_to_remove - 1 : page_to_remove + 1;
      m_embed_book.set_current_page(new_page);
    }

    m_embed_book.remove_page(page_to_remove);
    widget.unembed();
  }

  void NoteRecentChanges::foreground_embedded(EmbeddableWidget & widget)
  {
    try {
      int idx = m_embed_book.page_num(dynamic_cast<Gtk::Widget&>(widget));
      if(idx < 0) {
        ERR_OUT(_("Attempt to foreground a widget, that is not embedded in the window"));
        return;
      }

      m_embed_book.set_current_page(idx);
    }
    catch(std::bad_cast&) {
      ERR_OUT(_("Attempt to foreground a widget, that is not Gtk::Widget"));
    }
  }

  void NoteRecentChanges::on_foreground_embedded(EmbeddableWidget & widget)
  {
    try {
      widget.foreground();
      widget.size_internals();
      update_toolbar(widget);
    }
    catch(std::bad_cast&) {
    }
  }

  void NoteRecentChanges::background_embedded(EmbeddableWidget & widget)
  {
    widget.background();

    auto child = m_embedded_toolbar.get_child_at(0, 0);
    if(child) {
      m_embedded_toolbar.remove(*child);
    }
  }

  void NoteRecentChanges::on_current_page_changed(Gtk::Widget *new_page, guint)
  {
    int idx = m_embed_book.get_current_page();
    if(idx >= 0) {
      EmbeddableWidget *w = dynamic_cast<EmbeddableWidget*>(m_embed_book.get_nth_page(idx));
      if(w) {
        background_embedded(*w);
      }
    }

    auto widget = dynamic_cast<EmbeddableWidget*>(new_page);
    if(widget) {
      on_foreground_embedded(*widget);
    }
  }

  bool NoteRecentChanges::contains(EmbeddableWidget & widget)
  {
    try {
      return m_embed_book.page_num(dynamic_cast<Gtk::Widget&>(widget)) >= 0;
    }
    catch(std::bad_cast &) {
      return false;
    }
  }

  bool NoteRecentChanges::is_foreground(EmbeddableWidget & widget)
  {
    int idx = m_embed_book.get_current_page();
    if(idx < 0) {
      return false;
    }

    return m_embed_book.get_nth_page(idx) == dynamic_cast<Gtk::Widget*>(&widget);
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
    auto n = m_embed_book.get_current_page();
    if(n < 0) {
      return nullptr;
    }

    return dynamic_cast<EmbeddableWidget*>(m_embed_book.get_nth_page(n));
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

  bool NoteRecentChanges::on_entry_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state)
  {
    switch(keyval) {
    case GDK_KEY_Escape:
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
    update_search_bar(widget, true);
    auto has_actions = dynamic_cast<HasActions*>(&widget);
    if(has_actions) {
      m_current_embed_actions_button->show();
    }
    else {
      m_current_embed_actions_button->hide();
    }

    try {
      HasEmbeddableToolbar & toolbar_provider = dynamic_cast<HasEmbeddableToolbar&>(widget);
      Gtk::Widget *tool_item = toolbar_provider.embeddable_toolbar();
      if(tool_item) {
        m_embedded_toolbar.attach(*tool_item, 0, 0);
      }
    }
    catch(std::bad_cast &) {
    }
  }

  void NoteRecentChanges::on_show_window_menu()
  {
    std::vector<PopoverWidget> popover_widgets;
    popover_widgets.reserve(20);
    m_gnote.action_manager().signal_build_main_window_search_popover(popover_widgets);
    for(unsigned i = 0; i < popover_widgets.size(); ++i) {
      popover_widgets[i].secondary_order = i;
    }

    auto menu = make_window_menu(m_window_actions_button, std::move(popover_widgets));
    menu->popup();
  }

  void NoteRecentChanges::on_show_embed_action_menu()
  {
    HasActions *embed_with_actions = dynamic_cast<HasActions*>(currently_foreground());
    if(embed_with_actions) {
      auto menu = make_window_menu(m_current_embed_actions_button, std::move(embed_with_actions->get_popover_widgets()));
      menu->popup();
    }
  }

  Gtk::Popover *NoteRecentChanges::make_window_menu(Gtk::Button *button, std::vector<PopoverWidget> && items)
  {
    std::sort(items.begin(), items.end());
    Gtk::Popover *menu = manage(new Gtk::Popover);
    Gtk::Box *menu_box = manage(new Gtk::Box(Gtk::Orientation::VERTICAL));
    utils::set_common_popover_widget_props(*menu_box);
    if(items.size() > 0) {
      auto iter = items.begin();
      auto current_section = iter->section;
      for(; iter != items.end() && iter->section != APP_CUSTOM_SECTION; ++iter) {
          if(iter->section != current_section) {
            current_section = iter->section;
            menu_box->append(*manage(new Gtk::Separator));
          }
          menu_box->append(*manage(iter->widget));
          if(auto submenu_button = dynamic_cast<PopoverButton*>(iter->widget)) {
            submenu_button->parent_popover(menu);
          }
      }
      menu->set_child(*menu_box);
    }
    else {
      menu_box->append(*manage(new Gtk::Label(_("No configured actions"))));
      menu->set_child(*menu_box);
    }

    menu->set_position(Gtk::PositionType::BOTTOM);
    menu->signal_closed().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_window_menu_closed));
    menu->set_parent(*button);
    return menu;
  }

  void NoteRecentChanges::on_window_menu_closed()
  {
    auto idx = m_embed_book.get_current_page();
    if(idx < 0) {
      return;
    }
    auto current_page = dynamic_cast<EmbeddableWidget*>(m_embed_book.get_nth_page(idx));
    if(current_page == nullptr) {
      return;
    }

    current_page->set_initial_focus();
  }

  bool NoteRecentChanges::on_notes_widget_key_press(guint keyval, guint keycode, Gdk::ModifierType state)
  {
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
        if((state & Gdk::ModifierType::CONTROL_MASK) == Gdk::ModifierType::CONTROL_MASK
           || (state & Gdk::ModifierType::ALT_MASK) == Gdk::ModifierType::ALT_MASK) {
          return false;
        }
        guint32 character = gdk_keyval_to_unicode(keyval);
        if(character) {  // ignore special keys
          if(!m_search_button.get_active()) {
            // first show search box, then activate button
            // because we do not want the box to get selected
            show_search_bar(false);
            m_search_button.activate();
          }
          Glib::ustring s = m_search_entry->get_text();
          s += character;
          m_search_entry->set_text(s);
          return true;
        }
        return false;
      }
    }
  }

}

