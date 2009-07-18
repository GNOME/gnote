/*
 * gnote
 *
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

#include <dbus-c++/glib-integration.h>

#include "dbus/remotecontrol.hpp"
#include "remotecontrolproxy.hpp"


namespace gnote {

const char *RemoteControlProxy::GNOTE_SERVER_NAME = "org.gnome.Gnote";
const char *RemoteControlProxy::GNOTE_SERVER_PATH = "/org/gnome/Gnote/RemoteControl";

IRemoteControl *RemoteControlProxy::get_instance()
{
  return NULL;
}


DBus::Glib::BusDispatcher dispatcher;

RemoteControl *RemoteControlProxy::register_remote(NoteManager & manager)
{
  DBus::default_dispatcher = &dispatcher;
	dispatcher.attach(NULL);

	DBus::Connection conn = DBus::Connection::SessionBus();
	conn.request_name(GNOTE_SERVER_NAME);

  RemoteControl *remote_control = new RemoteControl (conn, manager);

  return remote_control;
}


}

