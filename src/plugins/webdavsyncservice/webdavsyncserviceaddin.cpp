/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017,2019-2023 Aurimas Cernius
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
#include <gtkmm/label.h>

#include "debug.hpp"
#include "ignote.hpp"
#include "preferences.hpp"
#include "webdavsyncserviceaddin.hpp"
#include "gnome_keyring/keyringexception.hpp"
#include "gnome_keyring/ring.hpp"
#include "sharp/directory.hpp"
#include "sharp/string.hpp"
#include "synchronization/isyncmanager.hpp"
#include "synchronization/filesystemsyncserver.hpp"


using gnome::keyring::KeyringException;
using gnome::keyring::Ring;
using gnote::Preferences;


namespace webdavsyncserviceaddin {

WebDavSyncServiceModule::WebDavSyncServiceModule()
{
  ADD_INTERFACE_IMPL(WebDavSyncServiceAddin);
}


class WebDavSyncServer
  : public gnote::sync::FileSystemSyncServer
{
public:
  static WebDavSyncServer *create(Glib::RefPtr<Gio::File> && path, Preferences & prefs)
    {
      return new WebDavSyncServer(std::move(path), prefs.sync_client_id());
    }
protected:
  void mkdir_p(const Glib::RefPtr<Gio::File> & path) override
    {
      // in WebDAV creating directory fails, if parent doesn't exist, can't create entire path
      if(sharp::directory_exists(path) == false) {
        auto parent = path->get_parent();
        if(parent) {
          mkdir_p(parent);
        }
        sharp::directory_create(path);
      }
    }
private:
  WebDavSyncServer(Glib::RefPtr<Gio::File> && local_sync_path, const Glib::ustring & client_id)
    : gnote::sync::FileSystemSyncServer(std::move(local_sync_path), client_id)
  {}
};


const char *WebDavSyncServiceAddin::KEYRING_ITEM_NAME = "Tomboy sync WebDAV account";
std::map<Glib::ustring, Glib::ustring> WebDavSyncServiceAddin::s_request_attributes;


WebDavSyncServiceAddin::WebDavSyncServiceAddin()
  : m_url_entry(nullptr)
  , m_username_entry(nullptr)
  , m_password_entry(nullptr)
{
}

WebDavSyncServiceAddin * WebDavSyncServiceAddin::create()
{
  s_request_attributes["name"] = KEYRING_ITEM_NAME;
  return new WebDavSyncServiceAddin;
}

Gtk::Widget *WebDavSyncServiceAddin::create_preferences_control(Gtk::Window &, EventHandler requiredPrefChanged)
{
  auto table = Gtk::make_managed<Gtk::Grid>();
  table->set_row_spacing(5);
  table->set_column_spacing(10);

  // Read settings out of gconf
  Glib::ustring url, username, password;
  get_config_settings(url, username, password);

  m_url_entry = Gtk::make_managed<Gtk::Entry>();
  m_url_entry->set_text(url);
  m_url_entry->signal_changed().connect(requiredPrefChanged);
  add_row(table, m_url_entry, _("_URL:"), 0);

  m_username_entry = Gtk::make_managed<Gtk::Entry>();
  m_username_entry->set_text(username);
  m_username_entry->signal_changed().connect(requiredPrefChanged);
  add_row(table, m_username_entry, _("User_name:"), 1);

  m_password_entry = Gtk::make_managed<Gtk::Entry>();
  m_password_entry->set_text(password);
  m_password_entry->set_visibility(false);
  m_password_entry->signal_changed().connect(requiredPrefChanged);
  add_row(table, m_password_entry, _("_Password:"), 2);

  table->set_hexpand(true);
  table->set_vexpand(false);
  return table;
}

bool WebDavSyncServiceAddin::is_configured() const
{
  Glib::ustring url, username, password;
  return get_config_settings(url, username, password);
}


bool WebDavSyncServiceAddin::are_settings_valid() const
{
  Glib::ustring url, username, password;
  return get_pref_widget_settings(url, username, password);
}


Glib::ustring WebDavSyncServiceAddin::name() const
{
  const char *res = _("WebDAV");
  return res;
}


Glib::ustring WebDavSyncServiceAddin::id() const
{
  return "wdfs";
}

gnote::sync::SyncServer *WebDavSyncServiceAddin::create_sync_server()
{
  gnote::sync::SyncServer *server;

  Glib::ustring sync_uri, username, password;
  if(get_config_settings(sync_uri, username, password)) {
    m_uri = sync_uri;

    auto path = Gio::File::create_for_uri(m_uri);
    if(!mount_sync(path, create_mount_operation(username, password))) {
      throw sharp::Exception(_("Failed to mount the folder"));
    }
    if(!path->query_exists())
      throw sharp::Exception(Glib::ustring::format(_("Synchronization destination %1 doesn't exist!"), sync_uri));

    server = WebDavSyncServer::create(std::move(path), ignote().preferences());
  }
  else {
    throw std::logic_error("GvfsSyncServiceAddin.create_sync_server() called without being configured");
  }

  return server;
}

bool WebDavSyncServiceAddin::save_configuration(const sigc::slot<void(bool, Glib::ustring)> & on_saved)
{
  Glib::ustring url, username, password;
  if(!get_pref_widget_settings(url, username, password)) {
    throw gnote::sync::GnoteSyncException(_("URL, username, or password field is empty."));
  }

  auto path = Gio::File::create_for_uri(url);
  auto on_mount_completed = [this, path, url, username, password, on_saved](bool success, Glib::ustring error) {
      if(success) {
        success = test_sync_directory(path, url, error);
      }
      unmount_async([this, url, username, password, on_saved, success, error] {
        if(success) {
          m_uri = url;
          save_config_settings(url, username, password);
        }
        on_saved(success, error);
      });
  };
  auto operation = create_mount_operation(username, password);
  if(mount_async(path, on_mount_completed, operation)) {
    std::thread thread([this, url, on_mount_completed]() {
      on_mount_completed(true, "");
    });
    thread.detach();
  }

  return true;
}

Glib::RefPtr<Gio::MountOperation> WebDavSyncServiceAddin::create_mount_operation(const Glib::ustring & username, const Glib::ustring & password)
{
  auto operation = Gio::MountOperation::create();
  operation->signal_ask_password().connect(
    [operation, username, password](const Glib::ustring & /* message */, const Glib::ustring & /* default_user */, const Glib::ustring & /* default_domain */, Gio::AskPasswordFlags flags) {
      if(Gio::AskPasswordFlags::NEED_DOMAIN == (flags & Gio::AskPasswordFlags::NEED_DOMAIN)) {
        operation->reply(Gio::MountOperationResult::ABORTED);
        return;
      }

      if(Gio::AskPasswordFlags::NEED_USERNAME == (flags & Gio::AskPasswordFlags::NEED_USERNAME)) {
        operation->set_username(username);
      }
      if(Gio::AskPasswordFlags::NEED_PASSWORD == (flags & Gio::AskPasswordFlags::NEED_PASSWORD)) {
        operation->set_password(password);
      }

      operation->reply(Gio::MountOperationResult::HANDLED);
    });
  return operation;
}

void WebDavSyncServiceAddin::reset_configuration()
{
  save_config_settings("", "", "");
}

bool WebDavSyncServiceAddin::get_config_settings(Glib::ustring & url, Glib::ustring & username, Glib::ustring & password) const
{
  // Retrieve configuration from the GNOME Keyring and GSettings
  url = "";
  username = "";
  password = "";

  try {
    password = sharp::string_trim(Ring::find_password(s_request_attributes));
    if(password != "") {
      username = sharp::string_trim(ignote().preferences().sync_fuse_wdfs_username());
      url = sharp::string_trim(ignote().preferences().sync_fuse_wdfs_url());
    }
  }
  catch(KeyringException & ke) {
    DBG_OUT("Getting configuration from the GNOME keyring failed with the following message: %s", ke.what());
  }

  return url != "" && username != "" && password != "";
}

void WebDavSyncServiceAddin::save_config_settings(const Glib::ustring & url, const Glib::ustring & username, const Glib::ustring & password)
{
  // Save configuration into the GNOME Keyring and GSettings
  try {
    ignote().preferences().sync_fuse_wdfs_username(username);
    ignote().preferences().sync_fuse_wdfs_url(url);

    if(password != "") {
      Ring::create_password(Ring::default_keyring(), KEYRING_ITEM_NAME, s_request_attributes, password);
    }
    else {
      Ring::clear_password(s_request_attributes);
    }
  }
  catch(KeyringException & ke) {
    DBG_OUT("Saving configuration to the GNOME keyring failed with the following message: %s", ke.what());
    // TODO: If the above fails (no keyring daemon), save all but password
    //       to GConf, and notify user.
    // Save configuration into GConf
    //Preferences.Set ("/apps/tomboy/sync_wdfs_url", url ?? string.Empty);
    //Preferences.Set ("/apps/tomboy/sync_wdfs_username", username ?? string.Empty);

    Glib::ustring msg = Glib::ustring::compose(
      // TRANSLATORS: %1 is the format placeholder for the error message.
      _("Saving configuration to the GNOME keyring failed with the following message:\n\n%1"),
      ke.what());
    throw gnote::sync::GnoteSyncException(msg.c_str());
  }
}

bool WebDavSyncServiceAddin::get_pref_widget_settings(Glib::ustring & url, Glib::ustring & username, Glib::ustring & password) const
{
  url = sharp::string_trim(m_url_entry->get_text());
  username = sharp::string_trim(m_username_entry->get_text());
  password = sharp::string_trim(m_password_entry->get_text());

  return url != "" && username != "" && password != "";
}

bool WebDavSyncServiceAddin::accept_ssl_cert()
{
  try {
    return ignote().preferences().sync_fuse_wdfs_accept_sllcert();
  }
  catch(...) {
    return false;
  }
}

void WebDavSyncServiceAddin::add_row(Gtk::Grid *table, Gtk::Widget *widget, const Glib::ustring & labelText, uint row)
{
  auto l = Gtk::make_managed<Gtk::Label>(labelText, true);
  l->property_xalign() = 0.0f;
  table->attach(*l, 0, row);

  table->attach(*widget, 1, row);

  l->set_mnemonic_widget(*widget);

  // TODO: Tooltips
}

}

