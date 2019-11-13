/*
 * gnote
 *
 * Copyright (C) 2012-2014,2017,2019 Aurimas Cernius
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


#include "debug.hpp"
#include "syncui.hpp"


namespace gnote {
namespace sync {

  SyncUI::SyncUI(IGnote & g, NoteManagerBase & manager)
    : m_gnote(g)
    , m_manager(manager)
  {
  }

  void SyncUI::note_synchronized_th(const Glib::ustring & noteTitle, NoteSyncType type)
  {
    utils::main_context_invoke([this, noteTitle, type]() { note_synchronized(noteTitle, type); });
  }


  sigc::connection SyncUI::signal_connecting_connect(const SlotConnecting & slot)
  {
    return m_signal_connecting.connect(slot);
  }


  void SyncUI::signal_connecting_emit()
  {
    utils::main_context_invoke(sigc::mem_fun(*this, &SyncUI::signal_connecting_emit_));
  }


  sigc::connection SyncUI::signal_idle_connect(const SlotIdle & slot)
  {
    return m_signal_idle.connect(slot);
  }


  void SyncUI::signal_idle_emit()
  {
    utils::main_context_invoke(sigc::mem_fun(*this, &SyncUI::signal_idle_emit_));
  }

}
}
