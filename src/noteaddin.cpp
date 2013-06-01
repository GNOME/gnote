/*
 * gnote
 *
 * Copyright (C) 2012-2013 Aurimas Cernius
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




#include "noteaddin.hpp"
#include "notewindow.hpp"


namespace gnote {

  const char * NoteAddin::IFACE_NAME = "gnote::NoteAddin";

  void NoteAddin::initialize(const Note::Ptr & note)
  {
    m_note = note;
    m_note_opened_cid = m_note->signal_opened().connect(
      sigc::mem_fun(*this, &NoteAddin::on_note_opened_event));
    initialize();
    if(m_note->is_opened()) {
      on_note_opened();
    }
  }


  void NoteAddin::dispose(bool disposing)
  {
    if (disposing) {
      for(std::list<std::string>::const_iterator iter = m_note_actions.begin();
          iter != m_note_actions.end(); ++iter) {
        get_window()->remove_widget_action(*iter);
      }
      for(std::list<Gtk::MenuItem*>::const_iterator iter = m_text_menu_items.begin();
          iter != m_text_menu_items.end(); ++iter) {
        delete *iter;
      }
        
      for(ToolItemMap::const_iterator iter = m_toolbar_items.begin();
               iter != m_toolbar_items.end(); ++iter) {
        delete iter->first;
      }

      shutdown ();
    }
    
    m_note_opened_cid.disconnect();
    m_note = Note::Ptr();
  }

  void NoteAddin::on_note_opened_event(Note & )
  {
    on_note_opened();
    NoteWindow * window = get_window();

    for(std::list<Gtk::MenuItem*>::const_iterator iter = m_text_menu_items.begin();
        iter != m_text_menu_items.end(); ++iter) {
      Gtk::Widget *item = *iter;
      if ((item->get_parent() == NULL) ||
          (item->get_parent() != window->text_menu())) {
        window->text_menu()->add (*item);
        window->text_menu()->reorder_child(*(Gtk::MenuItem*)item, 7);
      }
    }
      
    for(ToolItemMap::const_iterator iter = m_toolbar_items.begin();
        iter != m_toolbar_items.end(); ++iter) {
      if ((iter->first->get_parent() == NULL) ||
          (iter->first->get_parent() != window->embeddable_toolbar())) {
        Gtk::Grid *grid = window->embeddable_toolbar();
        int col = grid->get_children().size();
        grid->attach(*(iter->first), col, 0, 1, 1);
      }
    }
  }

  void NoteAddin::add_note_action(const Glib::RefPtr<Gtk::Action> & action, int order)
  {
    if(is_disposing()) {
      throw sharp::Exception("Plugin is disposing already");
    }

    m_note_actions.push_back(action->get_name());
    get_window()->add_widget_action(action, order);
  }

  void NoteAddin::add_tool_item (Gtk::ToolItem *item, int position)
  {
    if (is_disposing())
      throw sharp::Exception ("Add-in is disposing already");
        
    m_toolbar_items [item] = position;
      
    if (m_note->is_opened()) {
      Gtk::Grid *grid = get_window()->embeddable_toolbar();
      grid->attach(*item, grid->get_children().size(), 0, 1, 1);
    }
  }

  void NoteAddin::add_text_menu_item (Gtk::MenuItem * item)
  {
    if (is_disposing())
      throw sharp::Exception ("Plugin is disposing already");

    m_text_menu_items.push_back(item);

    if (m_note->is_opened()) {
      get_window()->text_menu()->add (*item);
      get_window()->text_menu()->reorder_child (*item, 7);
    }
  }

  Gtk::Window *NoteAddin::get_host_window() const
  {
    if(is_disposing() && !has_buffer()) {
      throw sharp::Exception("Plugin is disposing already");
    }
    NoteWindow *note_window = m_note->get_window();
    if(!note_window->host()) {
      throw std::runtime_error("Window is not hosted!");
    }
    return dynamic_cast<Gtk::Window*>(note_window->host());
  }
  
}
