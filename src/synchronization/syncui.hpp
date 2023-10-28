/*
 * gnote
 *
 * Copyright (C) 2012-2014,2017,2019,2021-2023 Aurimas Cernius
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


#include <glibmm/ustring.h>

#include "syncutils.hpp"


namespace gnote {

  class IGnote;

namespace sync {

  class SyncUI
    : public std::enable_shared_from_this<SyncUI>
  {
  public:
    typedef std::shared_ptr<SyncUI> Ptr;
    typedef sigc::slot<void()> SlotConnecting;
    typedef sigc::slot<void()> SlotIdle;

    virtual ~SyncUI() {}
    virtual void sync_state_changed(SyncState state) = 0;
    void note_synchronized_th(const Glib::ustring & noteTitle, NoteSyncType type);
    virtual void note_synchronized(const Glib::ustring & noteTitle, NoteSyncType type) = 0;
    virtual void note_conflict_detected(NoteBase & localConflictNote,
                                        NoteUpdate remoteNote,
                                        const std::vector<Glib::ustring> & noteUpdateTitles) = 0;
    virtual void present_ui() = 0;

    sigc::connection signal_connecting_connect(const SlotConnecting & slot);
    void signal_connecting_emit();
    sigc::connection signal_idle_connect(const SlotIdle & slot);
    void signal_idle_emit();
  protected:
    SyncUI(IGnote & g, NoteManagerBase & manager);

    IGnote & m_gnote;
    NoteManagerBase & m_manager;
  private:
    void signal_connecting_emit_()
      {
        m_signal_connecting.emit();
      }
    void signal_idle_emit_()
      {
        m_signal_idle.emit();
      }

    sigc::signal<void()> m_signal_connecting;
    sigc::signal<void()> m_signal_idle;
  };

}
}


#endif
