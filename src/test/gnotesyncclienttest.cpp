/*
 * gnote
 *
 * Copyright (C) 2014 Aurimas Cernius
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

#include <boost/test/minimal.hpp>

#include "testnotemanager.hpp"
#include "testsyncclient.hpp"
#include "testtagmanager.hpp"


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


std::string create_manifest()
{
  std::string test_manifest = std::tmpnam(NULL);
  FILE *file = std::fopen(test_manifest.c_str(), "w");
  if(!file) {
    std::fputs("Failed to write manifest file", stderr);
    abort();
  }

  std::fputs(TEST_MANIFEST, file);
  std::fclose(file);
  return test_manifest;
}


int test_main(int /*argc*/, char ** /*argv*/)
{
  std::string test_manifest = create_manifest();
  
  new test::TagManager;
  test::NoteManager manager(test::NoteManager::test_notes_dir());
  test::SyncClient client(manager);
  client.set_manifest_path(test_manifest);
  client.reparse();

  sharp::DateTime sync_date(sharp::DateTime::from_iso8601("2014-04-21T20:13:24.711343Z"));
  BOOST_CHECK(client.last_sync_date() == sync_date);
  BOOST_CHECK(client.last_synchronized_revision() == 0);
  BOOST_CHECK(client.associated_server_id() == "38afddc2-9ce8-46ba-b106-baf3916c74b8");
  BOOST_CHECK(client.deleted_note_titles().size() == 3);

  gnote::NoteBase::Ptr note = manager.create("test");
  client.set_revision(note, 1);
  client.reparse();
  BOOST_CHECK(client.get_revision(note) == 1);

  std::remove(test_manifest.c_str());
  return 0;
}

