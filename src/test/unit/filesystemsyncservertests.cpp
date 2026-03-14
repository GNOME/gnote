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


#include <glibmm.h>
#include <giomm/file.h>
#include <UnitTest++/UnitTest++.h>

#include "testutils.hpp"
#include "sharp/files.hpp"
#include "synchronization/filesystemsyncserver.hpp"


SUITE(FileSystemSyncServerTests)
{
  const Glib::ustring valid_manifest =
    "<sync revision=\"2\" server-id=\"0cac27e4-cb54-4d9a-aaaa-28a010f213d3\">"
    "<note id=\"69f26039-67fc-44ae-97c0-fa44aa2bc81c\" rev=\"1\"/>"
    "<note id=\"b97f40cf-1165-4466-8eb9-9d2822ff4819\" rev=\"2\"/>"
    "<note id=\"3f669325-f523-49c2-852d-4ba45f3ed707\" rev=\"2\"/>"
    "</sync>"
    ;

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

  struct FixtureWithManifest
    : Fixture
  {
    explicit FixtureWithManifest(const Glib::ustring &content)
    {
      auto manifest_file = Glib::build_filename(sync_path, "manifest.xml");
      sharp::file_write_all_text(manifest_file, content);
    }
  };

  struct FixtureValidManifest
    : FixtureWithManifest
  {
    FixtureValidManifest()
      : FixtureWithManifest(valid_manifest)
    {}
  };

  struct FixtureInvalidManifest
    : FixtureWithManifest
  {
    FixtureInvalidManifest()
      : FixtureWithManifest("=====")
    {}
  };

  TEST_FIXTURE(Fixture, get_all_note_uuids_empty_dir)
  {
    auto note_uids = server.get_all_note_uuids();
    CHECK_EQUAL(0, note_uids.size());
  }

  TEST_FIXTURE(FixtureValidManifest, get_all_note_uuids_with_proper_manifest)
  {
    auto note_uids = server.get_all_note_uuids();
    CHECK_EQUAL(3, note_uids.size());
    std::sort(note_uids.begin(), note_uids.end());
    CHECK_EQUAL("3f669325-f523-49c2-852d-4ba45f3ed707", note_uids[0]);
    CHECK_EQUAL("69f26039-67fc-44ae-97c0-fa44aa2bc81c", note_uids[1]);
    CHECK_EQUAL("b97f40cf-1165-4466-8eb9-9d2822ff4819", note_uids[2]);
  }

  TEST_FIXTURE(FixtureInvalidManifest, get_all_note_uuids_with_invalid_manifest)
  {
    try {
      auto note_uids = server.get_all_note_uuids();
      CHECK(false); // exception expected
    }
    catch(std::runtime_error &e) {
      // expected
    }
  }

  TEST_FIXTURE(Fixture, latest_revision_empty_dir)
  {
    int revision = server.latest_revision();
    CHECK_EQUAL(-1, revision);
  }

  TEST_FIXTURE(FixtureValidManifest, latest_revision_with_proper_manifest)
  {
    int revision = server.latest_revision();
    CHECK_EQUAL(2, revision);
  }

  TEST_FIXTURE(FixtureInvalidManifest, latest_revision_with_invalid_manifest)
  {
    try {
      server.latest_revision();
      CHECK(false);
    }
    catch(std::runtime_error &e) {
      // expected
    }
  }
}

