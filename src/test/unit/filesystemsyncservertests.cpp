/*
 * gnote
 *
 * Copyright (C) 2026 Aurimas Cernius
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
#include <UnitTest++/UnitTest++.h>

#include "testutils.hpp"
#include "synchronization/filesystemsyncserver.hpp"


SUITE(FileSystemSyncServerTests)
{
  struct Fixture
  {
    Glib::ustring sync_path;
    gnote::sync::FileSystemSyncServer server;

    Fixture()
      : sync_path(test::make_temp_dir())
      , server(Gio::File::create_for_path(sync_path), "test")
    {
    }
  };

  TEST_FIXTURE(Fixture, get_all_note_uuids_empty_dir)
  {
    auto note_uids = server.get_all_note_uuids();
    CHECK_EQUAL(0, note_uids.size());
  }
}

