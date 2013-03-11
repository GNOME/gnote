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
    , m_content_vbox(false, 0)
    , m_mapped(false)
    , m_entry_changed_timeout(NULL)
    , m_window_menu_search(NULL)
    , m_window_menu_note(NULL)
    , m_window_menu_default(NULL)
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

    Gtk::Box *toolbar = make_toolbar();
    m_content_vbox.pack_start(*toolbar, false, false, 0);
    m_content_vbox.pack_start(m_embed_box, true, true, 0);
    m_embed_box.show();
    m_content_vbox.show ();

    add (m_content_vbox);
    signal_delete_event().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_delete));
    signal_key_press_event()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_key_pressed)); // For Escape
    IGnote::obj().signal_quit
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_close_window));// to save size/pos

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

  Gtk::Box *NoteRecentChanges::make_toolbar()
  {
    Gtk::Box *toolbar = manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    toolbar->set_border_width(5);
    m_all_notes_button = manage(new Gtk::Button);
    m_all_notes_button->set_image(*manage(new Gtk::Image(Gtk::Stock::FIND, Gtk::ICON_SIZE_BUTTON)));
    m_all_notes_button->set_always_show_image(true);
    m_all_notes_button->set_label(_("All Notes"));
    m_all_notes_button->signal_clicked().connect(sigc::mem_fun(*this, &NoteRecentChanges::present_search));
    m_all_notes_button->show_all();
    toolbar->pack_start(*m_all_notes_button, false, false);

    Gtk::Button *button = manage(new Gtk::Button);
    button->set_image(*manage(new Gtk::Image(IconManager::obj().get_icon(IconManager::NOTE_NEW, 24))));
    button->set_always_show_image(true);
    button->set_label(_("New"));
    button->signal_clicked().connect(sigc::mem_fun(m_search_notes_widget, &SearchNotesWidget::new_note));
    button->show_all();
    toolbar->pack_start(*button, false, false);

    m_search_entry.set_activates_default(false);
    m_search_entry.set_size_request(300);
    m_search_entry.signal_changed()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_entry_changed));
    m_search_entry.signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_entry_activated));
    Gtk::Alignment *alignment = manage(new Gtk::Alignment(Gtk::ALIGN_CENTER, Gtk::ALIGN_CENTER, 0));
    alignment->add(m_search_entry);
    alignment->show_all();
    toolbar->pack_start(*alignment, true, true);

    button = manage(new Gtk::Button);
    button->set_image(*manage(new Gtk::Image(IconManager::obj().get_icon("emblem-system-symbolic", 24))));
    button->set_always_show_image(true);
    button->signal_clicked().connect(
      boost::bind(sigc::mem_fun(*this, &NoteRecentChanges::on_show_window_menu), button));
    button->show_all();
    toolbar->pack_end(*button, false, false);

    toolbar->show();
    return toolbar;
  }

  void NoteRecentChanges::present_search()
  {
    utils::EmbeddableWidget *current = currently_embedded();
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
      utils::EmbeddableWidget *widget = dynamic_cast<utils::EmbeddableWidget*>(*iter);
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
      // Allow Escape to close the window
      if(&m_search_notes_widget == dynamic_cast<SearchNotesWidget*>(currently_embedded())) {
        on_close_window();
      }
      else {
        utils::EmbeddableWidget *current_item = currently_embedded();
        if(current_item) {
          background_embedded(*current_item);
        }
        foreground_embedded(m_search_notes_widget);
      }
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

    if(m_embed_box.get_children().size() == 0 && m_embedded_widgets.size() > 0) {
      foreground_embedded(**m_embedded_widgets.rbegin());
    }
    std::vector<Gtk::Widget*> embedded = m_embed_box.get_children();
    if(embedded.size() == 1 && embedded.front() == &m_search_notes_widget) {
      m_search_entry.grab_focus();
    }
    MainWindow::on_show();
  }

  void NoteRecentChanges::set_search_text(const std::string & value)
  {
    m_search_entry.set_text(value);
  }

  void NoteRecentChanges::embed_widget(utils::EmbeddableWidget & widget)
  {
    if(std::find(m_embedded_widgets.begin(), m_embedded_widgets.end(), &widget) == m_embedded_widgets.end()) {
      widget.embed(this);
      m_embedded_widgets.push_back(&widget);
    }
    utils::EmbeddableWidget *current = currently_embedded();
    if(current && current != &widget) {
      background_embedded(*current);
    }
    foreground_embedded(widget);
  }

  void NoteRecentChanges::unembed_widget(utils::EmbeddableWidget & widget)
  {
    bool show_other = false;
    std::list<utils::EmbeddableWidget*>::iterator iter = std::find(
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

  void NoteRecentChanges::foreground_embedded(utils::EmbeddableWidget & widget)
  {
    try {
      if(currently_embedded() == &widget) {
        return;
      }
      Gtk::Widget &wid = dynamic_cast<Gtk::Widget&>(widget);
      m_embed_box.pack_start(wid, true, true, 0);
      widget.foreground();
      wid.show();
      update_toolbar(widget);
      on_embedded_name_changed(widget.get_name());
      m_current_embedded_name_slot = widget.signal_name_changed
        .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_embedded_name_changed));
    }
    catch(std::bad_cast&) {
    }
  }

  void NoteRecentChanges::background_embedded(utils::EmbeddableWidget & widget)
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
  }

  bool NoteRecentChanges::is_foreground(utils::EmbeddableWidget & widget)
  {
    std::vector<Gtk::Widget*> current = m_embed_box.get_children();
    for(std::vector<Gtk::Widget*>::iterator iter = current.begin();
        iter != current.end(); ++iter) {
      if(dynamic_cast<utils::EmbeddableWidget*>(*iter) == &widget) {
        return true;
      }
    }

    return false;
  }

  utils::EmbeddableWidget *NoteRecentChanges::currently_embedded()
  {
    std::vector<Gtk::Widget*> children = m_embed_box.get_children();
    return children.size() ? dynamic_cast<utils::EmbeddableWidget*>(children[0]) : NULL;
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
    if(m_entry_changed_timeout == NULL) {
      m_entry_changed_timeout = new utils::InterruptableTimeout();
      m_entry_changed_timeout->signal_timeout
        .connect(sigc::mem_fun(*this, &NoteRecentChanges::entry_changed_timeout));
    }

    std::string search_text = get_search_text();
    if(search_text.empty()) {
      m_search_notes_widget.perform_search(search_text);
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
    std::string search_text = get_search_text();
    if(search_text.empty()) {
      return;
    }

    m_search_notes_widget.perform_search(search_text);
  }

  std::string NoteRecentChanges::get_search_text()
  {
    std::string text = m_search_entry.get_text();
    text = sharp::string_trim(text);
    return text;
  }

  void NoteRecentChanges::update_toolbar(utils::EmbeddableWidget & widget)
  {
    bool search = dynamic_cast<SearchNotesWidget*>(&widget) == &m_search_notes_widget;
    m_all_notes_button->set_sensitive(!search);
    m_search_entry.set_visible(search);
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

