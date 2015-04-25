/*
 * gnote
 *
 * Copyright (C) 2010-2015 Aurimas Cernius
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

#include <boost/bind.hpp>
#include <glibmm/i18n.h>
#include <gtkmm/alignment.h>
#include <gtkmm/image.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/stock.h>

#include "debug.hpp"
#include "ignote.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "recentchanges.hpp"
#include "sharp/string.hpp"


namespace gnote {

  NoteRecentChanges::NoteRecentChanges(NoteManager& m)
    : MainWindow(_("Gnote"))
    , m_note_manager(m)
    , m_search_notes_widget(m)
    , m_search_box(0.5, 0.5, 0.0, 1.0)
    , m_mapped(false)
    , m_entry_changed_timeout(NULL)
    , m_window_menu_embedded(NULL)
    , m_window_menu_default(NULL)
    , m_keybinder(get_accel_group())
  {
    Glib::RefPtr<Gio::Settings> settings = Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE);
    m_open_notes_in_new_window = settings->get_boolean(Preferences::OPEN_NOTES_IN_NEW_WINDOW);
    m_close_note_on_escape = settings->get_boolean(Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE);
    set_default_size(450,400);
    set_resizable(true);
    if(settings->get_boolean(Preferences::MAIN_WINDOW_MAXIMIZED)) {
      maximize();
    }
    settings->signal_changed().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_settings_changed));

    set_has_resize_grip(true);

    m_search_notes_widget.signal_open_note
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note));
    m_search_notes_widget.signal_open_note_new_window
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note_new_window));
    m_search_notes_widget.notes_widget().signal_key_press_event()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_notes_widget_key_press));

    make_header_bar();
    set_titlebar(m_header_bar);
    make_search_box();
    m_content_vbox.set_orientation(Gtk::ORIENTATION_VERTICAL);
    m_content_vbox.attach(m_search_box, 0, 0, 1, 1);
    m_content_vbox.attach(m_embed_box, 0, 1, 1, 1);
    m_embed_box.set_hexpand(true);
    m_embed_box.set_vexpand(true);
    m_embed_box.show();
    m_content_vbox.show ();

    add (m_content_vbox);
    signal_delete_event().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_delete));
    signal_key_press_event()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_key_pressed));
    IGnote::obj().signal_quit
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::close_window));// to save size/pos
    m_keybinder.add_accelerator(sigc::mem_fun(*this, &NoteRecentChanges::close_window),
                                GDK_KEY_W, Gdk::CONTROL_MASK, (Gtk::AccelFlags)0);

    m_window_menu_default = make_window_menu(m_window_actions_button, std::vector<Gtk::MenuItem*>());
    embed_widget(m_search_notes_widget);
  }


  NoteRecentChanges::~NoteRecentChanges()
  {
    while(m_embedded_widgets.size()) {
      unembed_widget(**m_embedded_widgets.begin());
    }
    if(m_entry_changed_timeout) {
      delete m_entry_changed_timeout;
    }
    if(m_window_menu_embedded) {
      delete m_window_menu_embedded;
    }
    if(m_window_menu_default) {
      delete m_window_menu_default;
    }
  }

  void NoteRecentChanges::make_header_bar()
  {
    m_header_bar.set_show_close_button(true);

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
    m_all_notes_button->show_all();
    left_box->attach(*m_all_notes_button, 0, 0, 1, 1);

    m_new_note_button = manage(new Gtk::Button);
    m_new_note_button->set_label(_("New"));
    m_new_note_button->add_accelerator("activate", get_accel_group(), GDK_KEY_N, Gdk::CONTROL_MASK, (Gtk::AccelFlags) 0);
    m_new_note_button->signal_clicked().connect(sigc::mem_fun(m_search_notes_widget, &SearchNotesWidget::new_note));
    m_new_note_button->show_all();
    left_box->attach(*m_new_note_button, 1, 0, 1, 1);
    left_box->show();

    m_embedded_toolbar.set_margin_left(6);
    m_embedded_toolbar.set(Gtk::ALIGN_START, Gtk::ALIGN_CENTER, 0, 0);
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
    m_search_button.add_accelerator("activate", get_accel_group(), GDK_KEY_F, Gdk::CONTROL_MASK, (Gtk::AccelFlags) 0);
    m_search_button.set_tooltip_text(_("Search"));
    m_search_button.show_all();
    right_box->attach(m_search_button, 0, 0, 1, 1);

    m_window_actions_button = manage(new Gtk::Button);
    image = manage(new Gtk::Image);
    image->property_icon_name() = "emblem-system-symbolic";
    image->property_icon_size() = GTK_ICON_SIZE_MENU;
    m_window_actions_button->set_image(*image);
    m_window_actions_button->signal_clicked().connect(
      sigc::mem_fun(*this, &NoteRecentChanges::on_show_window_menu));
    m_window_actions_button->add_accelerator(
      "activate", get_accel_group(), GDK_KEY_F10, (Gdk::ModifierType) 0, (Gtk::AccelFlags) 0);
    m_window_actions_button->show_all();
    right_box->attach(*m_window_actions_button, 1, 0, 1, 1);
    right_box->show();

    m_header_bar.pack_start(*left_box);
    m_header_bar.pack_end(*right_box);
    m_header_bar.pack_end(m_embedded_toolbar);
    m_header_bar.show();
  }

  void NoteRecentChanges::make_search_box()
  {
    m_search_entry.set_activates_default(false);
    m_search_entry.set_size_request(300);
    m_search_entry.signal_changed()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_entry_changed));
    m_search_entry.signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_entry_activated));
    m_search_entry.show();

    Gtk::Grid *grid = manage(new Gtk::Grid);
    grid->set_margin_left(5);
    grid->set_margin_right(5);
    grid->set_hexpand(false);
    grid->attach(m_search_entry, 0, 0, 1, 1);

    m_find_next_prev_box.set_margin_left(5);

    Gtk::Button *find_next_button = manage(new Gtk::Button(_("Find _Next"), true));
    find_next_button->set_image(*manage(new Gtk::Image(Gtk::Stock::GO_FORWARD, Gtk::ICON_SIZE_MENU)));
    find_next_button->set_always_show_image(true);
    find_next_button->signal_clicked()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_find_next_button_clicked));
    find_next_button->show();
    m_find_next_prev_box.attach(*find_next_button, 0, 0, 1, 1);

    Gtk::Button *find_prev_button = manage(new Gtk::Button(_("Find _Previous"), true));
    find_prev_button->set_image(*manage(new Gtk::Image(Gtk::Stock::GO_BACK, Gtk::ICON_SIZE_MENU)));
    find_prev_button->set_always_show_image(true);
    find_prev_button->signal_clicked()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_find_prev_button_clicked));
    find_prev_button->show();
    m_find_next_prev_box.attach(*find_prev_button, 1, 0, 1, 1);

    grid->attach(m_find_next_prev_box, 1, 0, 1, 1);
    grid->show();

    m_search_box.add(*grid);
    m_search_box.set_hexpand(true);
  }

  void NoteRecentChanges::on_search_button_toggled()
  {
    if(m_search_button.get_active()) {
      show_search_bar();
    }
    else {
      m_search_box.hide();
      SearchableItem *searchable_widget = dynamic_cast<SearchableItem*>(currently_embedded());
      if(searchable_widget) {
        searchable_widget->perform_search("");
      }
    }
  }

  void NoteRecentChanges::on_find_next_button_clicked()
  {
    SearchableItem *searchable_widget = dynamic_cast<SearchableItem*>(currently_embedded());
    if(searchable_widget) {
      searchable_widget->goto_next_result();
    }
  }

  void NoteRecentChanges::on_find_prev_button_clicked()
  {
    SearchableItem *searchable_widget = dynamic_cast<SearchableItem*>(currently_embedded());
    if(searchable_widget) {
      searchable_widget->goto_previous_result();
    }
  }

  void NoteRecentChanges::show_search_bar(bool focus)
  {
    if(m_search_box.get_visible()) {
      return;
    }
    m_search_box.show();
    if(focus) {
      m_search_entry.grab_focus();
    }
    Glib::ustring text = m_search_entry.get_text();
    if(text != "") {
      SearchableItem *searchable_widget = dynamic_cast<SearchableItem*>(currently_embedded());
      if(searchable_widget) {
        searchable_widget->perform_search(text);
      }
    }
  }

  void NoteRecentChanges::present_search()
  {
    EmbeddableWidget *current = currently_embedded();
    if(&m_search_notes_widget == dynamic_cast<SearchNotesWidget*>(current)) {
      return;
    }
    if(current) {
      background_embedded(*current);
    }
    foreground_embedded(m_search_notes_widget);
  }

  void NoteRecentChanges::present_note(const Note::Ptr & note)
  {
    embed_widget(*note->get_window());
  }


  void NoteRecentChanges::new_note()
  {
    std::vector<Gtk::Widget*> current = m_embed_box.get_children();
    SearchNotesWidget *search_wgt = dynamic_cast<SearchNotesWidget*>(current.size() > 0 ? current[0] : NULL);
    if(search_wgt) {
      search_wgt->new_note();
    }
    else {
      present_note(static_pointer_cast<Note>(m_note_manager.create()));
    }
  }


  void NoteRecentChanges::on_open_note(const Note::Ptr & note)
  {
    if(m_open_notes_in_new_window) {
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
    present_in_new_window(note, m_close_note_on_escape);
  }

  void NoteRecentChanges::on_delete_note()
  {
    m_search_notes_widget.delete_selected_notes();
  }



  void NoteRecentChanges::close_window()
  {
    Glib::RefPtr<Gdk::Window> win = get_window();
    // background window (for tray to work) might not have GDK window
    if(win) {
      Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE)->set_boolean(
          Preferences::MAIN_WINDOW_MAXIMIZED,
          win->get_state() & Gdk::WINDOW_STATE_MAXIMIZED);
    }
    std::vector<Gtk::Widget*> current = m_embed_box.get_children();
    for(std::vector<Gtk::Widget*>::iterator iter = current.begin();
        iter != current.end(); ++iter) {
      EmbeddableWidget *widget = dynamic_cast<EmbeddableWidget*>(*iter);
      if(widget) {
        background_embedded(*widget);
      }
    }

    hide();
  }


  bool NoteRecentChanges::on_delete(GdkEventAny *)
  {
    close_window();
    return true;
  }

  bool NoteRecentChanges::on_key_pressed(GdkEventKey * ev)
  {
    switch (ev->keyval) {
    case GDK_KEY_Escape:
      if(m_search_button.get_active()) {
        m_search_entry.set_text("");
        m_search_button.set_active(false);
      }
      // Allow Escape to close the window
      else if(close_on_escape()) {
        close_window();
      }
      else if(m_close_note_on_escape) {
        EmbeddableWidget *current_item = currently_embedded();
        if(current_item) {
          background_embedded(*current_item);
        }
        foreground_embedded(m_search_notes_widget);
      }
      break;
    case GDK_KEY_F1:
      utils::show_help("gnote", "", get_screen()->gobj(), this);
      break;
    default:
      break;
    }
    return true;
  }


  void NoteRecentChanges::on_show()
  {
    // Select "All Notes" in the notebooks list
    m_search_notes_widget.select_all_notes_notebook();

    EmbeddableWidget *widget = NULL;
    if(m_embed_box.get_children().size() == 0 && m_embedded_widgets.size() > 0) {
      widget = *m_embedded_widgets.rbegin();
      foreground_embedded(*widget);
    }

    MainWindow::on_show();

    if(widget) {
      int x = 0, y = 0;
      widget->hint_position(x, y);
      if(x && y) {
        move(x, y);
      }
    }
  }

  void NoteRecentChanges::set_search_text(const std::string & value)
  {
    m_search_entry.set_text(value);
  }

  void NoteRecentChanges::embed_widget(EmbeddableWidget & widget)
  {
    if(std::find(m_embedded_widgets.begin(), m_embedded_widgets.end(), &widget) == m_embedded_widgets.end()) {
      widget.embed(this);
      m_embedded_widgets.push_back(&widget);
    }
    EmbeddableWidget *current = currently_embedded();
    if(current == &widget) {
      return;
    }
    if(current) {
      background_embedded(*current);
    }

   if(get_visible()) {
      foreground_embedded(widget);
    }
  }

  void NoteRecentChanges::unembed_widget(EmbeddableWidget & widget)
  {
    bool show_other = false;
    std::list<EmbeddableWidget*>::iterator iter = std::find(
      m_embedded_widgets.begin(), m_embedded_widgets.end(), &widget);
    if(iter != m_embedded_widgets.end()) {
      if(is_foreground(**iter)) {
        background_embedded(widget);
        show_other = true;
      }
      m_embedded_widgets.erase(iter);
      widget.unembed();
    }
    if(show_other) {
      if(m_embedded_widgets.size()) {
	foreground_embedded(**m_embedded_widgets.rbegin());
      }
      else if(get_visible()) {
        close_window();
      }
    }
  }

  void NoteRecentChanges::foreground_embedded(EmbeddableWidget & widget)
  {
    try {
      if(currently_embedded() == &widget) {
        return;
      }
      Gtk::Widget &wid = dynamic_cast<Gtk::Widget&>(widget);
      m_embed_box.add(wid);
      wid.show();
      widget.foreground();

      bool maximized = Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE)->get_boolean(
        Preferences::MAIN_WINDOW_MAXIMIZED);
      if(get_realized()) {
        //if window is showing, use actual state
        maximized = get_window()->get_state() & Gdk::WINDOW_STATE_MAXIMIZED;
      }
      int width = 0, height = 0;
      widget.hint_size(width, height);
      if(width && height) {
        set_default_size(width, height);
        if(!maximized && get_visible()) {
          get_window()->resize(width, height);
        }
      }
      widget.size_internals();
 
      update_toolbar(widget);
      if(&widget == &m_search_notes_widget) {
        m_header_bar.set_title(_("Gnote"));
      }
      else {
        m_header_bar.set_title(widget.get_name());
        m_current_embedded_name_slot = widget.signal_name_changed
          .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_embedded_name_changed));
      }
    }
    catch(std::bad_cast&) {
    }

    try {
      HasActions &has_actions = dynamic_cast<HasActions&>(widget);
      m_current_embedded_actions_slot = has_actions.signal_actions_changed().connect(
        boost::bind(sigc::mem_fun(*this, &NoteRecentChanges::on_main_window_actions_changed),
                    &m_window_menu_embedded));
      on_main_window_actions_changed(&m_window_menu_embedded);
    }
    catch(std::bad_cast&) {
    }
  }

  void NoteRecentChanges::background_embedded(EmbeddableWidget & widget)
  {
    try {
      if(currently_embedded() != &widget) {
        return;
      }
      Gtk::Widget &wid = dynamic_cast<Gtk::Widget&>(widget);
      widget.background();
      m_embed_box.remove(wid);

      m_current_embedded_name_slot.disconnect();
      m_current_embedded_actions_slot.disconnect();
      if(m_window_menu_embedded) {
        delete m_window_menu_embedded;
        m_window_menu_embedded = NULL;
      }
    }
    catch(std::bad_cast&) {
    }

    m_embedded_toolbar.remove();
  }

  bool NoteRecentChanges::contains(EmbeddableWidget & widget)
  {
    FOREACH(EmbeddableWidget *wgt, m_embedded_widgets) {
      if(dynamic_cast<EmbeddableWidget*>(wgt) == &widget) {
        return true;
      }
    }

    return false;
  }

  bool NoteRecentChanges::is_foreground(EmbeddableWidget & widget)
  {
    FOREACH(Gtk::Widget *wgt, m_embed_box.get_children()) {
      if(dynamic_cast<EmbeddableWidget*>(wgt) == &widget) {
        return true;
      }
    }

    return false;
  }

  EmbeddableWidget *NoteRecentChanges::currently_embedded()
  {
    std::vector<Gtk::Widget*> children = m_embed_box.get_children();
    return children.size() ? dynamic_cast<EmbeddableWidget*>(children[0]) : NULL;
  }

  bool NoteRecentChanges::on_map_event(GdkEventAny *evt)
  {
    bool res = MainWindow::on_map_event(evt);
    m_mapped = true;
    return res;
  }

  void NoteRecentChanges::on_entry_changed()
  {
    if(!m_search_box.get_visible()) {
      return;
    }
    if(m_entry_changed_timeout == NULL) {
      m_entry_changed_timeout = new utils::InterruptableTimeout();
      m_entry_changed_timeout->signal_timeout
        .connect(sigc::mem_fun(*this, &NoteRecentChanges::entry_changed_timeout));
    }

    std::string search_text = get_search_text();
    if(search_text.empty()) {
      SearchableItem *searchable_widget = dynamic_cast<SearchableItem*>(currently_embedded());
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
    if(!m_search_box.get_visible()) {
      return;
    }
    std::string search_text = get_search_text();
    if(search_text.empty()) {
      return;
    }

    SearchableItem *searchable_widget = dynamic_cast<SearchableItem*>(currently_embedded());
    if(searchable_widget) {
      searchable_widget->perform_search(search_text);
    }
  }

  std::string NoteRecentChanges::get_search_text()
  {
    std::string text = m_search_entry.get_text();
    text = sharp::string_trim(text);
    return text;
  }

  void NoteRecentChanges::update_toolbar(EmbeddableWidget & widget)
  {
    bool search = dynamic_cast<SearchNotesWidget*>(&widget) == &m_search_notes_widget;
    m_all_notes_button->set_visible(!search);
    m_new_note_button->set_visible(search);

    try {
      SearchableItem & searchable_item = dynamic_cast<SearchableItem&>(widget);
      m_search_button.show();
      if(searchable_item.supports_goto_result()) {
        m_find_next_prev_box.show();
      }
      else {
        m_find_next_prev_box.hide();
      }
      searchable_item.perform_search(m_search_button.get_active() ? m_search_entry.get_text() : "");
    }
    catch(std::bad_cast &) {
      m_search_button.set_active(false);
      m_search_button.hide();
    }

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
    HasActions *embed_with_actions = dynamic_cast<HasActions*>(currently_embedded());
    if(embed_with_actions) {
      utils::popup_menu(*m_window_menu_embedded, NULL);
    }
    else {
      utils::popup_menu(*m_window_menu_default, NULL);
    }
  }

  Gtk::Menu *NoteRecentChanges::make_window_menu(Gtk::Button *button, const std::vector<Gtk::MenuItem*> & items)
  {
    Gtk::Menu *menu = new Gtk::Menu;
    for(std::vector<Gtk::MenuItem*>::const_iterator iter = items.begin(); iter != items.end(); ++iter) {
      menu->append(**iter);
    }
    if(items.size()) {
      menu->append(*manage(new Gtk::SeparatorMenuItem));
    }
    Gtk::MenuItem *item = manage(new Gtk::MenuItem(_("_Close"), true));
    item->add_accelerator("activate", get_accel_group(), GDK_KEY_W, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    item->signal_activate().connect(sigc::mem_fun(*this, &NoteRecentChanges::close_window));
    menu->append(*item);
    menu->property_attach_widget() = button;
    menu->show_all();
    return menu;
  }

  std::vector<Gtk::MenuItem*> & NoteRecentChanges::make_menu_items(std::vector<Gtk::MenuItem*> & items,
    const std::vector<Glib::RefPtr<Gtk::Action> > & actions)
  {
    FOREACH(Glib::RefPtr<Gtk::Action> action, actions) {
      Gtk::MenuItem *item = manage(action ? action->create_menu_item() : new Gtk::SeparatorMenuItem);
      items.push_back(item);
    }
    return items;
  }

  void NoteRecentChanges::on_embedded_name_changed(const std::string & name)
  {
    m_header_bar.set_title(name);
  }

  void NoteRecentChanges::on_main_window_actions_changed(Gtk::Menu **menu)
  {
    if(*menu) {
      delete *menu;
      *menu = NULL;
    }

    HasActions *embed_with_actions = dynamic_cast<HasActions*>(currently_embedded());
    if(embed_with_actions) {
      if(!m_window_menu_embedded) {
        std::vector<Gtk::MenuItem*> items;
        m_window_menu_embedded = make_window_menu(m_window_actions_button,
          make_menu_items(items, embed_with_actions->get_widget_actions()));
      }
    }
  }

  void NoteRecentChanges::on_settings_changed(const Glib::ustring & key)
  {
    if(key == Preferences::OPEN_NOTES_IN_NEW_WINDOW) {
      m_open_notes_in_new_window = Preferences::obj().get_schema_settings(
        Preferences::SCHEMA_GNOTE)->get_boolean(Preferences::OPEN_NOTES_IN_NEW_WINDOW);
    }
    else if(key == Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE) {
      m_close_note_on_escape = Preferences::obj().get_schema_settings(
        Preferences::SCHEMA_GNOTE)->get_boolean(Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE);
    }
  }

  bool NoteRecentChanges::on_notes_widget_key_press(GdkEventKey *evt)
  {
    switch(evt->keyval) {
    case GDK_KEY_Escape:
    case GDK_KEY_Delete:
      return false;
    case GDK_KEY_BackSpace:
      if(m_search_button.get_active()) {
        Glib::ustring s = m_search_entry.get_text();
        if(s.size()) {
          m_search_entry.set_text(s.substr(0, s.size() - 1));
        }
      }
      return false;
    default:
      {
        guint32 character = gdk_keyval_to_unicode(evt->keyval);
        if(character) {  // ignore special keys
          if(!m_search_button.get_active()) {
            // first show search box, then activate button
            // because we do not want the box to get selected
            show_search_bar(false);
            m_search_button.activate();
          }
          Glib::ustring s;
          s += character;
          g_signal_emit_by_name(m_search_entry.gobj(), "insert-at-cursor", s.c_str());
          return true;
        }
        return false;
      }
    }
  }

}

