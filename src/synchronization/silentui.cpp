/*
 * gnote
 *
 * Copyright (C) 2012-2013 Aurimas Cernius
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
#include "isyncmanager.hpp"
#include "silentui.hpp"


namespace gnote {
namespace sync {

  SyncUI::Ptr SilentUI::create(NoteManager & nm)
  {
    return SyncUI::Ptr(new SilentUI(nm));
  }


  SilentUI::SilentUI(NoteManager & manager)
    : m_manager(manager)
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


  void SilentUI::note_synchronized(const std::string & noteTitle, NoteSyncType type)
  {
    DBG_OUT("note synchronized, Title: %s, Type: %d", noteTitle.c_str(), int(type));
  }


  void SilentUI::note_conflict_detected(NoteManager & manager,
                                        const Note::Ptr & localConflictNote,
                                        NoteUpdate remoteNote,
                                        const std::list<std::string> &)
  {
    DBG_OUT("note conflict detected, overwriting without a care");
    // TODO: At least respect conflict prefs
    // TODO: Implement more useful conflict handling
    if(localConflictNote->id() != remoteNote.m_uuid) {
      manager.delete_note(localConflictNote);
    }
    ISyncManager::obj().resolve_conflict(OVERWRITE_EXISTING);
  }


  void SilentUI::present_ui()
  {
  }


  void SilentUI::on_connecting()
  {
    m_manager.read_only(true);
    std::list<Note::Ptr> notes = m_manager.get_notes();
    for(std::list<Note::Ptr>::iterator iter = notes.begin(); iter != notes.end(); ++iter) {
      (*iter)->enabled(false);
    }
  }


  void SilentUI::on_idle()
  {
    m_manager.read_only(false);
    std::list<Note::Ptr> notes = m_manager.get_notes();
    for(std::list<Note::Ptr>::iterator iter = notes.begin(); iter != notes.end(); ++iter) {
      (*iter)->enabled(true);
    }
    m_ui_disabled = false;
  }

}
}
