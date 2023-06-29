/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017,2019,2021,2023 Aurimas Cernius
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

#include "sharp/dynamicmodule.hpp"
#include "synchronization/gvfssyncservice.hpp"


namespace webdavsyncserviceaddin {

class WebDavSyncServiceModule
  : public sharp::DynamicModule
{
public:
  WebDavSyncServiceModule();
};


DECLARE_MODULE(WebDavSyncServiceModule)


class WebDavSyncServiceAddin
  : public gnote::sync::GvfsSyncService
{
public:
  static WebDavSyncServiceAddin * create();

  WebDavSyncServiceAddin();

  /// <summary>
  /// Creates a Gtk.Widget that's used to configure the service.  This
  /// will be used in the Synchronization Preferences.  Preferences should
  /// not automatically be saved by a GConf Property Editor.  Preferences
  /// should be saved when SaveConfiguration () is called.
  /// </summary>
  virtual Gtk::Widget *create_preferences_control(Gtk::Window &, EventHandler requiredPrefChanged) override;

  /// <summary>
  /// Returns whether the addin is configured enough to actually be used.
  /// </summary>
  virtual bool is_configured() const override;

  /// <summary>
  /// Returns true if required settings are non-empty in the preferences widget
  /// </summary>
  virtual bool are_settings_valid() const override;

  /// The name that will be shown in the preferences to distinguish
  /// between this and other SyncServiceAddins.
  /// </summary>
  virtual Glib::ustring name() const override;

  /// <summary>
  /// Specifies a unique identifier for this addin.  This will be used to
  /// set the service in preferences.
  /// </summary>
  virtual Glib::ustring id() const override;

  virtual gnote::sync::SyncServer *create_sync_server() override;
  virtual bool save_configuration(const sigc::slot<void(bool, Glib::ustring)> & on_saved) override;
  virtual void reset_configuration() override;
private:
  static Glib::RefPtr<Gio::MountOperation> create_mount_operation(const Glib::ustring & username, const Glib::ustring & password);
  bool get_config_settings(Glib::ustring & url, Glib::ustring & username, Glib::ustring & password) const;
  void save_config_settings(const Glib::ustring & url, const Glib::ustring & username, const Glib::ustring & password);
  bool get_pref_widget_settings(Glib::ustring & url, Glib::ustring & username, Glib::ustring & password) const;
  bool accept_ssl_cert();
  void add_row(Gtk::Grid *table, Gtk::Widget *widget, const Glib::ustring & labelText, uint row);

  Gtk::Entry *m_url_entry;
  Gtk::Entry *m_username_entry;
  Gtk::Entry *m_password_entry;

  static const char *KEYRING_ITEM_NAME;
  static std::map<Glib::ustring, Glib::ustring> s_request_attributes;
};

}

#endif

