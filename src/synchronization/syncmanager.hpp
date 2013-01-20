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


#ifndef _SYNCHRONIZATION_SYNCMANAGER_HPP_
#define _SYNCHRONIZATION_SYNCMANAGER_HPP_


#include <map>

#include <glibmm/main.h>
#include <glibmm/thread.h>

#include "isyncmanager.hpp"


namespace gnote {
namespace sync {

  class SyncServiceAddin;

  class SyncManager
    : public ISyncManager
  {
  public:
    SyncManager(NoteManager &);
    static void init(NoteManager &);
    virtual void reset_client();
    virtual void perform_synchronization(const std::tr1::shared_ptr<SyncUI> & sync_ui);
    void synchronization_thread();
    virtual void resolve_conflict(SyncTitleConflictResolution resolution);
    virtual bool synchronized_note_xml_matches(const std::string & noteXml1, const std::string & noteXml2);
    virtual SyncState state() const
      {
        return m_state;
      }
  private:
    static SyncManager & _obj()
      {
        return static_cast<SyncManager&>(obj());
      }
    void _init(NoteManager &);
    void handle_note_saved_or_deleted(const Note::Ptr & note);
    void handle_note_buffer_changed(const Note::Ptr & note);
    void preferences_setting_changed(const Glib::ustring & key);
    void update_sync_action();
    void background_sync_checker();
    void set_state(SyncState new_state);
    SyncServiceAddin *get_configured_sync_service();
    SyncServiceAddin *get_sync_service_addin(const std::string & sync_service_id);
    void create_note_in_main_thread(const NoteUpdate & noteUpdate);
    void update_note_in_main_thread(const Note::Ptr & existingNote, const NoteUpdate & noteUpdate);
    void delete_note_in_main_thread(const Note::Ptr & existingNote);
    void update_local_note(const Note::Ptr & localNote, const NoteUpdate & serverNote, NoteSyncType syncType);
    Note::Ptr find_note_by_uuid(const std::string & uuid);
    NoteManager & note_mgr();
    void get_synchronized_xml_bits(const std::string & noteXml, std::string & title, std::string & tags, std::string & content);
    void delete_notes(const SyncServer::Ptr & server);
    void create_note(const NoteUpdate & noteUpdate);
    void update_note(const Note::Ptr & existingNote, const NoteUpdate & noteUpdate);
    void delete_note(const Note::Ptr & existingNote);
    static void note_save(const Note::Ptr & note);

    NoteManager & m_note_manager;
    SyncUI::Ptr m_sync_ui;
    SyncClient::Ptr m_client;
    SyncState m_state;
    Glib::Thread *m_sync_thread;
    SyncTitleConflictResolution m_conflict_resolution;
    utils::InterruptableTimeout m_autosync_timer;
    int m_autosync_timeout_pref_minutes;
    int m_current_autosync_timeout_minutes;
    sharp::DateTime m_last_background_check;
  };


}
}

#endif
