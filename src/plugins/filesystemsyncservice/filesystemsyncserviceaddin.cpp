/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017-2023,2025 Aurimas Cernius
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
#include <gtkmm/filechoosernative.h>
#include <gtkmm/label.h>

#include "debug.hpp"
#include "filesystemsyncserviceaddin.hpp"
#include "ignote.hpp"
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

gnote::sync::SyncServer *FileSystemSyncServiceAddin::create_sync_server()
{
  gnote::sync::SyncServer *server;

  Glib::ustring syncPath;
  if(get_config_settings(syncPath)) {
    m_path = syncPath;
    if(sharp::directory_exists(m_path) == false) {
      sharp::directory_create(m_path);
    }

    auto path = Gio::File::create_for_path(m_path);
    server = gnote::sync::FileSystemSyncServer::create(std::move(path), ignote().preferences());
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


Gtk::Widget *FileSystemSyncServiceAddin::create_preferences_control(Gtk::Window & parent, EventHandler requiredPrefChanged)
{
  auto table = Gtk::make_managed<Gtk::Grid>();
  table->set_row_spacing(5);
  table->set_column_spacing(10);

  // Read settings out of gconf
  Glib::ustring syncPath;
  if(get_config_settings(syncPath) == false) {
    syncPath = "";
  }

  auto l = Gtk::make_managed<Gtk::Label>(_("_Folder Path:"), true);
  l->property_xalign() = 1;
  table->attach(*l, 0, 0, 1, 1);

  m_path_button = Gtk::make_managed<Gtk::Button>();
  if(syncPath.empty()) {
    m_path_button->set_label(_("Select Synchronization Folder..."));
    m_path_button->set_use_underline(true);
  }
  else {
    m_path_button->set_label(syncPath);
    m_path_button->set_use_underline(false);
  }
  m_path_button->signal_clicked().connect([this, &parent, requiredPrefChanged] {
    auto dlg = Gtk::FileChooserNative::create(_("Select Synchronization Folder..."), Gtk::FileChooser::Action::SELECT_FOLDER);
    dlg->set_modal(true);
    dlg->set_transient_for(parent);
    Glib::ustring syncPath;
    if(get_config_settings(syncPath)) {
      dlg->set_file(Gio::File::create_for_path(syncPath));
    }
    dlg->signal_response().connect([this, dlg, requiredPrefChanged](int resp) {
      dlg->hide();
      if(resp != Gtk::ResponseType::ACCEPT) {
        return;
      }

      auto selected = dlg->get_file();
      m_path_button->set_label(selected->get_path());
      requiredPrefChanged();
    });
    dlg->show(); // TODO parent
  });
  l->set_mnemonic_widget(*m_path_button);

  table->attach(*m_path_button, 1, 0, 1, 1);

  table->set_hexpand(true);
  table->set_vexpand(false);
  return table;
}


bool FileSystemSyncServiceAddin::save_configuration(const sigc::slot<void(bool, Glib::ustring)> & on_saved)
{
  Glib::ustring syncPath = m_path_button->get_label();

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
    std::vector<Glib::ustring> files = sharp::directory_get_files(syncPath);
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
  ignote().preferences().sync_local_path(m_path);

  on_saved(true, "");
  return true;
}


void FileSystemSyncServiceAddin::reset_configuration()
{
  ignote().preferences().sync_local_path("");
}


bool FileSystemSyncServiceAddin::is_configured() const
{
  return ignote().preferences().sync_local_path() != "";
}


Glib::ustring FileSystemSyncServiceAddin::name() const
{
  char *res = _("Local Folder");
  return res ? res : "";
}


Glib::ustring FileSystemSyncServiceAddin::id() const
{
  return "local";
}


bool FileSystemSyncServiceAddin::is_supported() const
{
  return true;
}


bool FileSystemSyncServiceAddin::initialized()
{
  return m_initialized && m_enabled;
}


bool FileSystemSyncServiceAddin::get_config_settings(Glib::ustring & syncPath)
{
  syncPath = ignote().preferences().sync_local_path();
  return syncPath != "";
}

}
