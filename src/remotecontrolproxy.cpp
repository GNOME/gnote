/*
 * gnote
 *
 * Copyright (C) 2011,2013,2017,2019,2021-2022 Aurimas Cernius
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

#include <glibmm/i18n.h>
#include <giomm/dbusownname.h>


#include "config.h"
#include "debug.hpp"
#include "dbus/remotecontrol.hpp"
#include "dbus/searchprovider.hpp"
#include "remotecontrolproxy.hpp"
#include "sharp/files.hpp"


namespace {
  void load_interface_from_file(const char *filename, const char *interface_name,
                                Glib::RefPtr<Gio::DBus::InterfaceInfo> & interface)
  {
    if(interface) {
      return;
    }
    try {
      Glib::ustring introspect_xml = sharp::file_read_all_text(filename);
      Glib::RefPtr<Gio::DBus::NodeInfo> node = Gio::DBus::NodeInfo::create_for_xml(introspect_xml);
      interface = node->lookup_interface(interface_name);
    }
    catch(Glib::Error & e) {
      ERR_OUT(_("Failed to load D-Bus interface %s: %s"), interface_name, e.what());
    }
    catch(sharp::Exception & e) {
      ERR_OUT(_("Failed to load D-Bus interface %s: %s"), interface_name, e.what());
    }
  }
}

namespace gnote {

const char *RemoteControlProxy::GNOTE_SERVER_NAME = "org.gnome.Gnote";
const char *RemoteControlProxy::GNOTE_INTERFACE_NAME = "org.gnome.Gnote.RemoteControl";
const char *RemoteControlProxy::GNOTE_SERVER_PATH = "/org/gnome/Gnote/RemoteControl";
const char *RemoteControlProxy::GNOTE_SEARCH_PROVIDER_PATH = "/org/gnome/Gnote/SearchProvider";
const char *RemoteControlProxy::GNOTE_SEARCH_PROVIDER_INTERFACE_NAME = "org.gnome.Shell.SearchProvider2";


RemoteControlProxy::RemoteControlProxy()
  : m_remote_control(NULL)
  , m_search_provider(NULL)
{
}

RemoteControl *RemoteControlProxy::get_remote_control()
{
  return m_remote_control;
}


void RemoteControlProxy::register_object(const Glib::RefPtr<Gio::DBus::Connection> & conn, IGnote & g, NoteManagerBase & manager)
{
  load_introspection_xml();
  m_remote_control = new RemoteControl(conn, g, manager, GNOTE_SERVER_PATH, GNOTE_INTERFACE_NAME, m_gnote_interface);
  m_search_provider = new org::gnome::Gnote::SearchProvider(conn, GNOTE_SEARCH_PROVIDER_PATH,
                                                            m_search_provider_interface, g, manager);
  DBG_OUT("Gnote remote control active.");
}


void RemoteControlProxy::load_introspection_xml()
{
  load_interface_from_file(DATADIR"/gnote/gnote-introspect.xml", GNOTE_INTERFACE_NAME, m_gnote_interface);
  load_interface_from_file(DATADIR"/gnote/shell-search-provider-dbus-interfaces.xml",
                           GNOTE_SEARCH_PROVIDER_INTERFACE_NAME, m_search_provider_interface);
}

}

