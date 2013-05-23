/*
 * gnote
 *
 * Copyright (C) 2010-2013 Aurimas Cernius
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
#include "iactionmanager.hpp"
#include "ignote.hpp"
#include "iconmanager.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "recentchanges.hpp"
#include "sharp/string.hpp"


namespace gnote {

  NoteRecentChanges::NoteRecentChanges(NoteManager& m)
    : MainWindow(_("Notes"))
    , m_note_manager(m)
    , m_search_notes_widget(m)
    , m_search_box(0.5, 0.5, 0.0, 1.0)
    , m_mapped(false)
    , m_entry_changed_timeout(NULL)
    , m_window_menu_search(NULL)
    , m_window_menu_note(NULL)
    , m_window_menu_default(NULL)
    , m_keybinder(get_accel_group())
  {
    set_default_size(450,400);
    set_resizable(true);
    set_hide_titlebar_when_maximized(true);
    if(Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE)->get_boolean(
         Preferences::MAIN_WINDOW_MAXIMIZED)) {
      maximize();
    }

    set_has_resize_grip(true);

    m_search_notes_widget.signal_open_note
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note));
    m_search_notes_widget.signal_open_note_new_window
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note_new_window));

    Gtk::Toolbar *toolbar = make_toolbar();
    make_search_box();
    m_content_vbox.set_orientation(Gtk::ORIENTATION_VERTICAL);
    m_content_vbox.attach(*toolbar, 0, 0, 1, 1);
    m_content_vbox.attach(m_search_box, 0, 1, 1, 1);
    m_content_vbox.attach(m_embed_box, 0, 2, 1, 1);
    m_embed_box.set_hexpand(true);
    m_embed_box.set_vexpand(true);
    m_embed_box.show();
    m_content_vbox.show ();

    add (m_content_vbox);
    signal_delete_event().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_delete));
    signal_key_press_event()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_key_pressed));
    IGnote::obj().signal_quit
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_close_window));// to save size/pos
    m_keybinder.add_accelerator(sigc::mem_fun(*this, &NoteRecentChanges::on_close_window),
                                GDK_KEY_W, Gdk::CONTROL_MASK, (Gtk::AccelFlags)0);

    embed_widget(m_search_notes_widget);

    IActionManager::obj().signal_main_window_search_actions_changed
      .connect(boost::bind(sigc::mem_fun(*this, &NoteRecentChanges::on_main_window_actions_changed),
                           &m_window_menu_search));
    IActionManager::obj().signal_main_window_note_actions_changed
      .connect(boost::bind(sigc::mem_fun(*this, &NoteRecentChanges::on_main_window_actions_changed),
                           &m_window_menu_note));
  }


  NoteRecentChanges::~NoteRecentChanges()
  {
    while(m_embedded_widgets.size()) {
      unembed_widget(**m_embedded_widgets.begin());
    }
    if(m_entry_changed_timeout) {
      delete m_entry_changed_timeout;
    }
    if(m_window_menu_search) {
      delete m_window_menu_search;
    }
    if(m_window_menu_note) {
      delete m_window_menu_note;
    }
    if(m_window_menu_default) {
      delete m_window_menu_default;
    }
  }

  Gtk::Toolbar *NoteRecentChanges::make_toolbar()
  {
    gint icon_size = 16;
    gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &icon_size, NULL);
    Gtk::Toolbar *toolbar = manage(new Gtk::Toolbar);
    toolbar->set_hexpand(true);
    toolbar->set_vexpand(false);
    // let move window by dragging toolbar with mouse
    toolbar->get_style_context()->add_class(GTK_STYLE_CLASS_MENUBAR);
    Gtk::ToolItem *tool_item = manage(new Gtk::ToolItem);
    Gtk::Grid *box = manage(new Gtk::Grid);
    box->set_orientation(Gtk::ORIENTATION_HORIZONTAL);

    GtkIconSize toolbar_icon_size = gtk_toolbar_get_icon_size(toolbar->gobj());
    gint toolbar_size_px;
    gtk_icon_size_lookup(toolbar_icon_size, &toolbar_size_px, NULL);
    gint icon_margin = (gint) floor((toolbar_size_px - icon_size) / 2.0);

    Gtk::Grid *left_box = manage(new Gtk::Grid);
    left_box->get_style_context()->add_class(GTK_STYLE_CLASS_RAISED);
    left_box->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    m_all_notes_button = manage(new Gtk::Button);
    Gtk::Image *image = manage(new Gtk::Image(IconManager::obj().get_icon("go-previous-symbolic", icon_size)));
    image->property_margin() = icon_margin;
    m_all_notes_button->set_image(*image);
    m_all_notes_button->set_tooltip_text(_("All Notes"));
    m_all_notes_button->signal_clicked().connect(sigc::mem_fun(*this, &NoteRecentChanges::present_search));
    m_all_notes_button->show_all();
    left_box->attach(*m_all_notes_button, 0, 0, 1, 1);

    m_new_note_button = manage(new Gtk::Button);
    m_new_note_button->set_vexpand(true);
    m_new_note_button->set_label(_("New"));
    m_new_note_button->add_accelerator("activate", get_accel_group(), GDK_KEY_N, Gdk::CONTROL_MASK, (Gtk::AccelFlags) 0);
    m_new_note_button->signal_clicked().connect(sigc::mem_fun(m_search_notes_widget, &SearchNotesWidget::new_note));
    m_new_note_button->show_all();
    left_box->attach(*m_new_note_button, 1, 0, 1, 1);
    left_box->show();
    box->attach(*left_box, 0, 0, 1, 1);

    m_embedded_toolbar.set_hexpand(true);
    m_embedded_toolbar.set(0.5, 0.5, 0.0, 0.0);
    m_embedded_toolbar.show();
    box->attach(m_embedded_toolbar, 1, 0, 1, 1);

    Gtk::Grid *right_box = manage(new Gtk::Grid);
    right_box->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    right_box->set_column_spacing(5);
    image = manage(new Gtk::Image(IconManager::obj().get_icon("edit-find-symbolic", icon_size)));
    image->property_margin() = icon_margin;
    m_search_button.set_image(*image);
    m_search_button.signal_toggled().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_search_button_toggled));
    m_search_button.add_accelerator("activate", get_accel_group(), GDK_KEY_F, Gdk::CONTROL_MASK, (Gtk::AccelFlags) 0);
    m_search_button.set_tooltip_text(_("Search"));
    m_search_button.show_all();
    right_box->attach(m_search_button, 0, 0, 1, 1);

    Gtk::Button *button = manage(new Gtk::Button);
    image = manage(new Gtk::Image(IconManager::obj().get_icon("emblem-system-symbolic", icon_size)));
    image->property_margin() = icon_margin;
    button->set_image(*image);
    button->signal_clicked().connect(
      boost::bind(sigc::mem_fun(*this, &NoteRecentChanges::on_show_window_menu), button));
    button->add_accelerator("activate", get_accel_group(), GDK_KEY_F10, (Gdk::ModifierType) 0, (Gtk::AccelFlags) 0);
    button->show_all();
    right_box->attach(*button, 1, 0, 1, 1);
    right_box->show();
    box->attach(*right_box, 2, 0, 1, 1);

    box->show();
    tool_item->add(*box);
    tool_item->set_expand(true);
    tool_item->show();
    toolbar->add(*tool_item);
    toolbar->show();
    return toolbar;
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

  void NoteRecentChanges::show_search_bar()
  {
    m_search_box.show();
    m_search_entry.grab_focus();
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
      present_note(m_note_manager.create());
    }
  }


  void NoteRecentChanges::on_open_note(const Note::Ptr & note)
  {
    present_note(note);
  }

  void NoteRecentChanges::on_open_note_new_window(const Note::Ptr & note)
  {
    MainWindow & window = IGnote::obj().new_main_window();
    window.present();
    window.present_note(note);
  }

  void NoteRecentChanges::on_delete_note()
  {
    m_search_notes_widget.delete_selected_notes();
  }



  void NoteRecentChanges::on_close_window()
  {
    Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE)->set_boolean(
        Preferences::MAIN_WINDOW_MAXIMIZED,
        get_window()->get_state() & Gdk::WINDOW_STATE_MAXIMIZED);
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
    on_close_window();
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
      else if(&m_search_notes_widget == dynamic_cast<SearchNotesWidget*>(currently_embedded())) {
        on_close_window();
      }
      else {
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
        on_close_window();
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
      on_embedded_name_changed(widget.get_name());
      m_current_embedded_name_slot = widget.signal_name_changed
        .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_embedded_name_changed));
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
      m_current_embedded_name_slot.disconnect();
      Gtk::Widget &wid = dynamic_cast<Gtk::Widget&>(widget);
      widget.background();
      m_embed_box.remove(wid);
    }
    catch(std::bad_cast&) {
    }

    m_embedded_toolbar.remove();
  }

  bool NoteRecentChanges::is_foreground(EmbeddableWidget & widget)
  {
    std::vector<Gtk::Widget*> current = m_embed_box.get_children();
    for(std::vector<Gtk::Widget*>::iterator iter = current.begin();
        iter != current.end(); ++iter) {
      if(dynamic_cast<EmbeddableWidget*>(*iter) == &widget) {
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

  void NoteRecentChanges::on_embedded_name_changed(const std::string & name)
  {
    std::string title;
    if(name != "") {
      title = "[" + name + "] - ";
    }
    title += _("Notes");
    set_title(title);
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
      if(m_search_button.get_active() && m_search_entry.get_text() != "") {
        searchable_item.perform_search(m_search_entry.get_text());
      }
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

  void NoteRecentChanges::on_show_window_menu(Gtk::Button *button)
  {
    std::vector<Gtk::MenuItem*> items;
    if(dynamic_cast<SearchNotesWidget*>(currently_embedded()) == &m_search_notes_widget) {
      if(!m_window_menu_search) {
        m_window_menu_search = make_window_menu(button,
            make_menu_items(items, IActionManager::obj().get_main_window_search_actions()));
      }
      utils::popup_menu(*m_window_menu_search, NULL);
    }
    else if(dynamic_cast<NoteWindow*>(currently_embedded())) {
      if(!m_window_menu_note) {
        m_window_menu_note = make_window_menu(button,
            make_menu_items(items, IActionManager::obj().get_main_window_note_actions()));
      }
      utils::popup_menu(*m_window_menu_note, NULL);
    }
    else {
      if(!m_window_menu_default) {
        m_window_menu_default = make_window_menu(button, items);
      }
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
    item->signal_activate().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_close_window));
    menu->append(*item);
    menu->property_attach_widget() = button;
    menu->show_all();
    return menu;
  }

  std::vector<Gtk::MenuItem*> & NoteRecentChanges::make_menu_items(std::vector<Gtk::MenuItem*> & items,
    const std::vector<Glib::RefPtr<Gtk::Action> > & actions)
  {
    for(std::vector<Glib::RefPtr<Gtk::Action> >::const_iterator iter = actions.begin(); iter != actions.end(); ++iter) {
      Gtk::MenuItem *item = manage(new Gtk::MenuItem);
      item->set_related_action(*iter);
      items.push_back(item);
    }
    return items;
  }

  void NoteRecentChanges::on_main_window_actions_changed(Gtk::Menu **menu)
  {
    if(*menu) {
      delete *menu;
      *menu = NULL;
    }
  }

}

