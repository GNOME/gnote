/*
 * gnote
 *
 * Copyright (C) 2019-2021,2023 Aurimas Cernius
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

#include <giomm/settings.h>

#include "sharp/dynamicmodule.hpp"
#include "synchronization/gvfssyncservice.hpp"



namespace gvfssyncservice {

class GvfsSyncServiceModule
  : public sharp::DynamicModule
{
public:
  GvfsSyncServiceModule();
};


DECLARE_MODULE(GvfsSyncServiceModule)


class GvfsSyncServiceAddin
  : public gnote::sync::GvfsSyncService
{
public:
  static GvfsSyncServiceAddin *create()
    {
      return new GvfsSyncServiceAddin;
    }
  GvfsSyncServiceAddin();

  virtual void initialize() override;

  virtual gnote::sync::SyncServer *create_sync_server() override;
  virtual Gtk::Widget *create_preferences_control(Gtk::Window & parent, EventHandler requiredPrefChanged) override;
  virtual bool save_configuration(const sigc::slot<void(bool, Glib::ustring)> & on_saved) override;
  virtual void reset_configuration() override;
  virtual bool is_configured() const override;
  virtual Glib::ustring name() const override;
  virtual Glib::ustring id() const override;
private:
  static Glib::RefPtr<Gio::File> get_root_dir(const Glib::RefPtr<Gio::File> &);
  bool get_config_settings(Glib::ustring & sync_path);

  Glib::RefPtr<Gio::Settings> m_gvfs_settings;
  Gtk::Entry *m_uri_entry;
};

}

#endif
