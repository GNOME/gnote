/*
 * gnote
 *
 * Copyright (C) 2017-2020,2022-2023,2025 Aurimas Cernius
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
#include <iostream>

#include <glib/gstdio.h>
#include <UnitTest++/UnitTest++.h>

#include "notemanager.hpp"
#include "sharp/files.hpp"
#include "sharp/directory.hpp"
#include "synchronization/silentui.hpp"
#include "test/testgnote.hpp"
#include "test/testnote.hpp"
#include "test/testnotemanager.hpp"
#include "test/testsyncmanager.hpp"

using namespace gnote;


void remove_dir(const Glib::ustring dir)
{
  auto subitems = sharp::directory_get_directories(dir);
  for(auto subdir : subitems) {
    remove_dir(subdir);
  }
  subitems = sharp::directory_get_files(dir);
  for(auto file : subitems) {
    g_remove(file.c_str());
  }
  g_rmdir(dir.c_str());
}


SUITE(SyncManagerTests)
{
  struct Fixture
  {
    Glib::ustring tempdir1;
    Glib::ustring tempdir2;
    Glib::ustring notesdir1;
    Glib::ustring notesdir2;
    Glib::ustring syncdir;
    Glib::ustring manifest1;
    Glib::ustring manifest2;
    test::Gnote gnote1;
    test::Gnote gnote2;
    std::unique_ptr<test::NoteManager> manager1;
    std::unique_ptr<test::NoteManager> manager2;
    std::unique_ptr<test::SyncManager> sync_manager1;
    std::unique_ptr<test::SyncManager> sync_manager2;
    std::vector<Glib::ustring> files;

    Fixture()
    {
      tempdir1 = make_temp_dir();
      tempdir2 = make_temp_dir();
      notesdir1 = tempdir1 + "/notes";
      notesdir2 = tempdir2 + "/notes";
      syncdir = tempdir1 + "/sync";
      REQUIRE CHECK(g_mkdir(syncdir.c_str(), S_IRWXU) == 0);
      manifest1 = tempdir1 + "/manifest.xml";
      manifest2 = tempdir2 + "/manifest.xml";

      manager1.reset(new test::NoteManager(notesdir1, gnote1));
      gnote1.notebook_manager(&manager1->notebook_manager());
      create_note(*manager1, "note1", "content1");
      create_note(*manager1, "note2", "content2");
      create_note(*manager1, "note3", "content3");

      manager2.reset(new test::NoteManager(notesdir2, gnote2));
      gnote2.notebook_manager(&manager2->notebook_manager());

      sync_manager1.reset(new test::SyncManager(gnote1, *manager1, syncdir));
      gnote1.sync_manager(sync_manager1.get());
      sync_manager2.reset(new test::SyncManager(gnote2, *manager2, syncdir));
      gnote2.sync_manager(sync_manager2.get());
    }

    ~Fixture()
    {
      remove_dir(tempdir1);
      remove_dir(tempdir2);
    }

    Glib::ustring make_temp_dir()
    {
      char temp_dir_tmpl[] = "/tmp/gnotetestnotesXXXXXX";
      char *temp_dir = g_mkdtemp(temp_dir_tmpl);
      REQUIRE CHECK(temp_dir != NULL);
      return temp_dir;
    }

    Glib::ustring make_note_content(const Glib::ustring & title, const Glib::ustring & body)
    {
      return Glib::ustring::compose("<note-content><note-title>%1</note-title>\n\n%2</note-content>", title, body);
    }

    void create_note(test::NoteManager & manager, Glib::ustring && title, Glib::ustring && body)
    {
      auto content = make_note_content(title, body);
      manager.create(std::move(title), std::move(content)).save();
    }

    bool find_note_in_files(const Glib::ustring & title)
    {
      Glib::ustring content_search = Glib::ustring::compose("<note-title>%1</note-title>", title);
      for(auto file : files) {
        Glib::ustring content = sharp::file_read_all_text(file);
        if(content.find(content_search) != Glib::ustring::npos) {
          return true;
        }
      }

      return false;
    }

    void get_notes_in_dir(const Glib::ustring & dir)
    {
      files = sharp::directory_get_files_with_ext(dir, ".note");
    }
  };

#define FIRST_SYNC(ignote, sync_manager, note_manager, manifest, client, ui) \
  test::SyncClient & client = dynamic_cast<test::SyncClient&>(sync_manager->get_client(manifest)); \
  (void)client; \
  gnote::sync::SilentUI::Ptr ui = gnote::sync::SilentUI::create(ignote, *note_manager); \
  sync_manager->perform_synchronization(ui);

#define UPDATE_NOTE(manager, title, new_title, new_content) \
  do { \
    auto & note = dynamic_cast<test::Note&>(manager->find(title).value().get()); \
    note.set_xml_content(make_note_content(new_title, new_title)); \
    note.set_change_type(gnote::CONTENT_CHANGED); \
    note.save(); \
  } \
  while(0)

  TEST_FIXTURE(Fixture, clean_sync)
  {
    FIRST_SYNC(gnote1, sync_manager1, manager1, manifest1, sync_client, sync_ui)

    Glib::ustring syncednotesdir = syncdir + "/0/0";
    REQUIRE CHECK(sharp::directory_exists(syncednotesdir));
    get_notes_in_dir(syncednotesdir);
    REQUIRE CHECK_EQUAL(3, files.size());

    CHECK(find_note_in_files("note1"));
    CHECK(find_note_in_files("note2"));
    CHECK(find_note_in_files("note3"));
  }

  TEST_FIXTURE(Fixture, first_sync_existing_store)
  {
    FIRST_SYNC(gnote1, sync_manager1, manager1, manifest1, sync_client1, sync_ui1)
    FIRST_SYNC(gnote2, sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    get_notes_in_dir(notesdir2);
    REQUIRE CHECK_EQUAL(3, files.size());
    CHECK(find_note_in_files("note1"));
    CHECK(find_note_in_files("note2"));
    CHECK(find_note_in_files("note3"));
  }

  TEST_FIXTURE(Fixture, merge_two_clients)
  {
    FIRST_SYNC(gnote1, sync_manager1, manager1, manifest1, sync_client1, sync_ui1)

    create_note(*manager2, "note4", "content4");
    FIRST_SYNC(gnote2, sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    Glib::ustring syncednotesdir = syncdir + "/0/1";
    REQUIRE CHECK(sharp::directory_exists(syncednotesdir));
    get_notes_in_dir(syncednotesdir);
    REQUIRE CHECK_EQUAL(1, files.size());
    CHECK(find_note_in_files("note4"));
  }

  TEST_FIXTURE(Fixture, download_new_notes_from_server)
  {
    FIRST_SYNC(gnote1, sync_manager1, manager1, manifest1, sync_client1, sync_ui1)
    FIRST_SYNC(gnote2, sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    // create new note and sync again
    create_note(*manager2, "note4", "content4");
    sync_manager2->perform_synchronization(sync_ui2);

    // check sync dir contents
    Glib::ustring syncednotesdir = syncdir + "/0";
    REQUIRE CHECK(sharp::directory_exists(syncednotesdir));
    files = sharp::directory_get_directories(syncednotesdir);
    CHECK_EQUAL(2, files.size());  // first time client2 only downloads

    // sync to first client
    sync_manager1->perform_synchronization(sync_ui1);
    get_notes_in_dir(notesdir1);
    CHECK_EQUAL(4, files.size()); // 3 original + 1 from other client
    CHECK(find_note_in_files("note4"));
  }

  TEST_FIXTURE(Fixture, upload_note_update)
  {
    FIRST_SYNC(gnote1, sync_manager1, manager1, manifest1, sync_client1, sync_ui1)

    // update note and sync again
    UPDATE_NOTE(manager1, "note2", "note4", "updated content");
    sync_client1.reparse();
    sync_manager1->perform_synchronization(sync_ui1);

    // check sync dir contents
    Glib::ustring syncednotesdir = syncdir + "/0";
    REQUIRE CHECK(sharp::directory_exists(syncednotesdir));
    files = sharp::directory_get_directories(syncednotesdir);
    CHECK_EQUAL(2, files.size());

    FIRST_SYNC(gnote2, sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    get_notes_in_dir(notesdir2);
    REQUIRE CHECK_EQUAL(3, files.size());
    CHECK(!find_note_in_files("note2"));
    CHECK(find_note_in_files("note4"));
  }

  TEST_FIXTURE(Fixture, download_note_update)
  {
    FIRST_SYNC(gnote1, sync_manager1, manager1, manifest1, sync_client1, sync_ui1)
    FIRST_SYNC(gnote2, sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    // update note and sync again
    UPDATE_NOTE(manager1, "note2", "note4", "updated content");
    sync_client1.reparse();
    sync_manager1->perform_synchronization(sync_ui1);

    // download updates
    sync_manager2->perform_synchronization(sync_ui2);
    get_notes_in_dir(notesdir2);
    REQUIRE CHECK_EQUAL(3, files.size());
    CHECK(find_note_in_files("note1"));
    CHECK(find_note_in_files("note3"));
    CHECK(find_note_in_files("note4"));
    CHECK(!find_note_in_files("note2"));
  }

  TEST_FIXTURE(Fixture, delete_note)
  {
    FIRST_SYNC(gnote1, sync_manager1, manager1, manifest1, sync_client1, sync_ui1)

    // remove note
    auto note2 = manager1->find("note2");
    REQUIRE CHECK(bool(note2));
    manager1->delete_note(note2.value());
    sync_manager1->perform_synchronization(sync_ui1);

    FIRST_SYNC(gnote2, sync_manager2, manager2, manifest2, sync_client2, sync_ui2)
    get_notes_in_dir(notesdir2);
    REQUIRE CHECK_EQUAL(2, files.size());
    CHECK(find_note_in_files("note1"));
    CHECK(find_note_in_files("note3"));
    CHECK(!find_note_in_files("note2"));
  }

  TEST_FIXTURE(Fixture, note_modification_conflict)
  {
    FIRST_SYNC(gnote1, sync_manager1, manager1, manifest1, sync_client1, sync_ui1)
    FIRST_SYNC(gnote2, sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    // update note and sync again
    UPDATE_NOTE(manager1, "note2", "note4", "updated content");
    sync_client1.reparse();
    sync_manager1->perform_synchronization(sync_ui1);

    // update other client and sync
    UPDATE_NOTE(manager2, "note2", "note5", "content updated");
    sync_client2.reparse();
    sync_manager2->perform_synchronization(sync_ui1);

    // sync client1 again
    sync_manager1->perform_synchronization(sync_ui1);
    get_notes_in_dir(notesdir2);
    REQUIRE CHECK_EQUAL(3, files.size());
    CHECK(find_note_in_files("note1"));
    CHECK(find_note_in_files("note3"));
    CHECK(!find_note_in_files("note2"));
    CHECK(find_note_in_files("note4"));
  }

  TEST_FIXTURE(Fixture, conflict_with_deletion_on_server)
  {
    FIRST_SYNC(gnote1, sync_manager1, manager1, manifest1, sync_client1, sync_ui1)
    FIRST_SYNC(gnote2, sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    // remove note
    auto & note2 = dynamic_cast<test::Note&>(manager2->find("note2").value().get());
    manager2->delete_note(note2);
    sync_manager2->perform_synchronization(sync_ui2);

    // update note and sync again
    UPDATE_NOTE(manager1, "note2", "note4", "updated content");
    sync_client1.reparse();
    sync_manager1->perform_synchronization(sync_ui1);
    get_notes_in_dir(notesdir1);
    REQUIRE CHECK_EQUAL(2, files.size());
    CHECK(find_note_in_files("note1"));
    CHECK(find_note_in_files("note3"));
    CHECK(!find_note_in_files("note2"));
  }
}

