/*
 * gnote
 *
 * Copyright (C) 2019-2023 Aurimas Cernius
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


#include <thread>

#include <glibmm/i18n.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>

#include "debug.hpp"
#include "gvfssyncserviceaddin.hpp"
#include "ignote.hpp"
#include "sharp/directory.hpp"
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
{
}

void GvfsSyncServiceAddin::initialize()
{
  GvfsSyncService::initialize();
  if(!m_gvfs_settings) {
    m_gvfs_settings = Gio::Settings::create(SCHEMA_SYNC_GVFS);
  }
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
    auto root = get_root_dir(path);
    if(!mount_sync(root)) {
      throw sharp::Exception(_("Failed to mount the folder"));
    }
    if(!path->query_exists())
      sharp::directory_create(path);

    server = gnote::sync::FileSystemSyncServer::create(std::move(path), ignote().preferences());
  }
  else {
    throw std::logic_error("GvfsSyncServiceAddin.create_sync_server() called without being configured");
  }

  return server;
}


Gtk::Widget *GvfsSyncServiceAddin::create_preferences_control(Gtk::Window & parent, EventHandler required_pref_changed)
{
  auto table = Gtk::make_managed<Gtk::Grid>();
  table->set_row_spacing(5);
  table->set_column_spacing(10);

  // Read settings out of gconf
  Glib::ustring sync_path;
  if(get_config_settings(sync_path) == false) {
    sync_path = "";
  }

  auto l = Gtk::make_managed<Gtk::Label>(_("Folder _URI:"), true);
  l->property_xalign() = 1;
  table->attach(*l, 0, 0, 1, 1);

  m_uri_entry = Gtk::make_managed<Gtk::Entry>();
  m_uri_entry->set_text(sync_path);
  m_uri_entry->get_buffer()->signal_inserted_text().connect([required_pref_changed](guint, const gchar*, guint) { required_pref_changed(); });
  m_uri_entry->get_buffer()->signal_deleted_text().connect([required_pref_changed](guint, guint) { required_pref_changed(); });
  l->set_mnemonic_widget(*m_uri_entry);

  table->attach(*m_uri_entry, 1, 0, 1, 1);

  auto example = Gtk::make_managed<Gtk::Label>(_("Example: google-drive://name.surname@gmail.com/notes"));
  example->property_xalign() = 0;
  table->attach(*example, 1, 1, 1, 1);
  auto account_info = Gtk::make_managed<Gtk::Label>(_("Please, register your account in Online Accounts"));
  account_info->property_xalign() = 0;
  table->attach(*account_info, 1, 2, 1, 1);

  table->set_hexpand(true);
  table->set_vexpand(false);
  return table;
}


bool GvfsSyncServiceAddin::save_configuration(const sigc::slot<void(bool, Glib::ustring)> & on_saved)
{
  Glib::ustring sync_uri = m_uri_entry->get_text();

  if(sync_uri == "") {
    ERR_OUT(_("The URI is empty"));
    throw gnote::sync::GnoteSyncException(_("URI field is empty."));
  }

  auto path = Gio::File::create_for_uri(sync_uri);
  auto root = get_root_dir(path);
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
  if(mount_async(root, on_mount_completed)) {
    std::thread thread([this, on_mount_completed]() {
      on_mount_completed(true, "");
    });
    thread.detach();
  }

  return true;
}


void GvfsSyncServiceAddin::reset_configuration()
{
  m_gvfs_settings->set_string(SYNC_GVFS_URI, "");
}


bool GvfsSyncServiceAddin::is_configured() const
{
  return m_gvfs_settings->get_string(SYNC_GVFS_URI) != "";
}


Glib::ustring GvfsSyncServiceAddin::name() const
{
  char *res = _("Online Folder");
  return res ? res : "";
}


Glib::ustring GvfsSyncServiceAddin::id() const
{
  return "gvfs";
}


bool GvfsSyncServiceAddin::get_config_settings(Glib::ustring & sync_path)
{
  sync_path = m_gvfs_settings->get_string(SYNC_GVFS_URI);

  return sync_path != "";
}


Glib::RefPtr<Gio::File> GvfsSyncServiceAddin::get_root_dir(const Glib::RefPtr<Gio::File> & uri)
{
  auto root = uri;
  auto parent = root->get_parent();
  while(parent) {
    root = parent;
    parent = root->get_parent();
  }

  return root;
}


}
