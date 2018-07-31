/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017-2018 Aurimas Cernius
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


#include <stdexcept>

#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>
#include <gtkmm/label.h>
#include <gtkmm/table.h>

#include "debug.hpp"
#include "filesystemsyncserviceaddin.hpp"
#include "preferences.hpp"
#include "sharp/directory.hpp"
#include "sharp/files.hpp"
#include "synchronization/filesystemsyncserver.hpp"


namespace filesystemsyncservice {

FileSystemSyncServiceModule::FileSystemSyncServiceModule()
{
  ADD_INTERFACE_IMPL(FileSystemSyncServiceAddin);
}




FileSystemSyncServiceAddin::FileSystemSyncServiceAddin()
  : m_initialized(false)
  , m_enabled(false)
{
}

void FileSystemSyncServiceAddin::initialize()
{
  m_initialized = true;
  m_enabled = true;
}

void FileSystemSyncServiceAddin::shutdown()
{
  m_enabled = false;
}

gnote::sync::SyncServer::Ptr FileSystemSyncServiceAddin::create_sync_server()
{
  gnote::sync::SyncServer::Ptr server;

  Glib::ustring syncPath;
  if(get_config_settings(syncPath)) {
    m_path = syncPath;
    if(sharp::directory_exists(m_path) == false) {
      sharp::directory_create(m_path);
    }

    auto path = Gio::File::create_for_path(m_path);
    server = gnote::sync::FileSystemSyncServer::create(path);
  }
  else {
    throw std::logic_error("FileSystemSyncServiceAddin.create_sync_server() called without being configured");
  }

  return server;
}


void FileSystemSyncServiceAddin::post_sync_cleanup()
{
  // Nothing to do
}


Gtk::Widget *FileSystemSyncServiceAddin::create_preferences_control(EventHandler requiredPrefChanged)
{
  Gtk::Table *table = new Gtk::Table(1, 2, false);
  table->set_row_spacings(5);
  table->set_col_spacings(10);

  // Read settings out of gconf
  Glib::ustring syncPath;
  if(get_config_settings(syncPath) == false) {
    syncPath = "";
  }

  Gtk::Label *l = new Gtk::Label(_("_Folder Path:"), true);
  l->property_xalign() = 1;
  table->attach(*l, 0, 1, 0, 1,
                Gtk::FILL,
                Gtk::EXPAND | Gtk::FILL,
                0, 0);

  m_path_button = new Gtk::FileChooserButton(_("Select Synchronization Folder..."),
    Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
  m_path_button->signal_current_folder_changed().connect(requiredPrefChanged);
  l->set_mnemonic_widget(*m_path_button);
  m_path_button->set_filename(syncPath);

  table->attach(*m_path_button, 1, 2, 0, 1,
                Gtk::EXPAND | Gtk::FILL,
                Gtk::EXPAND | Gtk::FILL,
                0, 0);

  table->set_hexpand(true);
  table->set_vexpand(false);
  table->show_all();
  return table;
}


bool FileSystemSyncServiceAddin::save_configuration()
{
  Glib::ustring syncPath = m_path_button->get_filename();

  if(syncPath == "") {
    // TODO: Figure out a way to send the error back to the client
    DBG_OUT("The path is empty");
    throw gnote::sync::GnoteSyncException(_("Folder path field is empty."));
  }

  // Attempt to create the path and fail if we can't
  if(sharp::directory_exists(syncPath) == false) {
    if(!sharp::directory_create(syncPath)) {
      DBG_OUT("Could not create \"%s\"", syncPath.c_str());
      throw gnote::sync::GnoteSyncException(_("Specified folder path does not exist, and Gnote was unable to create it."));
    }
  }
  else {
    // Test creating/writing/deleting a file
    // FIXME: Should throw gnote::sync::GnoteSyncException once string changes are OK again
    Glib::ustring testPathBase = Glib::build_filename(syncPath, "test");
    Glib::ustring testPath = testPathBase;
    int count = 0;

    // Get unique new file name
    while(sharp::file_exists(testPath)) {
      testPath = testPathBase + TO_STRING(++count);
    }

    // Test ability to create and write
    Glib::ustring testLine = "Testing write capabilities.";
    sharp::file_write_all_text(testPath, testLine);

    // Test ability to read
    bool testFileFound = false;
    std::list<Glib::ustring> files;
    sharp::directory_get_files(syncPath, files);
    for(auto file : files) {
      if(file == testPath) {
        testFileFound = true;
        break;
      }
    }
    if(!testFileFound) {
      throw sharp::Exception("Failure writing test file");
    }
    Glib::ustring line = sharp::file_read_all_text(testPath);
    if(line != testLine) {
      throw sharp::Exception("Failure when checking test file contents");
    }

    // Test ability to delete
    sharp::file_delete(testPath);
  }

  m_path = syncPath;

  // TODO: Try to create and delete a file.  If it fails, this should fail
  gnote::Preferences::obj().get_schema_settings(
    gnote::Preferences::SCHEMA_SYNC)->set_string(gnote::Preferences::SYNC_LOCAL_PATH, m_path);

  return true;
}


void FileSystemSyncServiceAddin::reset_configuration()
{
  gnote::Preferences::obj().get_schema_settings(
    gnote::Preferences::SCHEMA_SYNC)->set_string(gnote::Preferences::SYNC_LOCAL_PATH, "");
}


bool FileSystemSyncServiceAddin::is_configured()
{
  return gnote::Preferences::obj().get_schema_settings(
    gnote::Preferences::SCHEMA_SYNC)->get_string(gnote::Preferences::SYNC_LOCAL_PATH) != "";
}


Glib::ustring FileSystemSyncServiceAddin::name()
{
  char *res = _("Local Folder");
  return res ? res : "";
}


Glib::ustring FileSystemSyncServiceAddin::id()
{
  return "local";
}


bool FileSystemSyncServiceAddin::is_supported()
{
  return true;
}


bool FileSystemSyncServiceAddin::initialized()
{
  return m_initialized && m_enabled;
}


bool FileSystemSyncServiceAddin::get_config_settings(Glib::ustring & syncPath)
{
  syncPath = gnote::Preferences::obj().get_schema_settings(
    gnote::Preferences::SCHEMA_SYNC)->get_string(gnote::Preferences::SYNC_LOCAL_PATH);

  return syncPath != "";
}

}
