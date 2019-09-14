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


#ifndef _GVFS_SYNC_SERVICE_ADDIN_
#define _GVFS_SYNC_SERVICE_ADDIN_

#include "sharp/dynamicmodule.hpp"
#include "synchronization/syncserviceaddin.hpp"



namespace gvfssyncservice {

class GvfsSyncServiceModule
  : public sharp::DynamicModule
{
public:
  GvfsSyncServiceModule();
};


DECLARE_MODULE(GvfsSyncServiceModule)


class GvfsSyncServiceAddin
  : public gnote::sync::SyncServiceAddin
{
public:
  static GvfsSyncServiceAddin *create()
    {
      return new GvfsSyncServiceAddin;
    }
  GvfsSyncServiceAddin();

  virtual void initialize() override;
  virtual void shutdown() override;

  virtual gnote::sync::SyncServer::Ptr create_sync_server() override;
  virtual void post_sync_cleanup() override;
  virtual Gtk::Widget *create_preferences_control(EventHandler requiredPrefChanged) override;
  virtual bool save_configuration(const sigc::slot<void, bool, Glib::ustring> & on_saved) override;
  virtual void reset_configuration() override;
  virtual bool is_configured() override;
  virtual Glib::ustring name() override;
  virtual Glib::ustring id() override;
  virtual bool is_supported() override;
  virtual bool initialized() override;
private:
  bool get_config_settings(Glib::ustring & sync_path);
  bool mount(const Glib::RefPtr<Gio::File> & path);
  bool mount_async(const Glib::RefPtr<Gio::File> & path, const sigc::slot<void, bool, Glib::ustring> & completed);
  void unmount();
  void unmount_async(const sigc::slot<void> & completed);
  bool test_sync_directory(const Glib::RefPtr<Gio::File> & path, const Glib::ustring & sync_uri, Glib::ustring & error);

  Glib::ustring m_uri;
  Gtk::Entry *m_uri_entry;
  bool m_initialized;
  bool m_enabled;
  Glib::RefPtr<Gio::Mount> m_mount;
};

}

#endif
