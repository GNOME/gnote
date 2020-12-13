/*
 * gnote
 *
 * Copyright (C) 2019,2020 Aurimas Cernius
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


#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/table.h>
#include <glibmm/thread.h>

#include "debug.hpp"
#include "gvfssyncserviceaddin.hpp"
#include "ignote.hpp"
#include "sharp/directory.hpp"
#include "sharp/files.hpp"
#include "synchronization/filesystemsyncserver.hpp"


namespace gvfssyncservice {

const char *SCHEMA_SYNC_GVFS = "org.gnome.gnote.sync.gvfs";
const Glib::ustring SYNC_GVFS_URI = "uri";


GvfsSyncServiceModule::GvfsSyncServiceModule()
{
  ADD_INTERFACE_IMPL(GvfsSyncServiceAddin);
}




GvfsSyncServiceAddin::GvfsSyncServiceAddin()
  : m_uri_entry(nullptr)
  , m_initialized(false)
  , m_enabled(false)
{
}

void GvfsSyncServiceAddin::initialize()
{
  m_initialized = true;
  m_enabled = true;
  if(!m_gvfs_settings) {
    m_gvfs_settings = Gio::Settings::create(SCHEMA_SYNC_GVFS);
  }
}

void GvfsSyncServiceAddin::shutdown()
{
  m_enabled = false;
}

gnote::sync::SyncServer *GvfsSyncServiceAddin::create_sync_server()
{
  gnote::sync::SyncServer *server;

  Glib::ustring sync_uri;
  if(get_config_settings(sync_uri)) {
    m_uri = sync_uri;
    if(sharp::directory_exists(m_uri) == false) {
      sharp::directory_create(m_uri);
    }

    auto path = Gio::File::create_for_uri(m_uri);
    if(!mount(path)) {
      throw sharp::Exception(_("Failed to mount the folder"));
    }
    if(!path->query_exists())
      sharp::directory_create(path);

    server = gnote::sync::FileSystemSyncServer::create(path, ignote().preferences());
  }
  else {
    throw std::logic_error("GvfsSyncServiceAddin.create_sync_server() called without being configured");
  }

  return server;
}


bool GvfsSyncServiceAddin::mount(const Glib::RefPtr<Gio::File> & path)
{
  bool ret = true, done = false;
  Glib::Mutex mutex;
  Glib::Cond cond;
  mutex.lock();
  if(mount_async(path, [&ret, &mutex, &cond, &done](bool result, const Glib::ustring&) {
       mutex.lock();
       ret = result;
       done = true;
       cond.signal();
       mutex.unlock();
     })) {
    mutex.unlock();
    return true;
  }

  while(!done) {
    cond.wait(mutex);
  }
  mutex.unlock();
  return ret;
}

bool GvfsSyncServiceAddin::mount_async(const Glib::RefPtr<Gio::File> & path, const sigc::slot<void, bool, Glib::ustring> & completed)
{
  try {
    path->find_enclosing_mount();
    return true;
  }
  catch(Gio::Error & e) {
  }

  auto root = path;
  auto parent = root->get_parent();
  while(parent) {
    root = parent;
    parent = root->get_parent();
  }

  root->mount_enclosing_volume([this, root, completed](Glib::RefPtr<Gio::AsyncResult> & result) {
    try {
      if(root->mount_enclosing_volume_finish(result)) {
        m_mount = root->find_enclosing_mount();
      }
    }
    catch(...) {
    }

    completed(bool(m_mount), "");
  });

  return false;
}


void GvfsSyncServiceAddin::unmount()
{
  if(!m_mount) {
    return;
  }

  Glib::Mutex mutex;
  Glib::Cond cond;
  mutex.lock();
  unmount_async([&mutex, &cond]{
    mutex.lock();
    cond.signal();
    mutex.unlock();
  });
  cond.wait(mutex);
  mutex.unlock();
}


void GvfsSyncServiceAddin::unmount_async(const sigc::slot<void> & completed)
{
  if(!m_mount) {
    completed();
    return;
  }

  m_mount->unmount([this, completed](Glib::RefPtr<Gio::AsyncResult> & result) {
    try {
      m_mount->unmount_finish(result);
    }
    catch(...) {
    }

    m_mount.reset();
    completed();
  });
}


void GvfsSyncServiceAddin::post_sync_cleanup()
{
  unmount();
}


Gtk::Widget *GvfsSyncServiceAddin::create_preferences_control(EventHandler required_pref_changed)
{
  Gtk::Table *table = manage(new Gtk::Table(1, 3, false));
  table->set_row_spacings(5);
  table->set_col_spacings(10);

  // Read settings out of gconf
  Glib::ustring sync_path;
  if(get_config_settings(sync_path) == false) {
    sync_path = "";
  }

  auto l = manage(new Gtk::Label(_("Folder _URI:"), true));
  l->property_xalign() = 1;
  table->attach(*l, 0, 1, 0, 1, Gtk::FILL);

  m_uri_entry = manage(new Gtk::Entry);
  m_uri_entry->set_text(sync_path);
  m_uri_entry->get_buffer()->signal_inserted_text().connect([required_pref_changed](guint, const gchar*, guint) { required_pref_changed(); });
  m_uri_entry->get_buffer()->signal_deleted_text().connect([required_pref_changed](guint, guint) { required_pref_changed(); });
  l->set_mnemonic_widget(*m_uri_entry);

  table->attach(*m_uri_entry, 1, 2, 0, 1);

  auto example = manage(new Gtk::Label(_("Example: google-drive://name.surname@gmail.com/notes")));
  example->property_xalign() = 0;
  table->attach(*example, 1, 2, 1, 2);
  auto account_info = manage(new Gtk::Label(_("Please, register your account in Online Accounts")));
  account_info->property_xalign() = 0;
  table->attach(*account_info, 1, 2, 2, 3);

  table->set_hexpand(true);
  table->set_vexpand(false);
  table->show_all();
  return table;
}


bool GvfsSyncServiceAddin::save_configuration(const sigc::slot<void, bool, Glib::ustring> & on_saved)
{
  Glib::ustring sync_uri = m_uri_entry->get_text();

  if(sync_uri == "") {
    ERR_OUT(_("The URI is empty"));
    throw gnote::sync::GnoteSyncException(_("URI field is empty."));
  }

  auto path = Gio::File::create_for_uri(sync_uri);
  auto on_mount_completed = [this, path, sync_uri, on_saved](bool success, Glib::ustring error) {
      if(success) {
        success = test_sync_directory(path, sync_uri, error);
      }
      unmount_async([this, sync_uri, on_saved, success, error] {
        if(success) {
          m_uri = sync_uri;
          m_gvfs_settings->set_string(SYNC_GVFS_URI, m_uri);
        }
        on_saved(success, error);
      });
  };
  if(mount_async(path, on_mount_completed)) {
    Glib::Thread::create([this, sync_uri, on_mount_completed]() {
      on_mount_completed(true, "");
    }, false);
  }

  return true;
}


bool GvfsSyncServiceAddin::test_sync_directory(const Glib::RefPtr<Gio::File> & path, const Glib::ustring & sync_uri, Glib::ustring & error)
{
  try {
    if(sharp::directory_exists(path) == false) {
      if(!sharp::directory_create(path)) {
        DBG_OUT("Could not create \"%s\"", sync_uri.c_str());
        error = _("Specified folder path does not exist, and Gnote was unable to create it.");
        return false;
      }
    }
    else {
      // Test creating/writing/deleting a file
      Glib::ustring test_path_base = Glib::build_filename(sync_uri, "test");
      Glib::RefPtr<Gio::File> test_path = Gio::File::create_for_uri(test_path_base);
      int count = 0;

      // Get unique new file name
      while(test_path->query_exists()) {
        test_path = Gio::File::create_for_uri(test_path_base + TO_STRING(++count));
      }

      // Test ability to create and write
      Glib::ustring test_line = "Testing write capabilities.";
      auto stream = test_path->create_file();
      stream->write(test_line);
      stream->close();

      if(!test_path->query_exists()) {
        error = _("Failure writing test file");
        return false;
      }
      Glib::ustring line = sharp::file_read_all_text(test_path);
      if(line != test_line) {
        error = _("Failure when checking test file contents");
        return false;
      }

      // Test ability to delete
      if(!test_path->remove()) {
        error = _("Failure when trying to remove test file");
        return false;
      }
    }

    return true;
  }
  catch(Glib::Exception & e) {
    error = e.what();
    return false;
  }
  catch(std::exception & e) {
    error = e.what();
    return false;
  }
  catch(...) {
    error = _("Unknown error");
    return false;
  }
}


void GvfsSyncServiceAddin::reset_configuration()
{
  m_gvfs_settings->set_string(SYNC_GVFS_URI, "");
}


bool GvfsSyncServiceAddin::is_configured()
{
  return m_gvfs_settings->get_string(SYNC_GVFS_URI) != "";
}


Glib::ustring GvfsSyncServiceAddin::name()
{
  char *res = _("Online Folder");
  return res ? res : "";
}


Glib::ustring GvfsSyncServiceAddin::id()
{
  return "gvfs";
}


bool GvfsSyncServiceAddin::is_supported()
{
  return true;
}


bool GvfsSyncServiceAddin::initialized()
{
  return m_initialized && m_enabled;
}


bool GvfsSyncServiceAddin::get_config_settings(Glib::ustring & sync_path)
{
  sync_path = m_gvfs_settings->get_string(SYNC_GVFS_URI);

  return sync_path != "";
}

}
