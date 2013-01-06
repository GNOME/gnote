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


#include <exception>
#include <map>
#include <string>

#include <glibmm/main.h>
#include <glibmm/thread.h>

#include "note.hpp"
#include "syncdialog.hpp"
#include "base/singleton.hpp"
#include "sharp/datetime.hpp"
#include "sharp/timespan.hpp"


namespace gnote {
namespace sync {

  class SyncServiceAddin;
  class SyncUI;

  class SyncClient
  {
  public:
    typedef std::tr1::shared_ptr<SyncClient> Ptr;

    virtual int last_synchronized_revision() = 0;
    virtual void last_synchronized_revision(int) = 0;
    virtual sharp::DateTime last_sync_date() = 0;
    virtual void last_sync_date(const sharp::DateTime &) = 0;
    virtual int get_revision(const Note::Ptr & note) = 0;
    virtual void set_revision(const Note::Ptr & note, int revision) = 0;
    virtual std::map<std::string, std::string> deleted_note_titles() = 0;
    virtual void reset() = 0;
    virtual std::string associated_server_id() = 0;
    virtual void associated_server_id(const std::string &) = 0;
  };

  class SyncManager
    : public base::Singleton<SyncManager>
  {
  public:
    SyncManager();
    ~SyncManager();
    static void init(NoteManager &);
    void reset_client();
    void perform_synchronization(const std::tr1::shared_ptr<SyncUI> & sync_ui);
    void synchronization_thread();
    void resolve_conflict(SyncTitleConflictResolution resolution);
    bool synchronized_note_xml_matches(const std::string & noteXml1, const std::string & noteXml2);
    SyncState state() const
      {
        return m_state;
      }
  private:
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
    static void on_delete_notes(GObject*, gpointer, gpointer);
    static void on_create_note(GObject*, gpointer, gpointer);
    static void on_update_note(GObject*, gpointer, gpointer, gpointer);
    static void on_delete_note(GObject*, gpointer, gpointer);
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
    GObject *m_sync_helper;
  };


  class SyncLockInfo
  {
  public:
    std::string client_id;
    std::string transaction_id;
    int renew_count;
    sharp::TimeSpan duration;
    int revision;

    SyncLockInfo();
    std::string hash_string();
  };


  class SyncServer
  {
  public:
    typedef std::tr1::shared_ptr<SyncServer> Ptr;

    virtual ~SyncServer();

    virtual bool begin_sync_transaction() = 0;
    virtual bool commit_sync_transaction() = 0;
    virtual bool cancel_sync_transaction() = 0;
    virtual std::list<std::string> get_all_note_uuids() = 0;
    virtual std::map<std::string, NoteUpdate> get_note_updates_since(int revision) = 0;
    virtual void delete_notes(const std::list<std::string> & deletedNoteUUIDs) = 0;
    virtual void upload_notes(const std::list<Note::Ptr> & notes) = 0;
    virtual int latest_revision() = 0; // NOTE: Only reliable during a transaction
    virtual SyncLockInfo current_sync_lock() = 0;
    virtual std::string id() = 0;
    virtual bool updates_available_since(int revision) = 0;
  };


  class GnoteSyncException
    : public std::runtime_error
  {
  public:
    GnoteSyncException(const char * what_arg) : std::runtime_error(what_arg){}
  };

}
}

#endif
