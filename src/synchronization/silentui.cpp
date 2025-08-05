/*
 * gnote
 *
 * Copyright (C) 2012-2014,2017,2019,2023,2025 Aurimas Cernius
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
#include "ignote.hpp"
#include "isyncmanager.hpp"
#include "silentui.hpp"


namespace gnote {
namespace sync {

  SyncUI::Ptr SilentUI::create(IGnote & g, NoteManagerBase & nm)
  {
    return std::make_shared<SilentUI>(g, nm);
  }


  SilentUI::SilentUI(IGnote & g, NoteManagerBase & manager)
    : SyncUI(g, manager)
    , m_ui_disabled(false)
  {
    signal_connecting_connect(sigc::mem_fun(*this, &SilentUI::on_connecting));
    signal_idle_connect(sigc::mem_fun(*this, &SilentUI::on_idle));
  }


  void SilentUI::sync_state_changed(SyncState state)
  {
    // TODO: Update tray/applet icon
    //       D-Bus event?
    //       libnotify bubbles when appropriate
    DBG_OUT("SilentUI: SyncStateChanged: %d", int(state));
    switch(state) {
    case CONNECTING:
      m_ui_disabled = true;
      // TODO: Disable all kinds of note editing
      //         -New notes from server should be disabled, too
      //         -Anyway we could skip this when uploading changes?
      //         -Should store original Enabled state
      signal_connecting_emit();
      break;
    case IDLE:
      if(m_ui_disabled) {
        signal_idle_emit();
      }
      break;
    default:
      break;
    }
  }


  void SilentUI::note_synchronized(const Glib::ustring & DBG(noteTitle), NoteSyncType DBG(type))
  {
    DBG_OUT("note synchronized, Title: %s, Type: %d", noteTitle.c_str(), int(type));
  }


  void SilentUI::note_conflict_detected(NoteBase & localConflictNote,
                                        NoteUpdate remoteNote,
                                        const std::vector<Glib::ustring> &)
  {
    DBG_OUT("note conflict detected, overwriting without a care");
    // TODO: At least respect conflict prefs
    // TODO: Implement more useful conflict handling
    if(localConflictNote.id() != remoteNote.m_uuid) {
      m_manager.delete_note(localConflictNote);
    }
  }


  void SilentUI::present_ui()
  {
  }


  void SilentUI::on_connecting()
  {
    m_manager.read_only(true);
    m_manager.for_each([](NoteBase & note) {
      note.enabled(false);
    });
  }


  void SilentUI::on_idle()
  {
    m_manager.read_only(false);
    m_manager.for_each([](NoteBase & note) {
      note.enabled(true);
    });
    m_ui_disabled = false;
  }

}
}
