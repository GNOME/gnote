/*
 * gnote
 *
 * Copyright (C) 2012-2016 Aurimas Cernius
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



#include <glibmm/i18n.h>

#include "debug.hpp"
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
      for (auto & iter : m_text_menu_items) {
        delete iter;
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

    window->signal_foregrounded.connect(sigc::mem_fun(*this, &NoteAddin::on_note_foregrounded));
    window->signal_backgrounded.connect(sigc::mem_fun(*this, &NoteAddin::on_note_backgrounded));

    for(auto & item : m_text_menu_items) {
      if ((item->get_parent() == NULL) ||
          (item->get_parent() != window->text_menu())) {
        append_text_item(window->text_menu(), *item);
      }
    }
      
    for(ToolItemMap::const_iterator iter = m_toolbar_items.begin();
        iter != m_toolbar_items.end(); ++iter) {
      if ((iter->first->get_parent() == NULL) ||
          (iter->first->get_parent() != window->embeddable_toolbar())) {
        Gtk::Grid *grid = window->embeddable_toolbar();
        grid->insert_column(iter->second);
        grid->attach(*iter->first, iter->second, 0, 1, 1);
      }
    }
  }

  void NoteAddin::append_text_item(Gtk::Widget *text_menu, Gtk::Widget & item)
  {
    NoteTextMenu *txt_menu = dynamic_cast<NoteTextMenu*>(text_menu);
    for(auto child : dynamic_cast<Gtk::Container*>(txt_menu->get_children().front())->get_children()) {
      if(child->get_name() == "formatting") {
        Gtk::Box *box = dynamic_cast<Gtk::Box*>(child);
        box->add(item);
      }
    }
  }

  void NoteAddin::on_note_foregrounded()
  {
    auto host = get_window()->host();
    if(!host) {
      return;
    }

    for(auto & callback : m_action_callbacks) {
      auto action = host->find_action(callback.first);
      if(action) {
        m_action_callbacks_cids.push_back(action->signal_activate().connect(callback.second));
      }
      else {
        ERR_OUT("Action %s not found!", callback.first.c_str());
      }
    }
  }

  void NoteAddin::on_note_backgrounded()
  {
    for(auto cid : m_action_callbacks_cids) {
      cid.disconnect();
    }
    m_action_callbacks_cids.clear();
  }

  void NoteAddin::add_tool_item (Gtk::ToolItem *item, int position)
  {
    if (is_disposing())
      throw sharp::Exception(_("Plugin is disposing already"));
        
    m_toolbar_items [item] = position;
      
    if (m_note->is_opened()) {
      Gtk::Grid *grid = get_window()->embeddable_toolbar();
      grid->insert_column(position);
      grid->attach(*item, position, 0, 1, 1);
    }
  }

  void NoteAddin::add_text_menu_item(Gtk::Widget *item)
  {
    if (is_disposing())
      throw sharp::Exception(_("Plugin is disposing already"));

    m_text_menu_items.push_back(item);

    if (m_note->is_opened()) {
      append_text_item(get_window()->text_menu(), *item);
    }
  }

  Gtk::Window *NoteAddin::get_host_window() const
  {
    if(is_disposing() && !has_buffer()) {
      throw sharp::Exception(_("Plugin is disposing already"));
    }
    NoteWindow *note_window = m_note->get_window();
    if(note_window == NULL || !note_window->host()) {
      throw std::runtime_error(_("Window is not embedded"));
    }
    return dynamic_cast<Gtk::Window*>(note_window->host());
  }

  std::map<int, Gtk::Widget*> NoteAddin::get_actions_popover_widgets() const
  {
    return std::map<int, Gtk::Widget*>();
  }

  void NoteAddin::register_main_window_action_callback(const Glib::ustring & action, sigc::slot<void, const Glib::VariantBase&> callback)
  {
    m_action_callbacks.emplace_back(action, callback);
  }
  
}
