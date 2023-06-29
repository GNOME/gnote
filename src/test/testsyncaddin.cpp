/*
 * gnote
 *
 * Copyright (C) 2017-2019,2022-2023 Aurimas Cernius
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


#include <giomm/file.h>

#include "testsyncaddin.hpp"
#include "synchronization/filesystemsyncserver.hpp"


namespace test {

SyncAddin::SyncAddin(const Glib::ustring & sync_path)
  : m_sync_path(sync_path)
{
}

gnote::sync::SyncServer *SyncAddin::create_sync_server()
{
  return new gnote::sync::FileSystemSyncServer(Gio::File::create_for_path(m_sync_path), "test");
}

void SyncAddin::post_sync_cleanup()
{
}

Gtk::Widget *SyncAddin::create_preferences_control(Gtk::Window&, EventHandler /*requiredPrefChanged*/)
{
  return NULL;
}

bool SyncAddin::save_configuration(const sigc::slot<void(bool, Glib::ustring)> & on_saved)
{
  on_saved(true, "");
  return true;
}

void SyncAddin::reset_configuration()
{
}

bool SyncAddin::is_configured() const
{
  return true;
}

Glib::ustring SyncAddin::name() const
{
  return "test";
}

Glib::ustring SyncAddin::id() const
{
  return "test";
}

bool SyncAddin::is_supported() const
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
