/*
 * gnote
 *
 * Copyright (C) 2012-2014,2017,2019-2025 Aurimas Cernius
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

#include <glibmm/i18n.h>
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
#include "sharp/xmlreader.hpp"


namespace gnote {
namespace sync {

namespace {

  std::vector<Glib::ustring> get_updated_note_titles(const SyncServer::NoteUpdatesMap &note_updates)
  {
    std::vector<Glib::ustring> titles;
    titles.reserve(note_updates.size());
    for(auto & iter : note_updates) {
      if(iter.second.m_title != "") {
        titles.push_back(iter.second.m_title);
      }
    }

    return titles;
  }

}

  SyncManager::SyncManager(IGnote & g, NoteManagerBase & m)
    : m_gnote(g)
    , m_note_manager(m)
    , m_state(IDLE)
  {
  }

  SyncManager::~SyncManager()
  {
    if (m_sync_checker_thread) {
      m_sync_checker_thread->join();
      m_sync_checker_thread.reset();
    }
  }

  void SyncManager::init()
  {
    m_client.reset(GnoteSyncClient::create(m_note_manager));
    // Add a "Synchronize Notes" to Gnote's Application Menu
    IActionManager & am(m_gnote.action_manager());
    am.add_app_action("sync-notes");
    am.add_app_menu_item(APP_SECTION_MANAGE, 200, _("Synchronize Notes"), "app.sync-notes");

    // Initialize all the SyncServiceAddins
    initialize_sync_service_addins(m_note_manager);

    connect_system_signals();

    // Update sync item based on configuration.
    update_sync_action();
  }


  void SyncManager::initialize_sync_service_addins(NoteManagerBase & note_manager)
  {
    try {
      NoteManager & manager(dynamic_cast<NoteManager&>(note_manager));
      manager.get_addin_manager().initialize_sync_service_addins();
    }
    catch(std::bad_cast & e) {
      ERR_OUT(_("Report a bug. Cast failed: %s"), e.what());
    }
  }


  void SyncManager::connect_system_signals()
  {
    try {
      NoteManager & manager(dynamic_cast<NoteManager&>(note_mgr()));
      m_gnote.preferences().signal_sync_selected_service_addin_changed.connect(sigc::mem_fun(*this, &SyncManager::update_sync_action));
      m_gnote.preferences().signal_sync_autosync_timeout_changed.connect(sigc::mem_fun(*this, &SyncManager::update_sync_action));
      manager.signal_note_saved.connect(sigc::mem_fun(*this, &SyncManager::handle_note_saved_or_deleted));
      manager.signal_note_deleted.connect(sigc::mem_fun(*this, &SyncManager::handle_note_saved_or_deleted));
      manager.signal_note_buffer_changed.connect(sigc::mem_fun(*this, &SyncManager::handle_note_buffer_changed));
      m_autosync_timer.signal_timeout.connect(sigc::mem_fun(*this, &SyncManager::background_sync_checker));
    }
    catch(std::bad_cast & e) {
      ERR_OUT(_("Report a bug. Cast failed: %s"), e.what());
    }
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


  void SyncManager::perform_synchronization(const SyncUI::Ptr & sync_ui)
  {
    if(m_sync_thread) {
      // A synchronization thread is already running
      // TODO: Start new sync if existing dlg is for finished sync
      DBG_OUT("A synchronization thread is already running");
      m_sync_ui->present_ui();
      return;
    }

    m_sync_ui = sync_ui;
    DBG_OUT("Creating synchronization thread");
    m_sync_thread.reset(new std::thread([this] { synchronization_thread(); }));
    m_sync_thread->detach();
  }


  void SyncManager::synchronization_thread()
  {
    struct finally {
      SyncManager & manager;
      SyncServiceAddin *addin;
      finally(SyncManager & m) : manager(m), addin(NULL){}
      ~finally()
      {
        try {
          if(addin) {
            addin->post_sync_cleanup();
          }
        }
        catch(std::exception & e) {
          ERR_OUT(_("Error cleaning up addin after synchronization: %s"), e.what());
        }
        auto &m = manager;
        utils::main_context_invoke([&m] { m.m_sync_thread.reset(); });
      }
    } f(*this);
    std::unique_ptr<SyncServer> server;
    try {
      f.addin = get_configured_sync_service();
      if(f.addin == NULL) {
        set_state(NO_CONFIGURED_SYNC_SERVICE);
        DBG_OUT("get_configured_sync_service is null");
        set_state(IDLE);
        return;
      }

      DBG_OUT("synchronization_thread using SyncServiceAddin: %s", f.addin->name().c_str());

      set_state(CONNECTING);
      try {
        server.reset(f.addin->create_sync_server());
        if(server == NULL)
          throw std::logic_error("addin.create_sync_server() returned NULL");
      }
      catch(std::exception & e) {
        set_state(SYNC_SERVER_CREATION_FAILED);
        ERR_OUT(_("Exception while creating SyncServer: %s"), e.what());
        set_state(IDLE);
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
        f.addin->post_sync_cleanup();
        return;
      }
      DBG_OUT("8");
      const auto latest_server_revision = server->latest_revision();
      const int new_revision = latest_server_revision + 1;

      // If the server has been wiped or reinitialized by another client
      // for some reason, our local manifest is inaccurate and could misguide
      // sync into erroneously deleting local notes, etc.  We reset the client
      // to prevent this situation.
      const Glib::ustring server_id = server->id();
      if(m_client->associated_server_id() != server_id) {
        m_client->reset();
        m_client->associated_server_id(server_id);
      }

      m_client->begin_synchronization();
      set_state(PREPARE_DOWNLOAD);

      // Handle notes modified or added on server
      DBG_OUT("Sync: get_note_updates_since rev %d", m_client->last_synchronized_revision());
      const auto note_updates = server->get_note_updates_since(m_client->last_synchronized_revision());
      DBG_OUT("Sync: %zu updates since rev %d", note_updates.size(), m_client->last_synchronized_revision());

      // Gather list of new/updated note titles
      // for title conflict handling purposes.
      const std::vector<Glib::ustring> note_update_titles{get_updated_note_titles(note_updates)};

      // First, check for new local notes that might have title conflicts
      // with the updates coming from the server.  Prompt the user if necessary.
      // TODO: Lots of searching here and in the next foreach...
      //       Want this stuff to happen all at once first, but
      //       maybe there's a way to store this info and pass it on?
      for(auto & iter : note_updates) {
        if(find_note_by_uuid(iter.second.m_uuid)) {
          auto existing_note = note_mgr().find(iter.second.m_title);
          if(existing_note && !iter.second.basically_equal_to(existing_note.value())) {
            DBG_OUT("Sync: Early conflict detection for '%s'", iter.second.m_title.c_str());
            if(m_sync_ui != 0) {
              m_sync_ui->note_conflict_detected(existing_note.value(), iter.second, note_update_titles);
            }
          }
        }
      }

      if(note_updates.size() > 0)
        set_state(DOWNLOADING);

      // TODO: Figure out why GUI doesn't always update smoothly

      // Process updates from the server; the bread and butter of sync!
      for(auto & iter : note_updates) {
        auto existing_note = find_note_by_uuid(iter.second.m_uuid);

        if(!existing_note) {
          // Actually, it's possible to have a conflict here
          // because of automatically-created notes like
          // template notes (if a note with a new tag syncs
          // before its associated template). So check by
          // title and delete if necessary.
          auto existingNote = note_mgr().find(iter.second.m_title);
          if(existingNote) {
            DBG_OUT("SyncManager: Deleting auto-generated note: %s", iter.second.m_title.c_str());
            delete_note_in_main_thread(existingNote.value());
          }
          create_note_in_main_thread(iter.second);
        }
        else {
          NoteBase & existing = existing_note.value();
          if(existing.metadata_change_date() <= m_client->last_sync_date()
                || iter.second.basically_equal_to(existing)) {
            // Existing note hasn't been modified since last sync; simply update it from server
            update_note_in_main_thread(existing, iter.second);
          }
          else {
            // Logger.Debug ("Sync: Late conflict detection for '{0}'", noteUpdate.Title);
            DBG_OUT("SyncManager: Content conflict in note update for note '%s'", iter.second.m_title.c_str());
            // Note already exists locally, but has been modified since last sync; prompt user
            if(m_sync_ui != 0) {
              m_sync_ui->note_conflict_detected(existing, iter.second, note_update_titles);
            }

            if(auto existing_note = find_note_by_uuid(iter.second.m_uuid)) {
              update_note_in_main_thread(existing_note.value(), iter.second);
            }
            else {
              // Note has been deleted or okay'd for overwrite
              create_note_in_main_thread(iter.second);
            }
          }
        }
      }

      // Note deletion may affect the GUI, so we have to use the
      // delegate to run in the main gtk thread.
      // To be consistent, any exceptions in the delgate will be caught
      // and then rethrown in the synchronization thread.
      delete_notes_in_main_thread(*server);

      // TODO: Add following updates to syncDialog treeview

      set_state(PREPARE_UPLOAD);
      // Look through all the notes modified on the client
      // and upload new or modified ones to the server
      std::vector<NoteBase::Ref> new_or_modified_notes;
      note_mgr().for_each([this, &new_or_modified_notes](NoteBase & note) {
        // This is a new note that has never been synchronized to the server
        bool new_note = m_client->get_revision(note) == -1;
        bool modified = m_client->get_revision(note) <= m_client->last_synchronized_revision()
          && note.metadata_change_date() > m_client->last_sync_date();
        if(new_note || modified) {
          note_save(note);
          new_or_modified_notes.push_back(note);
          if(m_sync_ui != 0) {
            m_sync_ui->note_synchronized_th(note.get_title(), new_note? UPLOAD_NEW : UPLOAD_MODIFIED);
          }
        }
      });

      DBG_OUT("Sync: Uploading %zu note updates", new_or_modified_notes.size());
      if(new_or_modified_notes.size() > 0) {
        set_state(UPLOADING);
        server->upload_notes(new_or_modified_notes); // TODO: Callbacks to update GUI as upload progresses
      }

      // Handle notes deleted on client
      std::vector<Glib::ustring> locally_deleted_uuids;
      auto all_note_uuids = server->get_all_note_uuids();
      for(auto & iter : all_note_uuids) {
        if(!find_note_by_uuid(iter)) {
          locally_deleted_uuids.push_back(iter);
          if(m_sync_ui != 0) {
            Glib::ustring deleted_title = iter;
            auto &deleted_note_titles = m_client->deleted_note_titles();
            auto deleted_note = deleted_note_titles.find(iter);
            if(deleted_note != deleted_note_titles.end()) {
              deleted_title = deleted_note->second;
            }
            m_sync_ui->note_synchronized_th(deleted_title, DELETE_FROM_SERVER);
          }
        }
      }
      if(locally_deleted_uuids.size() > 0) {
        set_state(DELETE_SERVER_NOTES);
        server->delete_notes(locally_deleted_uuids);
      }

      set_state(COMMITTING_CHANGES);
      bool commitResult = server->commit_sync_transaction();
      if(commitResult) {
        // Apply this revision number to all new/modified notes since last sync
        // TODO: Is this the best place to do this (after successful server commit)
        for(NoteBase & iter : new_or_modified_notes) {
          m_client->set_revision(iter, new_revision);
        }
        set_state(SUCCEEDED);
      }
      else {
        set_state(FAILED);
        // TODO: Figure out a way to let the GUI know what exactly failed
      }

      // This should be equivalent to new_revision
      m_client->last_synchronized_revision(server->latest_revision());

      m_client->last_sync_date(Glib::DateTime::create_now_utc());
      m_client->end_synchronization();

      DBG_OUT("Sync: New revision: %d", m_client->last_synchronized_revision());

      set_state(IDLE);

    }
    catch(std::exception & e) { // top-level try
      ERR_OUT(_("Synchronization failed with the following exception: %s"), e.what());
      abort_sync(server.get());
    }
  }


  void SyncManager::abort_sync(SyncServer *server)
  {
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
        m_client->cancel_synchronization();
      }
    }
    catch(...)
    {}
  }


  void SyncManager::handle_note_buffer_changed(NoteBase &)
  {
    // Note changed, iff a sync is coming up we kill the
    // timer to avoid interupting the user (we want to
    // make sure not to sync more often than the user's pref)
    if(!m_sync_thread) {
      Glib::TimeSpan time_since_last_check = Glib::DateTime::create_now_utc().difference(m_last_background_check);
      if(sharp::time_span_total_minutes(time_since_last_check) > m_autosync_timeout_pref_minutes - 1) {
        DBG_OUT("Note edited...killing autosync timer until next save or delete event");
        m_autosync_timer.cancel();
      }
    }
  }


  void SyncManager::update_sync_action()
  {
    Glib::ustring sync_addin_id = m_gnote.preferences().sync_selected_service_addin();
    m_gnote.action_manager().get_app_action("sync-notes")->set_enabled(sync_addin_id != "");

    int timeoutPref = m_gnote.preferences().sync_autosync_timeout();
    if(timeoutPref != m_autosync_timeout_pref_minutes) {
      m_autosync_timeout_pref_minutes = timeoutPref;
      m_autosync_timer.cancel();
      if(m_autosync_timeout_pref_minutes > 0) {
        DBG_OUT("Autosync pref changed...restarting sync timer");
        m_autosync_timeout_pref_minutes = m_autosync_timeout_pref_minutes >= 5 ? m_autosync_timeout_pref_minutes : 5;
        m_last_background_check = Glib::DateTime::create_now_utc();
        // Perform a sync no sooner than user specified
        m_current_autosync_timeout_minutes = m_autosync_timeout_pref_minutes;
        m_autosync_timer.reset(m_current_autosync_timeout_minutes * 60000);
      }
    }
  }


  void SyncManager::handle_note_saved_or_deleted(NoteBase &)
  {
    if(!m_sync_thread && m_autosync_timeout_pref_minutes > 0) {
      DBG_OUT("Note saved or deleted...restarting sync timer");
      m_last_background_check = Glib::DateTime::create_now_utc();
      // Perform a sync one minute after setting change
      m_current_autosync_timeout_minutes = 1;
      m_autosync_timer.reset(m_current_autosync_timeout_minutes * 60000);
    }
  }


  void SyncManager::sync_checker_thread()
  {
    bool need_update = false;

    m_last_background_check = Glib::DateTime::create_now_utc();
    m_current_autosync_timeout_minutes = m_autosync_timeout_pref_minutes;
    if(m_sync_thread) {
      utils::main_context_invoke([this, need_update]() { on_sync_checker_finished(need_update); });
      return;
    }
    SyncServiceAddin *addin = get_configured_sync_service();
    if(addin) {
      // TODO: block sync while checking
      std::unique_ptr<SyncServer> server;
      try {
        server.reset(addin->create_sync_server());
        if(server == 0) {
          throw std::logic_error("addin->create_sync_server() returned null");
        }
      }
      catch(std::exception & e) {
        DBG_OUT("Exception while creating SyncServer: %s\n", e.what());
        addin->post_sync_cleanup();// TODO: Needed?
        utils::main_context_invoke([this, need_update]() { on_sync_checker_finished(need_update); });
        return;
        // TODO: Figure out a clever way to get the specific error up to the GUI
      }
      bool server_has_updates = false;
      bool client_has_updates = m_client->deleted_note_titles().size() > 0;
      if(!client_has_updates) {
        client_has_updates = note_mgr().search([this](const NoteBase & note, bool & has_updates) {
          if(m_client->get_revision(note) == -1 || note.metadata_change_date() > m_client->last_sync_date()) {
            has_updates = true;
            return false;
          }
          return true;
        }, false);
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
        utils::main_context_invoke([this, need_update]() { on_sync_checker_finished(need_update); });
        return;
      }

      addin->post_sync_cleanup(); // Let FUSE unmount, etc

      if(client_has_updates || server_has_updates) {
        DBG_OUT("Detected that sync would be a good idea now");
        // TODO: Check that it's safe to sync, block other sync UIs
	need_update = true;
      }
    }

    utils::main_context_invoke([this, need_update]() { on_sync_checker_finished(need_update); });

    return;
  }


  void SyncManager::on_sync_checker_finished(bool need_update)
  {
    if (m_sync_checker_thread) {
      m_sync_checker_thread->join();
      m_sync_checker_thread.reset();
    }

    if (need_update) {
      perform_synchronization(SilentUI::create(m_gnote, note_mgr()));
    }
  }


  void SyncManager::background_sync_checker()
  {
    if(m_sync_checker_thread) {
      // A synchronization checker thread is already running
      DBG_OUT("A sync checker thread is already running");
      return;
    }

    DBG_OUT("Creating sync checker thread");
    m_sync_checker_thread.reset(new std::thread([this] { sync_checker_thread(); }));
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

    Glib::ustring sync_service_id = m_gnote.preferences().sync_selected_service_addin();
    if(sync_service_id != "") {
      addin = get_sync_service_addin(sync_service_id);
    }

    return addin;
  }


  SyncServiceAddin *SyncManager::get_sync_service_addin(const Glib::ustring & sync_service_id)
  {
    SyncServiceAddin *addin = NULL;

    try {
      NoteManager & manager(dynamic_cast<NoteManager&>(m_note_manager));
      std::vector<SyncServiceAddin*> addins = manager.get_addin_manager().get_sync_service_addins();
      for(auto iter : addins) {
        if(iter->id() == sync_service_id) {
          addin = iter;
          break;
        }
      }
    }
    catch(std::bad_cast & e) {
      ERR_OUT(_("Report a bug. Cast failed: %s"), e.what());
    }

    return addin;
  }


  void SyncManager::create_note_in_main_thread(const NoteUpdate & noteUpdate)
  {
    // Note creation may affect the GUI, so we have to use the
    // delegate to run in the main gtk thread.
    // To be consistent, any exceptions in the delgate will be caught
    // and then rethrown in the synchronization thread.
    utils::main_context_call([this, noteUpdate]() { create_note(noteUpdate); });
  }


  void SyncManager::update_note_in_main_thread(const NoteBase & existing_note, const NoteUpdate & note_update)
  {
    // Note update may affect the GUI, so we have to use the
    // delegate to run in the main gtk thread.
    auto existing = existing_note.uri();
    utils::main_context_call([this, existing, note_update]() {
      note_mgr().find_by_uri(existing, [this, note_update](NoteBase & note) {
        update_note(note, note_update);
      });
    });
  }


  void SyncManager::delete_note_in_main_thread(const NoteBase & existing_note)
  {
    // Note deletion may affect the GUI, so we have to use the
    // delegate to run in the main gtk thread.
    auto existing = existing_note.uri();
    utils::main_context_call([this, existing]() {
      auto note = note_mgr().find_by_uri(existing);
      if(note) {
        delete_note(note.value());
      }
    });
  }


  void SyncManager::update_local_note(NoteBase & local_note, const NoteUpdate & server_note, NoteSyncType sync_type)
  {
    // In each case, update existingNote's content and revision
    try {
      local_note.load_foreign_note_xml(server_note.m_xml_content, OTHER_DATA_CHANGED);
    }
    catch(...)
    {} // TODO: Handle exception in case that serverNote.XmlContent is invalid XML
    m_client->set_revision(local_note, server_note.m_latest_revision);

    // Update dialog's sync status
    if(m_sync_ui != 0) {
      m_sync_ui->note_synchronized(local_note.get_title(), sync_type);
    }
  }


  NoteBase::ORef SyncManager::find_note_by_uuid(const Glib::ustring & uuid)
  {
    auto note = note_mgr().find_by_uri("note://gnote/" + uuid);
    if(note) {
      return std::ref(*note);
    }
    return NoteBase::ORef();
  }


  NoteManagerBase & SyncManager::note_mgr()
  {
    return m_note_manager;
  }


  bool SyncManager::synchronized_note_xml_matches(const Glib::ustring & noteXml1, const Glib::ustring & noteXml2)
  {
    try {
      Glib::ustring title1, tags1, content1;
      Glib::ustring title2, tags2, content2;

      get_synchronized_xml_bits(noteXml1, title1, tags1, content1);
      get_synchronized_xml_bits(noteXml2, title2, tags2, content2);

      return title1 == title2 && tags1 == tags2 && content1 == content2;
    }
    catch(std::exception & e) {
      DBG_OUT("synchronized_note_xml_matches threw exception: %s", e.what());
      return false;
    }
  }


  void SyncManager::get_synchronized_xml_bits(const Glib::ustring & noteXml, Glib::ustring & title, Glib::ustring & tags, Glib::ustring & content)
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


  void SyncManager::delete_notes_in_main_thread(SyncServer & server)
  {
    utils::main_context_call([this, &server]() { delete_notes(server); });
  }


  void SyncManager::delete_notes(SyncServer & server)
  {
    try {
      // Get all notes currently on server
      auto server_notes = server.get_all_note_uuids();
      std::vector<NoteBase::Ref> to_delete;

      // Delete notes locally that have been deleted on the server
      note_mgr().for_each([this, server_notes=std::move(server_notes), &to_delete](NoteBase & note) {
	if(m_client->get_revision(note) != -1
	   && std::find(server_notes.begin(), server_notes.end(), note.id()) == server_notes.end()) {
	  if(m_sync_ui != 0) {
	    m_sync_ui->note_synchronized(note.get_title(), DELETE_FROM_CLIENT);
	  }
          to_delete.push_back(note);
	}
      });

      for(auto note : to_delete) {
        note_mgr().delete_note(note);
      }
    }
    catch(std::exception & e) {
      DBG_OUT("Exception caught in %s: %s\n", __func__, e.what());
    }
    catch(...) {
      DBG_OUT("Exception caught in %s\n", __func__);
    }
  }


  void SyncManager::create_note(const NoteUpdate & noteUpdate)
  {
    try {
      auto & existingNote = note_mgr().create_with_guid(Glib::ustring(noteUpdate.m_title), Glib::ustring(noteUpdate.m_uuid));
      update_local_note(existingNote, noteUpdate, DOWNLOAD_NEW);
    }
    catch(std::exception & e) {
      DBG_OUT("Exception caught in %s: %s\n", __func__, e.what());
    }
    catch(...) {
      DBG_OUT("Exception caught in %s\n", __func__);
    }
  }


  void SyncManager::update_note(NoteBase & existing_note, const NoteUpdate & note_update)
  {
    try {
      update_local_note(existing_note, note_update, DOWNLOAD_MODIFIED);
    }
    catch(std::exception & e) {
      DBG_OUT("Exception caught in %s: %s\n", __func__, e.what());
    }
    catch(...) {
      DBG_OUT("Exception caught in %s\n", __func__);
    }
  }


  void SyncManager::delete_note(NoteBase & existing_note)
  {
    try {
      note_mgr().delete_note(existing_note);
    }
    catch(std::exception & e) {
      DBG_OUT("Exception caught in %s: %s\n", __func__, e.what());
    }
    catch(...) {
      DBG_OUT("Exception caught in %s\n", __func__);
    }
  }


  void SyncManager::note_save(const NoteBase & note)
  {
    auto uri = note.uri();
    utils::main_context_call([this, uri] {
      note_mgr().find_by_uri(uri, [](NoteBase & note) { note.save(); });
    });
  }


}
}
