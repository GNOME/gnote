/*
 * gnote
 *
 * Copyright (C) 2019 Aurimas Cernius
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

#include "debug.hpp"
#include "gvfssyncserviceaddin.hpp"
#include "preferences.hpp"
#include "sharp/directory.hpp"
#include "sharp/files.hpp"
#include "synchronization/filesystemsyncserver.hpp"


namespace gvfssyncservice {

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
}

void GvfsSyncServiceAddin::shutdown()
{
  m_enabled = false;
}

gnote::sync::SyncServer::Ptr GvfsSyncServiceAddin::create_sync_server()
{
  gnote::sync::SyncServer::Ptr server;

  Glib::ustring sync_uri;
  if(get_config_settings(sync_uri)) {
    m_uri = sync_uri;
    if(sharp::directory_exists(m_uri) == false) {
      sharp::directory_create(m_uri);
    }

    auto path = Gio::File::create_for_uri(m_uri);
    server = gnote::sync::FileSystemSyncServer::create(path);
  }
  else {
    throw std::logic_error("GvfsSyncServiceAddin.create_sync_server() called without being configured");
  }

  return server;
}


void GvfsSyncServiceAddin::post_sync_cleanup()
{
  // Nothing to do
}


Gtk::Widget *GvfsSyncServiceAddin::create_preferences_control(EventHandler required_pref_changed)
{
  Gtk::Table *table = manage(new Gtk::Table(1, 1, false));
  table->set_row_spacings(5);
  table->set_col_spacings(10);

  // Read settings out of gconf
  Glib::ustring sync_path;
  if(get_config_settings(sync_path) == false) {
    sync_path = "";
  }

  auto l = manage(new Gtk::Label(_("_Folder Path:"), true));
  l->property_xalign() = 1;
  table->attach(*l, 0, 1, 0, 1,
                Gtk::FILL,
                Gtk::EXPAND | Gtk::FILL,
                0, 0);

  m_uri_entry = manage(new Gtk::Entry);
  m_uri_entry->get_buffer()->signal_inserted_text().connect([required_pref_changed](guint, const gchar*, guint) { required_pref_changed(); });
  m_uri_entry->get_buffer()->signal_deleted_text().connect([required_pref_changed](guint, guint) { required_pref_changed(); });
  l->set_mnemonic_widget(*m_uri_entry);

  table->attach(*m_uri_entry, 1, 2, 0, 1,
                Gtk::EXPAND | Gtk::FILL,
                Gtk::EXPAND | Gtk::FILL,
                0, 0);

  table->set_hexpand(true);
  table->set_vexpand(false);
  table->show_all();
  return table;
}


bool GvfsSyncServiceAddin::save_configuration()
{
  Glib::ustring sync_uri = m_uri_entry->get_text();

  if(sync_uri == "") {
    ERR_OUT(_("The URI is empty"));
    throw gnote::sync::GnoteSyncException(_("URI field is empty."));
  }

  auto path = Gio::File::create_for_uri(sync_uri);
  if(sharp::directory_exists(path) == false) {
    if(!sharp::directory_create(path)) {
      DBG_OUT("Could not create \"%s\"", sync_uri.c_str());
      throw gnote::sync::GnoteSyncException(_("Specified folder path does not exist, and Gnote was unable to create it."));
    }
  }
  else {
    // Test creating/writing/deleting a file
    Glib::ustring test_path_base = Glib::build_filename(sync_uri, "test");
    Glib::ustring test_path = test_path_base;
    int count = 0;

    // Get unique new file name
    while(sharp::file_exists(test_path)) {
      test_path = test_path_base + TO_STRING(++count);
    }

    // Test ability to create and write
    Glib::ustring test_line = "Testing write capabilities.";
    sharp::file_write_all_text(test_path, test_line);

    // Test ability to read
    bool test_file_found = false;
    std::vector<Glib::RefPtr<Gio::File>> files;
    sharp::directory_get_files(path, files);
    for(auto & file : files) {
      if(file->get_uri() == test_path) {
        test_file_found = true;
        break;
      }
    }
    if(!test_file_found) {
      throw sharp::Exception("Failure writing test file");
    }
    Glib::ustring line = sharp::file_read_all_text(test_path);
    if(line != test_line) {
      throw sharp::Exception("Failure when checking test file contents");
    }

    // Test ability to delete
    sharp::file_delete(test_path);
  }

  m_uri = sync_uri;

  // TODO: Try to create and delete a file.  If it fails, this should fail
  gnote::Preferences::obj().get_schema_settings(
    gnote::Preferences::SCHEMA_SYNC_GVFS)->set_string(gnote::Preferences::SYNC_GVFS_URI, m_uri);

  return true;
}


void GvfsSyncServiceAddin::reset_configuration()
{
  gnote::Preferences::obj().get_schema_settings(
    gnote::Preferences::SCHEMA_SYNC_GVFS)->set_string(gnote::Preferences::SYNC_GVFS_URI, "");
}


bool GvfsSyncServiceAddin::is_configured()
{
  return gnote::Preferences::obj().get_schema_settings(
    gnote::Preferences::SCHEMA_SYNC_GVFS)->get_string(gnote::Preferences::SYNC_GVFS_URI) != "";
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
  sync_path = gnote::Preferences::obj().get_schema_settings(
    gnote::Preferences::SCHEMA_SYNC_GVFS)->get_string(gnote::Preferences::SYNC_GVFS_URI);

  return sync_path != "";
}

}
