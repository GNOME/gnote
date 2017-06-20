/*
 * gnote
 *
 * Copyright (C) 2017 Aurimas Cernius
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


#include "testsyncaddin.hpp"
#include "synchronization/filesystemsyncserver.hpp"


namespace test {

SyncAddin::SyncAddin(const Glib::ustring & sync_path)
  : m_sync_path(sync_path)
{
}

gnote::sync::SyncServer::Ptr SyncAddin::create_sync_server()
{
  return gnote::sync::FileSystemSyncServer::create(m_sync_path, "test");
}

void SyncAddin::post_sync_cleanup()
{
}

Gtk::Widget *SyncAddin::create_preferences_control(EventHandler /*requiredPrefChanged*/)
{
  return NULL;
}

bool SyncAddin::save_configuration()
{
  return true;
}

void SyncAddin::reset_configuration()
{
}

bool SyncAddin::is_configured()
{
  return true;
}

Glib::ustring SyncAddin::name()
{
  return "test";
}

Glib::ustring SyncAddin::id()
{
  return "test";
}

bool SyncAddin::is_supported()
{
  return true;
}

void SyncAddin::initialize()
{
}

void SyncAddin::shutdown()
{
}

bool SyncAddin::initialized()
{
  return true;
}


}
