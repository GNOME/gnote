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


#include "config.h"

#include <boost/format.hpp>
#include <glibmm/i18n.h>
#include <gtkmm/actiongroup.h>
#include <sigc++/sigc++.h>

#include "iactionmanager.hpp"
#include "addinmanager.hpp"
#include "debug.hpp"
#include "filesystemsyncserver.hpp"
#include "ignote.hpp"
#include "gnotesyncclient.hpp"
#include "notemanager.hpp"
#include "preferences.hpp"
#include "silentui.hpp"
#include "syncmanager.hpp"
#include "syncserviceaddin.hpp"
#include "sharp/uuid.hpp"
#include "sharp/xmlreader.hpp"


namespace gnote {
namespace sync {

  namespace {

    typedef GObject SyncHelper;
    typedef GObjectClass SyncHelperClass;

    G_DEFINE_TYPE(SyncHelper, sync_helper, G_TYPE_OBJECT)

    void sync_helper_init(SyncHelper*)
    {}

    void sync_helper_class_init(SyncHelperClass * klass)
    {
      g_signal_new("delete-notes", G_TYPE_FROM_CLASS(klass),
                   GSignalFlags(G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
                   0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
                   G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
      g_signal_new("create-note", G_TYPE_FROM_CLASS(klass),
                   GSignalFlags(G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
                   0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
                   G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
      g_signal_new("update-note", G_TYPE_FROM_CLASS(klass),
                   GSignalFlags(G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
                   0, NULL, NULL, g_cclosure_marshal_generic,
                   G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER, NULL);
      g_signal_new("delete-note", G_TYPE_FROM_CLASS(klass),
                   GSignalFlags(G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS),
                   0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
                   G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
    }

    GObject * sync_helper_new()
    {
      g_type_init();
      return G_OBJECT(g_object_new(sync_helper_get_type(), NULL));
    }

    void sync_helper_delete_notes(GObject * helper, gpointer server)
    {
      gdk_threads_enter();
      g_signal_emit_by_name(helper, "delete-notes", server);
      gdk_threads_leave();
    }

    void sync_helper_create_note(GObject * helper, gpointer note_update)
    {
      gdk_threads_enter();
      g_signal_emit_by_name(helper, "create-note", note_update);
      gdk_threads_leave();
    }

    void sync_helper_update_note(GObject * helper, gpointer existing_note, gpointer note_update)
    {
      gdk_threads_enter();
      g_signal_emit_by_name(helper, "update-note", existing_note, note_update);
      gdk_threads_leave();
    }

    void sync_helper_delete_note(GObject * helper, gpointer existing_note)
    {
      gdk_threads_enter();
      g_signal_emit_by_name(helper, "delete-note", existing_note);
      gdk_threads_leave();
    }

  }


  SyncManager::SyncManager(NoteManager & m)
    : m_note_manager(m)
  {
  }


  SyncManager::~SyncManager()
  {
    g_object_unref(m_sync_helper);
  }


  void SyncManager::init(NoteManager & m)
  {
    new SyncManager(m);
    SyncManager::obj()._init(m);
  }


  void SyncManager::_init(NoteManager & manager)
  {
    m_sync_helper = sync_helper_new();
    g_signal_connect(m_sync_helper, "delete-notes", G_CALLBACK(SyncManager::on_delete_notes), NULL);
    g_signal_connect(m_sync_helper, "create-note", G_CALLBACK(SyncManager::on_create_note), NULL);
    g_signal_connect(m_sync_helper, "update-note", G_CALLBACK(SyncManager::on_update_note), NULL);
    g_signal_connect(m_sync_helper, "delete-note", G_CALLBACK(SyncManager::on_delete_note), NULL);
    m_client = SyncClient::Ptr(new GnoteSyncClient(manager));
    // Add a "Synchronize Notes" to Gnote's Application Menu
    IActionManager & am(IActionManager::obj());
    am.add_app_action("sync-notes");
    am.add_app_menu_item(IActionManager::APP_ACTION_MANAGE, 200, _("Synchronize Notes"), "app.sync-notes");

    // Initialize all the SyncServiceAddins
    manager.get_addin_manager().initialize_sync_service_addins();

    Preferences::obj().get_schema_settings(Preferences::SCHEMA_SYNC)->signal_changed()
      .connect(sigc::mem_fun(*this, &SyncManager::preferences_setting_changed));
    note_mgr().signal_note_saved.connect(sigc::mem_fun(*this, &SyncManager::handle_note_saved_or_deleted));
    note_mgr().signal_note_deleted.connect(sigc::mem_fun(*this, &SyncManager::handle_note_saved_or_deleted));
    note_mgr().signal_note_buffer_changed.connect(sigc::mem_fun(*this, &SyncManager::handle_note_buffer_changed));
    m_autosync_timer.signal_timeout.connect(sigc::mem_fun(*this, &SyncManager::background_sync_checker));

    // Update sync item based on configuration.
    update_sync_action();
  }


  void SyncManager::reset_client()
  {
    try {
      m_client->reset();
    }
    catch(std::exception & e) {
      DBG_OUT("Error deleting client manifest during reset: %s", e.what());
    }
  }


  void SyncManager::perform_synchronization(const std::tr1::shared_ptr<SyncUI> & sync_ui)
  {
    if(m_sync_thread != NULL) {
      // A synchronization thread is already running
      // TODO: Start new sync if existing dlg is for finished sync
      m_sync_ui->present_ui();
      return;
    }

    m_sync_ui = sync_ui;
    m_sync_thread = Glib::Thread::create(sigc::mem_fun(*this, &SyncManager::synchronization_thread), false);
  }


  void SyncManager::synchronization_thread()
  {
    struct finally {
      SyncServiceAddin *addin;
      finally() : addin(NULL){}
      ~finally()
      {
        SyncManager::obj().m_sync_thread = NULL;
        try {
          if(addin) {
            addin->post_sync_cleanup();
          }
        }
        catch(std::exception & e) {
          ERR_OUT("Error cleaning up addin after sync: %s", e.what());
        }
      }
    } f;
    SyncServer::Ptr server;
    try {
      f.addin = get_configured_sync_service();
      if(f.addin == NULL) {
        set_state(NO_CONFIGURED_SYNC_SERVICE);
        DBG_OUT("GetConfiguredSyncService is null");
        set_state(IDLE);
        m_sync_thread = NULL;
        return;
      }

      DBG_OUT("SyncThread using SyncServiceAddin: %s", f.addin->name().c_str());

      set_state(CONNECTING);
      try {
        server = f.addin->create_sync_server();
        if(server == NULL)
          throw new std::logic_error("addin.CreateSyncServer () returned null");
      }
      catch(std::exception & e) {
        set_state(SYNC_SERVER_CREATION_FAILED);
        ERR_OUT("Exception while creating SyncServer: %s", e.what());
        set_state(IDLE);
        m_sync_thread = NULL;
        f.addin->post_sync_cleanup();// TODO: Needed?
        return;
        // TODO: Figure out a clever way to get the specific error up to the GUI
      }

      // TODO: Call something that processes all queued note saves!
      //       For now, only saving before uploading (not sufficient for note conflict handling)

      set_state(ACQUIRING_LOCK);
      // TODO: We should really throw exceptions from BeginSyncTransaction ()
      if(!server->begin_sync_transaction()) {
        set_state(LOCKED);
        DBG_OUT("Server locked, try again later");
        set_state(IDLE);
        m_sync_thread = NULL;
        f.addin->post_sync_cleanup();
        return;
      }
      DBG_OUT("8");
      int latestServerRevision = server->latest_revision();
      int newRevision = latestServerRevision + 1;

      // If the server has been wiped or reinitialized by another client
      // for some reason, our local manifest is inaccurate and could misguide
      // sync into erroneously deleting local notes, etc.  We reset the client
      // to prevent this situation.
      std::string serverId = server->id();
      if(m_client->associated_server_id() != serverId) {
        m_client->reset();
        m_client->associated_server_id(serverId);
      }

      set_state(PREPARE_DOWNLOAD);

      // Handle notes modified or added on server
      DBG_OUT("Sync: GetNoteUpdatesSince rev %d", m_client->last_synchronized_revision());
      std::map<std::string, NoteUpdate> noteUpdates = server->get_note_updates_since(m_client->last_synchronized_revision());
      DBG_OUT("Sync: %d updates since rev %d", int(noteUpdates.size()), m_client->last_synchronized_revision());

      // Gather list of new/updated note titles
      // for title conflict handling purposes.
      std::list<std::string> noteUpdateTitles;
      for(std::map<std::string, NoteUpdate>::iterator iter = noteUpdates.begin();
          iter != noteUpdates.end(); ++iter) {
        if(iter->second.m_title != "") {
          noteUpdateTitles.push_back(iter->second.m_title);
        }
      }

      // First, check for new local notes that might have title conflicts
      // with the updates coming from the server.  Prompt the user if necessary.
      // TODO: Lots of searching here and in the next foreach...
      //       Want this stuff to happen all at once first, but
      //       maybe there's a way to store this info and pass it on?
      for(std::map<std::string, NoteUpdate>::iterator iter = noteUpdates.begin();
          iter != noteUpdates.end(); ++iter) {
        if(!find_note_by_uuid(iter->second.m_uuid) != 0) {
          Note::Ptr existingNote = note_mgr().find(iter->second.m_title);
          if(existingNote != 0 && !iter->second.basically_equal_to(existingNote)) {
            DBG_OUT("Sync: Early conflict detection for '%s'", iter->second.m_title.c_str());
            if(m_sync_ui != 0) {
              m_sync_ui->note_conflict_detected(note_mgr(), existingNote, iter->second, noteUpdateTitles);

              // Suspend this thread while the GUI is presented to
              // the user.
              //syncThread.Suspend ();  TODO: findout what to do with this!!!
            }
          }
        }
      }

      if(noteUpdates.size() > 0)
        set_state(DOWNLOADING);

      // TODO: Figure out why GUI doesn't always update smoothly

      // Process updates from the server; the bread and butter of sync!
      for(std::map<std::string, NoteUpdate>::iterator iter = noteUpdates.begin();
          iter != noteUpdates.end(); ++iter) {
        Note::Ptr existingNote = find_note_by_uuid(iter->second.m_uuid);

        if(existingNote == 0) {
          // Actually, it's possible to have a conflict here
          // because of automatically-created notes like
          // template notes (if a note with a new tag syncs
          // before its associated template). So check by
          // title and delete if necessary.
          existingNote = note_mgr().find(iter->second.m_title);
          if(existingNote != 0) {
            DBG_OUT("SyncManager: Deleting auto-generated note: %s", iter->second.m_title.c_str());
            delete_note_in_main_thread(existingNote);
          }
          create_note_in_main_thread(iter->second);
        }
        else if(existingNote->metadata_change_date() <= m_client->last_sync_date()
                || iter->second.basically_equal_to(existingNote)) {
          // Existing note hasn't been modified since last sync; simply update it from server
          update_note_in_main_thread(existingNote, iter->second);
        }
        else {
          // Logger.Debug ("Sync: Late conflict detection for '{0}'", noteUpdate.Title);
          DBG_OUT("SyncManager: Content conflict in note update for note '%s'", iter->second.m_title.c_str());
          // Note already exists locally, but has been modified since last sync; prompt user
          if(m_sync_ui != 0) {
            m_sync_ui->note_conflict_detected(note_mgr(), existingNote, iter->second, noteUpdateTitles);

            // Suspend this thread while the GUI is presented to
            // the user.
            //syncThread.Suspend ();  TODO find out what to do with this !!!
          }

          // Note has been deleted or okay'd for overwrite
          existingNote = find_note_by_uuid(iter->second.m_uuid);
          if(existingNote == 0)
            create_note_in_main_thread(iter->second);
          else
            update_note_in_main_thread(existingNote, iter->second);
        }
      }

      // Note deletion may affect the GUI, so we have to use the
      // delegate to run in the main gtk thread.
      // To be consistent, any exceptions in the delgate will be caught
      // and then rethrown in the synchronization thread.
      sync_helper_delete_notes(m_sync_helper, &server);

      // TODO: Add following updates to syncDialog treeview

      set_state(PREPARE_UPLOAD);
      // Look through all the notes modified on the client
      // and upload new or modified ones to the server
      std::list<Note::Ptr> newOrModifiedNotes;
      std::list<Note::Ptr> notes = note_mgr().get_notes();
      for(std::list<Note::Ptr>::iterator iter = notes.begin(); iter != notes.end(); ++iter) {
        if(m_client->get_revision(*iter) == -1) {
          // This is a new note that has never been synchronized to the server
          // TODO: *OR* this is a note that we lost revision info for!!!
          // TODO: Do the above NOW!!! (don't commit this dummy)
          note_save(*iter);
          newOrModifiedNotes.push_back(*iter);
          if(m_sync_ui != 0)
            m_sync_ui->note_synchronized_th((*iter)->get_title(), UPLOAD_NEW);
        }
        else if(m_client->get_revision(*iter) <= m_client->last_synchronized_revision()
                && (*iter)->metadata_change_date() > m_client->last_sync_date()) {
          note_save(*iter);
          newOrModifiedNotes.push_back(*iter);
          if(m_sync_ui != 0) {
            m_sync_ui->note_synchronized_th((*iter)->get_title(), UPLOAD_MODIFIED);
          }
        }
      }

      DBG_OUT("Sync: Uploading %d note updates", int(newOrModifiedNotes.size()));
      if(newOrModifiedNotes.size() > 0) {
        set_state(UPLOADING);
        server->upload_notes(newOrModifiedNotes); // TODO: Callbacks to update GUI as upload progresses
      }

      // Handle notes deleted on client
      std::list<std::string> locallyDeletedUUIDs;
      std::list<std::string> all_note_uuids = server->get_all_note_uuids();
      for(std::list<std::string>::iterator iter = all_note_uuids.begin();
          iter != all_note_uuids.end(); ++iter) {
        if(find_note_by_uuid(*iter) == 0) {
          locallyDeletedUUIDs.push_back(*iter);
          if(m_sync_ui != 0) {
            std::string deletedTitle = *iter;
            std::map<std::string, std::string> deleted_note_titles = m_client->deleted_note_titles();
            if(deleted_note_titles.find(*iter) != deleted_note_titles.end()) {
              deletedTitle = deleted_note_titles[*iter];
            }
            m_sync_ui->note_synchronized_th(deletedTitle, DELETE_FROM_SERVER);
          }
        }
      }
      if(locallyDeletedUUIDs.size() > 0) {
        set_state(DELETE_SERVER_NOTES);
        server->delete_notes(locallyDeletedUUIDs);
      }

      set_state(COMMITTING_CHANGES);
      bool commitResult = server->commit_sync_transaction();
      if(commitResult) {
        // Apply this revision number to all new/modified notes since last sync
        // TODO: Is this the best place to do this (after successful server commit)
        for(std::list<Note::Ptr>::iterator iter = newOrModifiedNotes.begin();
            iter != newOrModifiedNotes.end(); ++iter) {
          m_client->set_revision(*iter, newRevision);
        }
        set_state(SUCCEEDED);
      }
      else {
        set_state(FAILED);
        // TODO: Figure out a way to let the GUI know what exactly failed
      }

      // This should be equivalent to newRevision
      m_client->last_synchronized_revision(server->latest_revision());

      m_client->last_sync_date(sharp::DateTime::now());

      DBG_OUT("Sync: New revision: %d", m_client->last_synchronized_revision());

      set_state(IDLE);

    }
    catch(std::exception & e) { // top-level try
      ERR_OUT("Synchronization failed with the following exception: %s", e.what());
      // TODO: Report graphically to user
      try {
        set_state(IDLE); // stop progress
        set_state(FAILED);
        set_state(IDLE); // required to allow user to sync again
        if(server != 0) {
          // TODO: All I really want to do here is cancel
          //       the update lock timeout, but in most cases
          //       this will delete lock files, too.  Do better!
          server->cancel_sync_transaction();
        }
      }
      catch(...)
      {}
    }
  }


  void SyncManager::resolve_conflict(SyncTitleConflictResolution resolution)
  {
    if(m_sync_thread) {
      m_conflict_resolution = resolution;
    }
  }


  void SyncManager::handle_note_buffer_changed(const Note::Ptr &)
  {
    // Note changed, iff a sync is coming up we kill the
    // timer to avoid interupting the user (we want to
    // make sure not to sync more often than the user's pref)
    if(m_sync_thread == NULL) {
      sharp::TimeSpan time_since_last_check = sharp::DateTime::now() - m_last_background_check;
      if(time_since_last_check.total_minutes() > m_autosync_timeout_pref_minutes - 1) {
        DBG_OUT("Note edited...killing autosync timer until next save or delete event");
        m_autosync_timer.cancel();
      }
    }
  }


  void SyncManager::preferences_setting_changed(const Glib::ustring &)
  {
    // Update sync item based on configuration.
    update_sync_action();
  }


  void SyncManager::update_sync_action()
  {
    Glib::RefPtr<Gio::Settings> settings = Preferences::obj().get_schema_settings(Preferences::SCHEMA_SYNC);
    std::string sync_addin_id = settings->get_string(Preferences::SYNC_SELECTED_SERVICE_ADDIN);
    IActionManager::obj().get_app_action("sync-notes")->set_enabled(sync_addin_id != "");

    int timeoutPref = settings->get_int(Preferences::SYNC_AUTOSYNC_TIMEOUT);
    if(timeoutPref != m_autosync_timeout_pref_minutes) {
      m_autosync_timeout_pref_minutes = timeoutPref;
      m_autosync_timer.cancel();
      if(m_autosync_timeout_pref_minutes > 0) {
        DBG_OUT("Autosync pref changed...restarting sync timer");
        m_autosync_timeout_pref_minutes = m_autosync_timeout_pref_minutes >= 5 ? m_autosync_timeout_pref_minutes : 5;
        m_last_background_check = sharp::DateTime::now();
        // Perform a sync no sooner than user specified
        m_current_autosync_timeout_minutes = m_autosync_timeout_pref_minutes;
        m_autosync_timer.reset(m_current_autosync_timeout_minutes * 60000);
      }
    }
  }


  void SyncManager::handle_note_saved_or_deleted(const Note::Ptr &)
  {
    if(m_sync_thread == NULL && m_autosync_timeout_pref_minutes > 0) {
      sharp::TimeSpan time_since_last_check(sharp::DateTime::now() - m_last_background_check);
      sharp::TimeSpan time_until_next_check(
        sharp::TimeSpan(0, m_current_autosync_timeout_minutes, 0) - time_since_last_check);
      if(time_until_next_check.total_minutes() < 1) {
        DBG_OUT("Note saved or deleted within a minute of next autosync...resetting sync timer");
        m_current_autosync_timeout_minutes = 1;
        m_autosync_timer.reset(m_current_autosync_timeout_minutes * 60000);
      }
    }
    else if(m_sync_thread == NULL && m_autosync_timeout_pref_minutes > 0) {
      DBG_OUT("Note saved or deleted...restarting sync timer");
      m_last_background_check = sharp::DateTime::now();
      // Perform a sync one minute after setting change
      m_current_autosync_timeout_minutes = 1;
      m_autosync_timer.reset(m_current_autosync_timeout_minutes * 60000);
    }
  }


  void SyncManager::background_sync_checker()
  {
    m_last_background_check = sharp::DateTime::now();
    m_current_autosync_timeout_minutes = m_autosync_timeout_pref_minutes;
    if(m_sync_thread != NULL) {
      return;
    }
    SyncServiceAddin *addin = get_configured_sync_service();
    if(addin) {
      // TODO: block sync while checking
      SyncServer::Ptr server;
      try {
        server = SyncServer::Ptr(addin->create_sync_server());
        if(server == 0) {
          throw std::logic_error("addin->create_sync_server() returned null");
        }
      }
      catch(std::exception & e) {
        DBG_OUT("Exception while creating SyncServer: %s\n", e.what());
        addin->post_sync_cleanup();// TODO: Needed?
        return;
        // TODO: Figure out a clever way to get the specific error up to the GUI
      }
      bool server_has_updates = false;
      bool client_has_updates = m_client->deleted_note_titles().size() > 0;
      if(!client_has_updates) {
        std::list<Note::Ptr> notes = note_mgr().get_notes();
        for(std::list<Note::Ptr>::iterator iter = notes.begin(); iter != notes.end(); ++iter) {
          if(m_client->get_revision(*iter) == -1 || (*iter)->metadata_change_date() > m_client->last_sync_date()) {
            client_has_updates = true;
            break;
          }
        }
      }

      // NOTE: Important to check, at least to verify
      //       that server is available
      try {
        DBG_OUT("Checking server for updates");
        server_has_updates = server->updates_available_since(m_client->last_synchronized_revision());
      }
      catch(...) {
        // TODO: A libnotify bubble might be nice
        DBG_OUT("Error connecting to server");
        addin->post_sync_cleanup();
        return;
      }

      addin->post_sync_cleanup(); // Let FUSE unmount, etc

      if(client_has_updates || server_has_updates) {
        DBG_OUT("Detected that sync would be a good idea now");
        // TODO: Check that it's safe to sync, block other sync UIs
        perform_synchronization(SilentUI::create(note_mgr()));
      }
    }

    return;
  }


  void SyncManager::set_state(SyncState new_state)
  {
    m_state = new_state;
    if(m_sync_ui != 0) {
      // Notify the event handlers
      try {
        m_sync_ui->sync_state_changed(m_state);
      }
      catch(...)
      {}
    }
  }


  SyncServiceAddin *SyncManager::get_configured_sync_service()
  {
    SyncServiceAddin *addin = NULL;

    std::string sync_service_id = Preferences::obj()
      .get_schema_settings(Preferences::SCHEMA_SYNC)->get_string(Preferences::SYNC_SELECTED_SERVICE_ADDIN);
    if(sync_service_id != "") {
      addin = get_sync_service_addin(sync_service_id);
    }

    return addin;
  }


  SyncServiceAddin *SyncManager::get_sync_service_addin(const std::string & sync_service_id)
  {
    SyncServiceAddin *addin = NULL;

    std::list<SyncServiceAddin*> addins;
    m_note_manager.get_addin_manager().get_sync_service_addins(addins);
    for(std::list<SyncServiceAddin*>::iterator iter = addins.begin(); iter != addins.end(); ++iter) {
      if((*iter)->id() == sync_service_id) {
        addin = *iter;
        break;
      }
    }

    return addin;
  }


  void SyncManager::create_note_in_main_thread(const NoteUpdate & noteUpdate)
  {
    // Note creation may affect the GUI, so we have to use the
    // delegate to run in the main gtk thread.
    // To be consistent, any exceptions in the delgate will be caught
    // and then rethrown in the synchronization thread.
    sync_helper_create_note(m_sync_helper, const_cast<NoteUpdate*>(&noteUpdate));
  }


  void SyncManager::update_note_in_main_thread(const Note::Ptr & existingNote, const NoteUpdate & noteUpdate)
  {
    // Note update may affect the GUI, so we have to use the
    // delegate to run in the main gtk thread.
    // To be consistent, any exceptions in the delgate will be caught
    // and then rethrown in the synchronization thread.
    sync_helper_update_note(m_sync_helper, const_cast<Note::Ptr*>(&existingNote), const_cast<NoteUpdate*>(&noteUpdate));
  }


  void SyncManager::delete_note_in_main_thread(const Note::Ptr & existingNote)
  {
    // Note deletion may affect the GUI, so we have to use the
    // delegate to run in the main gtk thread.
    // To be consistent, any exceptions in the delgate will be caught
    // and then rethrown in the synchronization thread.
    sync_helper_delete_note(m_sync_helper, const_cast<Note::Ptr*>(&existingNote));
  }


  void SyncManager::update_local_note(const Note::Ptr & localNote, const NoteUpdate & serverNote, NoteSyncType syncType)
  {
    // In each case, update existingNote's content and revision
    try {
      localNote->load_foreign_note_xml(serverNote.m_xml_content, OTHER_DATA_CHANGED);
    }
    catch(...)
    {} // TODO: Handle exception in case that serverNote.XmlContent is invalid XML
    m_client->set_revision(localNote, serverNote.m_latest_revision);

    // Update dialog's sync status
    if(m_sync_ui != 0) {
      m_sync_ui->note_synchronized(localNote->get_title(), syncType);
    }
  }


  Note::Ptr SyncManager::find_note_by_uuid(const std::string & uuid)
  {
    return note_mgr().find_by_uri("note://gnote/" + uuid);
  }


  NoteManager & SyncManager::note_mgr()
  {
    return m_note_manager;
  }


  bool SyncManager::synchronized_note_xml_matches(const std::string & noteXml1, const std::string & noteXml2)
  {
    try {
      std::string title1, tags1, content1;
      std::string title2, tags2, content2;

      get_synchronized_xml_bits(noteXml1, title1, tags1, content1);
      get_synchronized_xml_bits(noteXml2, title2, tags2, content2);

      return title1 == title2 && tags1 == tags2 && content1 == content2;
    }
    catch(std::exception & e) {
      DBG_OUT("synchronized_note_xml_matches threw exception: %s", e.what());
      return false;
    }
  }


  void SyncManager::get_synchronized_xml_bits(const std::string & noteXml, std::string & title, std::string & tags, std::string & content)
  {
    title = "";
    tags = "";
    content = "";

    sharp::XmlReader xml;
    xml.load_buffer(noteXml);
    while(xml.read()) {
      switch(xml.get_node_type()) {
      case XML_READER_TYPE_ELEMENT:
        if(xml.get_name() == "title") {
          title = xml.read_string();
        }
        else if(xml.get_name() == "tags") {
          tags = xml.read_inner_xml();
          DBG_OUT("In the bits: tags = %s", tags.c_str()); // TODO: Delete
        }
        else if(xml.get_name() == "text") {
          content = xml.read_inner_xml();
        }
      default:
        break;
      }
    }
  }


  void SyncManager::on_delete_notes(GObject*, gpointer serv, gpointer)
  {
    try {
      SyncServer::Ptr & server = *static_cast<SyncServer::Ptr*>(serv);
      // Make list of all local notes
      std::list<Note::Ptr> localNotes = SyncManager::obj().note_mgr().get_notes();

      // Get all notes currently on server
      std::list<std::string> serverNotes = server->get_all_note_uuids();

      // Delete notes locally that have been deleted on the server
      for(std::list<Note::Ptr>::iterator iter = localNotes.begin(); iter != localNotes.end(); ++iter) {
	if(SyncManager::obj().m_client->get_revision(*iter) != -1
	   && std::find(serverNotes.begin(), serverNotes.end(), (*iter)->id()) == serverNotes.end()) {
	  if(SyncManager::obj().m_sync_ui != 0) {
	    SyncManager::obj().m_sync_ui->note_synchronized((*iter)->get_title(), DELETE_FROM_CLIENT);
	  }
	  SyncManager::obj().note_mgr().delete_note(*iter);
	}
      }
    }
    catch(std::exception & e) {
      DBG_OUT("Exception caught in %s: %s\n", __func__, e.what());
    }
    catch(...) {
      DBG_OUT("Exception caught in %s\n", __func__);
    }
  }


  void SyncManager::on_create_note(GObject*, gpointer note_update, gpointer)
  {
    try {
      NoteUpdate & noteUpdate = *static_cast<NoteUpdate*>(note_update);
      Note::Ptr existingNote = SyncManager::obj().note_mgr().create_with_guid(noteUpdate.m_title, noteUpdate.m_uuid);
      SyncManager::obj().update_local_note(existingNote, noteUpdate, DOWNLOAD_NEW);
    }
    catch(std::exception & e) {
      DBG_OUT("Exception caught in %s: %s\n", __func__, e.what());
    }
    catch(...) {
      DBG_OUT("Exception caught in %s\n", __func__);
    }
  }


  void SyncManager::on_update_note(GObject*, gpointer existing_note, gpointer note_update, gpointer)
  {
    try {
      Note::Ptr *existingNote = static_cast<Note::Ptr*>(existing_note);
      NoteUpdate & noteUpdate = *static_cast<NoteUpdate*>(note_update);
      SyncManager::obj().update_local_note(*existingNote, noteUpdate, DOWNLOAD_MODIFIED);
    }
    catch(std::exception & e) {
      DBG_OUT("Exception caught in %s: %s\n", __func__, e.what());
    }
    catch(...) {
      DBG_OUT("Exception caught in %s\n", __func__);
    }
  }


  void SyncManager::on_delete_note(GObject*, gpointer existing_note, gpointer)
  {
    try {
      Note::Ptr *existingNote = static_cast<Note::Ptr*>(existing_note);
      SyncManager::obj().note_mgr().delete_note(*existingNote);
    }
    catch(std::exception & e) {
      DBG_OUT("Exception caught in %s: %s\n", __func__, e.what());
    }
    catch(...) {
      DBG_OUT("Exception caught in %s\n", __func__);
    }
  }


  void SyncManager::note_save(const Note::Ptr & note)
  {
    gdk_threads_enter();
    note->save();
    gdk_threads_leave();
  }


  SyncLockInfo::SyncLockInfo()
    : client_id(Preferences::obj().get_schema_settings(Preferences::SCHEMA_SYNC)->get_string(Preferences::SYNC_CLIENT_ID))
    , transaction_id(sharp::uuid().string())
    , renew_count(0)
    , duration(0, 2, 0) // default of 2 minutes
    , revision(0)
  {
  }


  std::string SyncLockInfo::hash_string()
  {
    return str(boost::format("%1%-%2%-%3%-%4%-%5%") % transaction_id % client_id % renew_count % duration.string() % revision);
  }


  SyncServer::~SyncServer()
  {}

}
}
