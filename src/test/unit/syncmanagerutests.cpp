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


SUITE(SyncManagerTests)
{
  class Synchronizer
  {
  public:
    Synchronizer(const Glib::ustring &tempdir, const Glib::ustring &syncdir)
      : m_tempdir(tempdir)
      , m_notesdir(tempdir + "/notes")
      , m_manifest(tempdir + "/manifest.xml")
    {
      m_note_manager.reset(new test::NoteManager(m_notesdir, m_gnote));
      m_gnote.notebook_manager(&m_note_manager->notebook_manager());
      m_sync_manager.reset(new test::SyncManager(m_gnote, *m_note_manager, syncdir));
      m_gnote.sync_manager(m_sync_manager.get());
    }

    test::NoteManager &note_manager()
    {
      return *m_note_manager;
    }

    const Glib::ustring &notes_dir() const
    {
      return m_notesdir;
    }

    void perform_sync()
    {
      auto _ = m_sync_manager->get_client(m_manifest);
      auto ui = gnote::sync::SilentUI::create(m_gnote, *m_note_manager);
      m_sync_manager->perform_synchronization(ui);
    }
  private:
    test::Gnote m_gnote;
    std::unique_ptr<test::NoteManager> m_note_manager;
    std::unique_ptr<test::SyncManager> m_sync_manager;
    const Glib::ustring &m_tempdir;
    const Glib::ustring m_notesdir;
    const Glib::ustring m_manifest;
  };

  struct Fixture1
  {
    Glib::ustring tempdir;
    Glib::ustring syncdir;
    Synchronizer synchronizer;

    Fixture1()
      : tempdir(make_temp_dir())
      , syncdir(tempdir + "/sync")
      , synchronizer(tempdir, syncdir)
    {
      REQUIRE CHECK(g_mkdir(syncdir.c_str(), S_IRWXU) == 0);
      auto &note_manager = synchronizer.note_manager();
      create_note(note_manager, "note1", "content1");
      create_note(note_manager, "note2", "content2");
      create_note(note_manager, "note3", "content3");
    }

    ~Fixture1()
    {
      test::remove_dir(tempdir);
    }

    Glib::ustring make_temp_dir()
    {
      char temp_dir_tmpl[] = "/tmp/gnotetestnotesXXXXXX";
      char *temp_dir = g_mkdtemp(temp_dir_tmpl);
      REQUIRE CHECK(temp_dir != NULL);
      return temp_dir;
    }

    static Glib::ustring make_note_content(const Glib::ustring &title, const Glib::ustring &body)
    {
      return Glib::ustring::compose("<note-content><note-title>%1</note-title>\n\n%2</note-content>", title, body);
    }

    static void create_note(test::NoteManager &manager, Glib::ustring &&title, Glib::ustring &&body)
    {
      auto content = make_note_content(title, body);
      manager.create(std::move(title), std::move(content)).save();
    }

    static std::vector<Glib::ustring> get_notes_in_dir(const Glib::ustring &dir)
    {
      return sharp::directory_get_files_with_ext(dir, ".note");
    }

    bool find_note_in_files(const std::vector<Glib::ustring> &files, const Glib::ustring &title)
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

    void update_note(test::NoteManager &manager, const Glib::ustring &title, const Glib::ustring &new_title, const Glib::ustring &new_content)
    {
      auto & note = dynamic_cast<test::Note&>(manager.find(title).value().get());
      note.set_xml_content(make_note_content(new_title, new_title));
      note.set_change_type(gnote::CONTENT_CHANGED);
      note.save();
    }
  };

  struct Fixture2
    : Fixture1
  {
    Glib::ustring tempdir2;
    Synchronizer synchronizer2;

    Fixture2()
      : tempdir2(make_temp_dir())
      , synchronizer2(tempdir2, syncdir)
    {
    }

    ~Fixture2()
    {
      test::remove_dir(tempdir2);
    }
  };

  TEST_FIXTURE(Fixture1, clean_sync)
  {
    synchronizer.perform_sync();

    Glib::ustring syncednotesdir = syncdir + "/0/0";
    REQUIRE CHECK(sharp::directory_exists(syncednotesdir));
    auto files = get_notes_in_dir(syncednotesdir);
    REQUIRE CHECK_EQUAL(3, files.size());

    CHECK(find_note_in_files(files, "note1"));
    CHECK(find_note_in_files(files, "note2"));
    CHECK(find_note_in_files(files, "note3"));
  }

  TEST_FIXTURE(Fixture2, first_sync_existing_store)
  {
    synchronizer.perform_sync();
    synchronizer2.perform_sync();

    auto files = get_notes_in_dir(synchronizer2.notes_dir());
    REQUIRE CHECK_EQUAL(3, files.size());
    CHECK(find_note_in_files(files, "note1"));
    CHECK(find_note_in_files(files, "note2"));
    CHECK(find_note_in_files(files, "note3"));
  }

  TEST_FIXTURE(Fixture2, merge_two_clients)
  {
    synchronizer.perform_sync();

    create_note(synchronizer2.note_manager(), "note4", "content4");
    synchronizer2.perform_sync();

    Glib::ustring syncednotesdir = syncdir + "/0/1";
    REQUIRE CHECK(sharp::directory_exists(syncednotesdir));
    auto files = get_notes_in_dir(syncednotesdir);
    REQUIRE CHECK_EQUAL(1, files.size());
    CHECK(find_note_in_files(files, "note4"));

    CHECK_EQUAL(3, synchronizer.note_manager().note_count());
    create_note(synchronizer.note_manager(), "note5", "content5");
    synchronizer.perform_sync();
    files = get_notes_in_dir(synchronizer.notes_dir());
    CHECK_EQUAL(5, files.size());
    CHECK_EQUAL(5, synchronizer.note_manager().note_count());
  }

  TEST_FIXTURE(Fixture2, download_new_notes_from_server)
  {
    synchronizer.perform_sync();
    synchronizer2.perform_sync();

    // create new note and sync again
    create_note(synchronizer2.note_manager(), "note4", "content4");
    synchronizer2.perform_sync();

    // check sync dir contents
    Glib::ustring syncednotesdir = syncdir + "/0";
    REQUIRE CHECK(sharp::directory_exists(syncednotesdir));
    auto files = sharp::directory_get_directories(syncednotesdir);
    CHECK_EQUAL(2, files.size());  // first time client2 only downloads

    // sync to first client
    synchronizer.perform_sync();
    files = get_notes_in_dir(synchronizer.notes_dir());
    CHECK_EQUAL(4, files.size()); // 3 original + 1 from other client
    CHECK(find_note_in_files(files, "note4"));
  }

  TEST_FIXTURE(Fixture2, upload_note_update)
  {
    synchronizer.perform_sync();

    // update note and sync again
    update_note(synchronizer.note_manager(), "note2", "note4", "updated content");
    synchronizer.perform_sync();

    // check sync dir contents
    Glib::ustring syncednotesdir = syncdir + "/0";
    REQUIRE CHECK(sharp::directory_exists(syncednotesdir));
    auto files = sharp::directory_get_directories(syncednotesdir);
    CHECK_EQUAL(2, files.size());

    synchronizer2.perform_sync();

    files = get_notes_in_dir(synchronizer2.notes_dir());
    REQUIRE CHECK_EQUAL(3, files.size());
    CHECK(!find_note_in_files(files, "note2"));
    CHECK(find_note_in_files(files, "note4"));
  }

  TEST_FIXTURE(Fixture2, download_note_update)
  {
    synchronizer.perform_sync();
    synchronizer2.perform_sync();

    // update note and sync again
    update_note(synchronizer.note_manager(), "note2", "note4", "updated content");
    synchronizer.perform_sync();

    // download updates
    synchronizer2.perform_sync();
    auto files = get_notes_in_dir(synchronizer2.notes_dir());
    REQUIRE CHECK_EQUAL(3, files.size());
    CHECK(find_note_in_files(files, "note1"));
    CHECK(find_note_in_files(files, "note3"));
    CHECK(find_note_in_files(files, "note4"));
    CHECK(!find_note_in_files(files, "note2"));
  }

  TEST_FIXTURE(Fixture2, delete_note)
  {
    synchronizer.perform_sync();

    // remove note
    auto note2 = synchronizer.note_manager().find("note2");
    REQUIRE CHECK(bool(note2));
    synchronizer.note_manager().delete_note(note2.value());
    synchronizer.perform_sync();

    synchronizer2.perform_sync();
    auto files = get_notes_in_dir(synchronizer2.notes_dir());
    REQUIRE CHECK_EQUAL(2, files.size());
    CHECK(find_note_in_files(files, "note1"));
    CHECK(find_note_in_files(files, "note3"));
    CHECK(!find_note_in_files(files, "note2"));
  }

  TEST_FIXTURE(Fixture2, note_modification_conflict)
  {
    synchronizer.perform_sync();
    synchronizer2.perform_sync();

    // update note and sync again
    update_note(synchronizer.note_manager(), "note2", "note4", "updated content");
    synchronizer.perform_sync();

    // update other client and sync
    update_note(synchronizer2.note_manager(), "note2", "note5", "content updated");
    synchronizer2.perform_sync();

    // sync client1 again
    synchronizer.perform_sync();
    auto files = get_notes_in_dir(synchronizer2.notes_dir());
    REQUIRE CHECK_EQUAL(3, files.size());
    CHECK(find_note_in_files(files, "note1"));
    CHECK(find_note_in_files(files, "note3"));
    CHECK(!find_note_in_files(files, "note2"));
    CHECK(find_note_in_files(files, "note4"));
  }

  TEST_FIXTURE(Fixture2, conflict_with_deletion_on_server)
  {
    synchronizer.perform_sync();
    synchronizer2.perform_sync();

    // remove note
    auto & note2 = dynamic_cast<test::Note&>(synchronizer2.note_manager().find("note2").value().get());
    synchronizer2.note_manager().delete_note(note2);
    synchronizer2.perform_sync();

    // update note and sync again
    update_note(synchronizer.note_manager(), "note2", "note4", "updated content");
    synchronizer.perform_sync();
    auto files = get_notes_in_dir(synchronizer.notes_dir());
    REQUIRE CHECK_EQUAL(2, files.size());
    CHECK(find_note_in_files(files, "note1"));
    CHECK(find_note_in_files(files, "note3"));
    CHECK(!find_note_in_files(files, "note2"));
  }
}

