/*
 * gnote
 *
 * Copyright (C) 2012-2014 Aurimas Cernius
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


#ifndef _SYNCHRONIZATION_SILENTUI_HPP_
#define _SYNCHRONIZATION_SILENTUI_HPP_


#include "base/macros.hpp"
#include "notemanager.hpp"
#include "syncui.hpp"



namespace gnote {
namespace sync {

  class SilentUI
    : public SyncUI
  {
  public:
    static SyncUI::Ptr create(NoteManagerBase &);
  private:
    explicit SilentUI(NoteManagerBase &);
    virtual void sync_state_changed(SyncState state) override;
    virtual void note_synchronized(const std::string & noteTitle, NoteSyncType type) override;
    virtual void note_conflict_detected(const Note::Ptr & localConflictNote,
                                        NoteUpdate remoteNote,
                                        const std::list<std::string> & noteUpdateTitles) override;
    virtual void present_ui() override;
    void on_connecting();
    void on_idle();

    bool m_ui_disabled;
  };

}
}

#endif
