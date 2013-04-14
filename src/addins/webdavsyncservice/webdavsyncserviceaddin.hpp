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


#ifndef _WEV_DAV_SYNC_SERVICE_ADDIN_
#define _WEV_DAV_SYNC_SERVICE_ADDIN_


#include <gtkmm/entry.h>
#include <gtkmm/table.h>

#include "sharp/dynamicmodule.hpp"
#include "synchronization/fusesyncserviceaddin.hpp"


namespace webdavsyncserviceaddin {

class WebDavSyncServiceModule
  : public sharp::DynamicModule
{
public:
  WebDavSyncServiceModule();
};


DECLARE_MODULE(WebDavSyncServiceModule)


class WebDavSyncServiceAddin
  : public gnote::sync::FuseSyncServiceAddin
{
public:
  static WebDavSyncServiceAddin * create();

  /// <summary>
  /// Creates a Gtk.Widget that's used to configure the service.  This
  /// will be used in the Synchronization Preferences.  Preferences should
  /// not automatically be saved by a GConf Property Editor.  Preferences
  /// should be saved when SaveConfiguration () is called.
  /// </summary>
  virtual Gtk::Widget *create_preferences_control(EventHandler requiredPrefChanged);

  /// <summary>
  /// Returns whether the addin is configured enough to actually be used.
  /// </summary>
  virtual bool is_configured();

  /// <summary>
  /// Returns true if required settings are non-empty in the preferences widget
  /// </summary>
  virtual bool are_settings_valid();

  /// The name that will be shown in the preferences to distinguish
  /// between this and other SyncServiceAddins.
  /// </summary>
  virtual std::string name();

  /// <summary>
  /// Specifies a unique identifier for this addin.  This will be used to
  /// set the service in preferences.
  /// </summary>
  virtual std::string id();

  virtual std::string fuse_mount_directory_error();
protected:
  virtual std::vector<std::string> get_fuse_mount_exe_args(const std::string & mountPath, bool fromStoredValues);
  virtual std::string get_fuse_mount_exe_args_for_display(const std::string & mountPath, bool fromStoredValues);
  virtual std::string fuse_mount_exe_name();
  virtual bool verify_configuration();
  virtual void save_configuration_values();

  /// <summary>
  /// Reset the configuration so that IsConfigured will return false.
  /// </summary>
  virtual void reset_configuration_values();
private:
  std::vector<std::string> get_fuse_mount_exe_args(const std::string & mountPath, const std::string & url,
      const std::string & username, const std::string & password, bool acceptSsl);
  bool get_config_settings(std::string & url, std::string & username, std::string & password);
  void save_config_settings(const std::string & url, const std::string & username, const std::string & password);
  bool get_pref_widget_settings(std::string & url, std::string & username, std::string & password);
  bool accept_ssl_cert();
  void add_row(Gtk::Table *table, Gtk::Widget *widget, const std::string & labelText, uint row);

  Gtk::Entry *m_url_entry;
  Gtk::Entry *m_username_entry;
  Gtk::Entry *m_password_entry;

  static const char *KEYRING_ITEM_NAME;
  static std::map<std::string, std::string> s_request_attributes;
};

}

#endif

