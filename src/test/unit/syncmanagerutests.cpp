/*
 * gnote
 *
 * Copyright (C) 2017,2018 Aurimas Cernius
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
#include "test/testnote.hpp"
#include "test/testnotemanager.hpp"
#include "test/testsyncmanager.hpp"
#include "test/testtagmanager.hpp"

using namespace gnote;


SUITE(SyncManagerTests)
{
  struct Fixture
  {
    Glib::ustring notesdir1;
    Glib::ustring notesdir2;
    Glib::ustring syncdir;
    Glib::ustring manifest1;
    Glib::ustring manifest2;
    test::NoteManager *manager1;
    test::NoteManager *manager2;
    test::SyncManager *sync_manager1;
    test::SyncManager *sync_manager2;

    Fixture()
    {
      Glib::ustring notes_dir1 = make_temp_dir();
      Glib::ustring notes_dir2 = make_temp_dir();
      notesdir1 = notes_dir1 + "/notes";
      notesdir2 = notes_dir2 + "/notes";
      syncdir = notes_dir1 + "/sync";
      REQUIRE CHECK(g_mkdir(syncdir.c_str(), S_IRWXU) == 0);
      manifest1 = notes_dir1 + "/manifest.xml";
      manifest2 = notes_dir2 + "/manifest.xml";

      test::TagManager::ensure_exists();

      manager1 = new test::NoteManager(notesdir1);
      create_note(*manager1, "note1", "content1");
      create_note(*manager1, "note2", "content2");
      create_note(*manager1, "note3", "content3");

      manager2 = new test::NoteManager(notesdir2);

      sync_manager1 = new test::SyncManager(*manager1, syncdir);
      sync_manager2 = new test::SyncManager(*manager2, syncdir);
    }

    ~Fixture()
    {
      delete manager1;
      delete manager2;
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

    void create_note(test::NoteManager & manager, const Glib::ustring & title, const Glib::ustring & body)
    {
      manager.create(title, make_note_content(title, body))->save();
    }

    bool find_note(const std::list<Glib::ustring> & files, const Glib::ustring & title)
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
  };

#define FIRST_SYNC(sync_manager, note_manager, manifest, client, ui) \
  test::SyncClient::Ptr client = dynamic_pointer_cast<test::SyncClient>(sync_manager->get_client(manifest)); \
  gnote::sync::SilentUI::Ptr ui = gnote::sync::SilentUI::create(*note_manager); \
  sync_manager->perform_synchronization(ui);

  TEST_FIXTURE(Fixture, clean_sync)
  {
    FIRST_SYNC(sync_manager1, manager1, manifest1, sync_client, sync_ui)

    Glib::ustring syncednotesdir = syncdir + "/0/0";
    REQUIRE CHECK(sharp::directory_exists(syncednotesdir));
    std::list<Glib::ustring> files;
    sharp::directory_get_files_with_ext(syncednotesdir, ".note", files);
    REQUIRE CHECK_EQUAL(3, files.size());

    CHECK(find_note(files, "note1"));
    CHECK(find_note(files, "note2"));
    CHECK(find_note(files, "note3"));
  }

  TEST_FIXTURE(Fixture, first_sync_existing_store)
  {
    FIRST_SYNC(sync_manager1, manager1, manifest1, sync_client1, sync_ui1)
    FIRST_SYNC(sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    std::list<Glib::ustring> files;
    sharp::directory_get_files_with_ext(notesdir2, ".note", files);
    REQUIRE CHECK_EQUAL(4, files.size()); // 3 downloaded notes + template
    CHECK(find_note(files, "note1"));
    CHECK(find_note(files, "note2"));
    CHECK(find_note(files, "note3"));
  }

  TEST_FIXTURE(Fixture, merge_two_clients)
  {
    FIRST_SYNC(sync_manager1, manager1, manifest1, sync_client1, sync_ui1)

    create_note(*manager2, "note4", "content4");
    FIRST_SYNC(sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    Glib::ustring syncednotesdir = syncdir + "/0/1";
    REQUIRE CHECK(sharp::directory_exists(syncednotesdir));
    std::list<Glib::ustring> files;
    sharp::directory_get_files_with_ext(syncednotesdir, ".note", files);
    REQUIRE CHECK_EQUAL(2, files.size());  // note + template note
    CHECK(find_note(files, "note4"));
  }

  TEST_FIXTURE(Fixture, download_new_notes_from_server)
  {
    FIRST_SYNC(sync_manager1, manager1, manifest1, sync_client1, sync_ui1)
    FIRST_SYNC(sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    // create new note and sync again
    create_note(*manager2, "note4", "content4");
    sync_manager2->perform_synchronization(sync_ui2);

    // check sync dir contents
    Glib::ustring syncednotesdir = syncdir + "/0";
    REQUIRE CHECK(sharp::directory_exists(syncednotesdir));
    std::list<Glib::ustring> files;
    sharp::directory_get_directories(syncednotesdir, files);
    CHECK_EQUAL(3, files.size());

    // sync to first client
    sync_manager1->perform_synchronization(sync_ui1);
    files.clear();
    sharp::directory_get_files_with_ext(notesdir1, ".note", files);
    CHECK_EQUAL(5, files.size()); // 3 original + 1 from other client + template from other client
    CHECK(find_note(files, "note4"));
  }

  TEST_FIXTURE(Fixture, upload_note_update)
  {
    FIRST_SYNC(sync_manager1, manager1, manifest1, sync_client1, sync_ui1)

    // update note and sync again
    auto note = std::dynamic_pointer_cast<test::Note>(manager1->find("note2"));
    note->set_xml_content(make_note_content("note4", "updated content"));
    note->set_change_type(gnote::CONTENT_CHANGED);
    note->save();
    sync_client1->reparse();
    sync_manager1->perform_synchronization(sync_ui1);

    // check sync dir contents
    Glib::ustring syncednotesdir = syncdir + "/0";
    REQUIRE CHECK(sharp::directory_exists(syncednotesdir));
    std::list<Glib::ustring> files;
    sharp::directory_get_directories(syncednotesdir, files);
    CHECK_EQUAL(2, files.size());

    FIRST_SYNC(sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    files.clear();
    sharp::directory_get_files_with_ext(notesdir2, ".note", files);
    REQUIRE CHECK_EQUAL(4, files.size()); // 3 downloaded notes + template
    CHECK(!find_note(files, "note2"));
    CHECK(find_note(files, "note4"));
  }

  TEST_FIXTURE(Fixture, download_note_update)
  {
    FIRST_SYNC(sync_manager1, manager1, manifest1, sync_client1, sync_ui1)
    FIRST_SYNC(sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    // update note and sync again
    auto note = std::dynamic_pointer_cast<test::Note>(manager1->find("note2"));
    note->set_xml_content(make_note_content("note4", "updated content"));
    note->set_change_type(gnote::CONTENT_CHANGED);
    note->save();
    sync_client1->reparse();
    sync_manager1->perform_synchronization(sync_ui1);

    // download updates
    sync_manager2->perform_synchronization(sync_ui2);
    std::list<Glib::ustring> files;
    sharp::directory_get_files_with_ext(notesdir2, ".note", files);
    REQUIRE CHECK_EQUAL(4, files.size()); // 3 downloaded notes + template
    CHECK(find_note(files, "note1"));
    CHECK(find_note(files, "note3"));
    CHECK(find_note(files, "note4"));
    CHECK(!find_note(files, "note2"));
  }

  TEST_FIXTURE(Fixture, delete_note)
  {
    FIRST_SYNC(sync_manager1, manager1, manifest1, sync_client1, sync_ui1)

    // remove note
    auto note = std::dynamic_pointer_cast<test::Note>(manager1->find("note2"));
    manager1->delete_note(note);
    note.reset();
    sync_manager1->perform_synchronization(sync_ui1);

    FIRST_SYNC(sync_manager2, manager2, manifest2, sync_client2, sync_ui2)
    std::list<Glib::ustring> files;
    sharp::directory_get_files_with_ext(notesdir2, ".note", files);
    REQUIRE CHECK_EQUAL(3, files.size()); // 2 downloaded notes + template
    CHECK(find_note(files, "note1"));
    CHECK(find_note(files, "note3"));
    CHECK(!find_note(files, "note2"));
  }

  TEST_FIXTURE(Fixture, note_modification_conflict)
  {
    FIRST_SYNC(sync_manager1, manager1, manifest1, sync_client1, sync_ui1)
    FIRST_SYNC(sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    // update note and sync again
    auto note = std::dynamic_pointer_cast<test::Note>(manager1->find("note2"));
    note->set_xml_content(make_note_content("note4", "updated content"));
    note->set_change_type(gnote::CONTENT_CHANGED);
    note->save();
    sync_client1->reparse();
    sync_manager1->perform_synchronization(sync_ui1);

    // update other clinet and sync
    note = std::dynamic_pointer_cast<test::Note>(manager2->find("note2"));
    note->set_xml_content(make_note_content("note5", "content updated"));
    note->set_change_type(gnote::CONTENT_CHANGED);
    note->save();
    sync_client2->reparse();
    sync_manager2->perform_synchronization(sync_ui1);

    // sync client1 again
    sync_manager1->perform_synchronization(sync_ui1);
    std::list<Glib::ustring> files;
    sharp::directory_get_files_with_ext(notesdir2, ".note", files);
    REQUIRE CHECK_EQUAL(4, files.size()); // 3 downloaded notes + template
    CHECK(find_note(files, "note1"));
    CHECK(find_note(files, "note3"));
    CHECK(!find_note(files, "note2"));
    CHECK(find_note(files, "note4"));
  }

  TEST_FIXTURE(Fixture, conflict_with_deletion_on_server)
  {
    FIRST_SYNC(sync_manager1, manager1, manifest1, sync_client1, sync_ui1)
    FIRST_SYNC(sync_manager2, manager2, manifest2, sync_client2, sync_ui2)

    // remove note
    auto note = std::dynamic_pointer_cast<test::Note>(manager2->find("note2"));
    manager2->delete_note(note);
    note.reset();
    sync_manager2->perform_synchronization(sync_ui2);

    // update note and sync again
    note = std::dynamic_pointer_cast<test::Note>(manager1->find("note2"));
    note->set_xml_content(make_note_content("note4", "updated content"));
    note->set_change_type(gnote::CONTENT_CHANGED);
    note->save();
    sync_client1->reparse();
    sync_manager1->perform_synchronization(sync_ui1);
    std::list<Glib::ustring> files;
    sharp::directory_get_files_with_ext(notesdir1, ".note", files);
    REQUIRE CHECK_EQUAL(3, files.size()); // 2 downloaded notes + template
    CHECK(find_note(files, "note1"));
    CHECK(find_note(files, "note3"));
    CHECK(!find_note(files, "note2"));
  }
}

