/*
 * gnote
 *
 * Copyright (C) 2017,2019,2022-2023 Aurimas Cernius
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


#ifndef _TESTSYNCADDIN_HPP_
#define _TESTSYNCADDIN_HPP_


#include "synchronization/syncserviceaddin.hpp"


namespace test {

class SyncAddin
  : public gnote::sync::SyncServiceAddin
{
public:
  SyncAddin(const Glib::ustring & sync_path);
  virtual gnote::sync::SyncServer *create_sync_server() override;
  virtual void post_sync_cleanup() override;
  virtual Gtk::Widget *create_preferences_control(Gtk::Window & parent, EventHandler requiredPrefChanged) override;
  virtual bool save_configuration(const sigc::slot<void(bool, Glib::ustring)> & on_saved) override;
  virtual void reset_configuration() override;
  virtual bool is_configured() const override;
  virtual Glib::ustring name() const override;
  virtual Glib::ustring id() const override;
  virtual bool is_supported() const override;
  virtual void initialize() override;
  virtual void shutdown() override;
  virtual bool initialized() override;
private:
  Glib::ustring m_sync_path;
};

}


#endif

