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


#ifndef _SYNCHRONIZATION_SYNCUI_HPP_
#define _SYNCHRONIZATION_SYNCUI_HPP_


#include <list>
#include <string>
#include <tr1/memory>

#include "syncutils.hpp"


namespace gnote {
namespace sync {

  class SyncUI
    : public std::tr1::enable_shared_from_this<SyncUI>
  {
  public:
    typedef std::tr1::shared_ptr<SyncUI> Ptr;
    typedef sigc::slot<void> SlotConnecting;
    typedef sigc::slot<void> SlotIdle;

    virtual void sync_state_changed(SyncState state) = 0;
    void note_synchronized_th(const std::string & noteTitle, NoteSyncType type);
    virtual void note_synchronized(const std::string & noteTitle, NoteSyncType type) = 0;
    virtual void note_conflict_detected(NoteManager & manager,
                                        const Note::Ptr & localConflictNote,
                                        NoteUpdate remoteNote,
                                        const std::list<std::string> & noteUpdateTitles) = 0;
    virtual void present_ui() = 0;

    sigc::connection signal_connecting_connect(const SlotConnecting & slot);
    void signal_connecting_emit();
    sigc::connection signal_idle_connect(const SlotIdle & slot);
    void signal_idle_emit();
  private:
    void signal_connecting_emit_()
      {
        m_signal_connecting.emit();
      }
    void signal_idle_emit_()
      {
        m_signal_idle.emit();
      }

    sigc::signal<void> m_signal_connecting;
    sigc::signal<void> m_signal_idle;
  };

}
}


#endif
