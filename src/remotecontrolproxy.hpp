/*
 * gnote
 *
 * Copyright (C) 2011,2013,2019,2021 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
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


#ifndef _REMOTECONTROL_PROXY_HPP_
#define _REMOTECONTROL_PROXY_HPP_

#include <giomm/dbusconnection.h>
#include <giomm/dbusintrospection.h>

namespace org {
namespace gnome {
namespace Gnote {
  class SearchProvider;
}
}
}

namespace gnote {

class IGnote;
class RemoteControl;
class NoteManagerBase;

class RemoteControlProxy 
{
public:
  static const char *GNOTE_SERVER_PATH;
  static const char *GNOTE_INTERFACE_NAME;
  static const char *GNOTE_SERVER_NAME;
  static const char *GNOTE_SEARCH_PROVIDER_PATH;
  static const char *GNOTE_SEARCH_PROVIDER_INTERFACE_NAME;

  typedef sigc::slot<void, bool, bool> slot_name_acquire_finish;
  typedef sigc::slot<void> slot_connected;

  explicit RemoteControlProxy(IGnote & g);

  RemoteControl *get_remote_control();
  void register_remote(NoteManagerBase & manager, const slot_name_acquire_finish & on_finish);
  void register_object(const Glib::RefPtr<Gio::DBus::Connection> & conn, NoteManagerBase & manager,
                       const slot_name_acquire_finish & on_finish);
private:
  void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> & conn, const Glib::ustring & name);
  void on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection> & conn, const Glib::ustring & name);
  void on_name_lost(const Glib::RefPtr<Gio::DBus::Connection> & conn, const Glib::ustring & name);
  void load_introspection_xml();

  IGnote & m_gnote;
  NoteManagerBase *m_manager;
  RemoteControl *m_remote_control;
  org::gnome::Gnote::SearchProvider *m_search_provider;
  bool m_bus_acquired;
  Glib::RefPtr<Gio::DBus::Connection> m_connection;
  Glib::RefPtr<Gio::DBus::InterfaceInfo> m_gnote_interface;
  Glib::RefPtr<Gio::DBus::InterfaceInfo> m_search_provider_interface;
  slot_name_acquire_finish m_on_name_acquire_finish;
};

}

#endif
