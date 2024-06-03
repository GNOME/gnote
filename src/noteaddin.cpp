/*
 * gnote
 *
 * Copyright (C) 2012-2016,2019,2022-2024 Aurimas Cernius
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

  void NoteAddin::initialize(IGnote & ignote, Note::Ptr && note)
  {
    AbstractAddin::initialize(ignote);
    m_note = std::move(note);
    m_note_opened_cid = m_note->signal_opened().connect(
      sigc::mem_fun(*this, &NoteAddin::on_note_opened_event));
    initialize();
    if(m_note->is_opened()) {
      NoteWindow * window = get_window();

      on_note_opened();
      /* Connect these two signals here, because signal_opened won't emit for
       * opening already opened notes. */
      window->signal_foregrounded.connect(sigc::mem_fun(*this, &NoteAddin::on_foregrounded));
      window->signal_backgrounded.connect(sigc::mem_fun(*this, &NoteAddin::on_backgrounded));

      // if already foreground, trigger that too
      if(window->host()->is_foreground(*window)) {
        on_foregrounded();
      }
    }
  }


  void NoteAddin::dispose(bool disposing)
  {
    if (disposing) {
      shutdown ();
    }
    
    m_note_opened_cid.disconnect();
    m_note = Note::Ptr();
  }

  void NoteAddin::on_note_opened_event(Note & )
  {
    on_note_opened();
    NoteWindow * window = get_window();

    window->signal_foregrounded.connect(sigc::mem_fun(*this, &NoteAddin::on_foregrounded));
    window->signal_backgrounded.connect(sigc::mem_fun(*this, &NoteAddin::on_backgrounded));
  }

  void NoteAddin::on_foregrounded()
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

    on_note_foregrounded();
  }

  void NoteAddin::on_backgrounded()
  {
    for(auto cid : m_action_callbacks_cids) {
      cid.disconnect();
    }
    m_action_callbacks_cids.clear();
    on_note_backgrounded();
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

  std::vector<gnote::PopoverWidget> NoteAddin::get_actions_popover_widgets() const
  {
    return std::vector<gnote::PopoverWidget>();
  }

  void NoteAddin::register_main_window_action_callback(const Glib::ustring & action, sigc::slot<void(const Glib::VariantBase&)> && callback)
  {
    m_action_callbacks.emplace_back(action, std::move(callback));
  }
  
}
