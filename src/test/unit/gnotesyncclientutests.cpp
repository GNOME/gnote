/*
 * gnote
 *
 * Copyright (C) 2017,2019-2020,2023 Aurimas Cernius
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


#include <cstdio>

#include <UnitTest++/UnitTest++.h>

#include "test/testgnote.hpp"
#include "test/testnotemanager.hpp"
#include "test/testsyncclient.hpp"


SUITE(GnoteSyncClient)
{

const char *TEST_MANIFEST =
  "<manifest>"
  "  <last-sync-date>2014-04-21T20:13:24.711343Z</last-sync-date>"
  "  <last-sync-rev>0</last-sync-rev>"
  "  <server-id>38afddc2-9ce8-46ba-b106-baf3916c74b8</server-id>"
  "  <note-revisions>"
  "    <note guid=\"0ead2704-4c24-4110-b7da-22d00cae25f3\" latest-revision=\"0\"/>"
  "    <note guid=\"138274ad-5056-4fd0-b7ef-8f27ce9408ce\" latest-revision=\"0\"/>"
  "    <note guid=\"19e1739b-be39-42dc-9060-c7b237c6a080\" latest-revision=\"0\"/>"
  "  </note-revisions>"
  "  <note-deletions>"
  "    <note guid=\"49c4668b-a464-492f-8fc0-1a495735cd24\" title=\"Deleted Note 1\"/>"
  "    <note guid=\"507dd82f-b6f3-40a5-bb0a-0d140673eac2\" title=\"Deleted Note 2\"/>"
  "    <note guid=\"551741e8-0c7d-407b-98b2-f8ef2dfa9c79\" title=\"Deleted Note 3\"/>"
  "  </note-deletions>"
  "</manifest>";


Glib::ustring create_manifest()
{
  char temp_file_name[] = "/tmp/gnotetestXXXXXX";
  int fd = mkstemp(temp_file_name);
  if(fd < 0) {
    std::fputs("Failed to write manifest file", stderr);
    abort();
  }

  Glib::ustring test_manifest = temp_file_name;
  FILE *file = fdopen(fd, "w");
  std::fputs(TEST_MANIFEST, file);
  std::fclose(file);
  return test_manifest;
}


TEST(manifest_parsing)
{
  Glib::ustring test_manifest = create_manifest();

  test::Gnote g;
  test::NoteManager manager(test::NoteManager::test_notes_dir(), g);
  test::SyncClient client(manager);
  client.set_manifest_path(test_manifest);
  client.reparse();

  auto sync_date = sharp::date_time_from_iso8601("2014-04-21T20:13:24.711343Z");
  CHECK(client.last_sync_date().compare(sync_date) == 0);
  CHECK_EQUAL(0, client.last_synchronized_revision());
  CHECK_EQUAL("38afddc2-9ce8-46ba-b106-baf3916c74b8", client.associated_server_id());
  CHECK_EQUAL(3, client.deleted_note_titles().size());

  client.begin_synchronization();
  const auto & note = manager.create("test");
  client.set_revision(note, 1);
  client.end_synchronization();
  client.reparse();
  CHECK_EQUAL(1, client.get_revision(note));

  std::remove(test_manifest.c_str());
}

}

