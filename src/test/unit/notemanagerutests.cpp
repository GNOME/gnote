/*
 * gnote
 *
 * Copyright (C) 2017,2019-2020,2023,2025 Aurimas Cernius
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


#include <utime.h>
#include <glib/gstdio.h>
#include <UnitTest++/UnitTest++.h>

#include "sharp/directory.hpp"
#include "test/testgnote.hpp"
#include "test/testnotemanager.hpp"


SUITE(NoteManager)
{
  struct Fixture
  {
    test::Gnote g;
    test::NoteManager manager;

    Fixture()
      : manager(make_notes_dir(), g)
    {
      g.notebook_manager(&manager.notebook_manager());
    }

    static Glib::ustring make_notes_dir()
    {
      char notes_dir_tmpl[] = "/tmp/gnotetestnotesXXXXXX";
      char *notes_dir = g_mkdtemp(notes_dir_tmpl);
      return notes_dir;
    }

    gnote::NoteBase::Ptr create_template_note()
    {
      auto & templ = manager.get_or_create_template_note();
      templ.set_xml_content(Glib::ustring::compose("<note-content><note-title>%1</note-title>\n\ntest template content</note-content>", templ.get_title()));
      return templ.shared_from_this();
    }
  };


  TEST(get_note_content)
  {
    auto content = gnote::NoteManagerBase::get_note_content("test_title", "test_content");
    CHECK(content.find("<note-title>test_title</note-title>") != Glib::ustring::npos);
    CHECK(content.find("test_content") != Glib::ustring::npos);
  }

  TEST(get_note_content_special_chars)
  {
    auto content = gnote::NoteManagerBase::get_note_content("test<title", "test>content");
    CHECK(content.find("<note-title>test&lt;title</note-title>") != Glib::ustring::npos);
    CHECK(content.find("test&gt;content") != Glib::ustring::npos);
  }

  TEST(split_title_from_content)
  {
    Glib::ustring body;
    auto title = gnote::NoteManagerBase::split_title_from_content("test", body);
    CHECK_EQUAL("test", title);
    CHECK(body.empty());

    title = gnote::NoteManagerBase::split_title_from_content("test\ncontent", body);
    CHECK_EQUAL("test", title);
    CHECK_EQUAL("content", body);
  }

  TEST_FIXTURE(Fixture, create_no_args)
  {
    auto & note1 = manager.create();
    auto & note2 = manager.create();

    CHECK_EQUAL("New Note 1", note1.get_title());
    CHECK(note1.data().text().find("Describe your new note here.") != Glib::ustring::npos);
    CHECK_EQUAL("New Note 2", note2.get_title());
    CHECK(note2.data().text().find("Describe your new note here.") != Glib::ustring::npos);
    CHECK_EQUAL(2, manager.note_count());
  }

  TEST_FIXTURE(Fixture, create_no_args_from_template)
  {
    auto templ = create_template_note();
    auto & note = manager.create();
    CHECK_EQUAL("New Note 1", note.get_title());
    CHECK(note.data().text().find("test template content") != Glib::ustring::npos);
  }

  TEST_FIXTURE(Fixture, create_with_title)
  {
    auto & note = manager.create("test");
    CHECK_EQUAL("test", note.get_title());
    CHECK(note.data().text().find("Describe your new note here.") != Glib::ustring::npos);
    CHECK_EQUAL(1, manager.note_count());
  }

  TEST_FIXTURE(Fixture, create_with_title_from_template)
  {
    auto templ = create_template_note();
    auto & note = manager.create("test");
    CHECK_EQUAL("test", note.get_title());
    CHECK(note.data().text().find("test template content") != Glib::ustring::npos);
    CHECK_EQUAL(2, manager.note_count());
  }

  TEST_FIXTURE(Fixture, create_with_text_content)
  {
    auto & note = manager.create("test\ntest content");
    CHECK_EQUAL("test", note.get_title());
    CHECK(note.data().text().find("test content") != Glib::ustring::npos);
    CHECK_EQUAL(1, manager.note_count());
  }

  TEST_FIXTURE(Fixture, create_with_text_content_having_template)
  {
    auto templ = create_template_note();
    auto & note = manager.create("test\ntest content");
    CHECK_EQUAL("test", note.get_title());
    CHECK(note.data().text().find("test content") != Glib::ustring::npos);
    CHECK_EQUAL(2, manager.note_count());
  }

  TEST_FIXTURE(Fixture, create_and_find)
  {
    manager.create();
    manager.create();
    auto & test_note = manager.create("test note");
    CHECK_EQUAL(3, manager.note_count());
    CHECK(&manager.find("test note").value().get() == &test_note);
    CHECK(&manager.find_by_uri(test_note.uri()).value().get() == &test_note);
  }

  TEST_FIXTURE(Fixture, create_with_xml)
  {
    auto & note = manager.create("test", "<note-content><note-title>test</note-title>\n\ntest content");
    CHECK_EQUAL("test", note.get_title());
    CHECK(note.data().text().find("test content") != Glib::ustring::npos);
    CHECK_EQUAL(1, manager.note_count());
  }

  TEST_FIXTURE(Fixture, create_with_guid)
  {
    auto & note = manager.create_with_guid("test", "93b3f3ef-9eea-4cdc-9f78-76af1629987a");
    CHECK_EQUAL("test", note.get_title());
    CHECK(note.data().text().find("Describe your new note here.") != Glib::ustring::npos);
    CHECK_EQUAL("93b3f3ef-9eea-4cdc-9f78-76af1629987a", note.id());
    CHECK_EQUAL("note://gnote/93b3f3ef-9eea-4cdc-9f78-76af1629987a", note.uri());
    CHECK_EQUAL(1, manager.note_count());
    auto other = manager.find_by_uri("note://gnote/93b3f3ef-9eea-4cdc-9f78-76af1629987a");
    REQUIRE CHECK(other.has_value());
    CHECK_EQUAL(&note, &other.value().get());
  }

  TEST_FIXTURE(Fixture, create_with_guid_from_template)
  {
    auto templ = create_template_note();
    auto & note = manager.create_with_guid("test", "93b3f3ef-9eea-4cdc-9f78-76af1629987a");
    CHECK_EQUAL("test", note.get_title());
    CHECK(note.data().text().find("test template content") != Glib::ustring::npos);
    CHECK_EQUAL("93b3f3ef-9eea-4cdc-9f78-76af1629987a", note.id());
    CHECK_EQUAL("note://gnote/93b3f3ef-9eea-4cdc-9f78-76af1629987a", note.uri());
    CHECK_EQUAL(2, manager.note_count());
    auto other = manager.find_by_uri("note://gnote/93b3f3ef-9eea-4cdc-9f78-76af1629987a");
    REQUIRE CHECK(other.has_value());
    CHECK_EQUAL(&note, &other.value().get());
  }

  TEST_FIXTURE(Fixture, create_with_guid_multiline_title)
  {
    auto & note = manager.create_with_guid("test\ntest content", "93b3f3ef-9eea-4cdc-9f78-76af1629987a");
    CHECK_EQUAL("test", note.get_title());
    CHECK(note.data().text().find("test content") != Glib::ustring::npos);
    CHECK_EQUAL("93b3f3ef-9eea-4cdc-9f78-76af1629987a", note.id());
    CHECK_EQUAL("note://gnote/93b3f3ef-9eea-4cdc-9f78-76af1629987a", note.uri());
    CHECK_EQUAL(1, manager.note_count());
  }

  TEST(delete_old_backups)
  {
    auto dir = Fixture::make_notes_dir();
    auto current_time = Glib::DateTime::create_now_utc();
    auto modified = current_time.add_days(-50).to_unix();
    for(unsigned i = 0; i < 10; ++i) {
      auto file_name = Glib::ustring::compose("%1/file%2.txt", dir, i);
      auto file = fopen(file_name.c_str(), "w");
      fprintf(file, "test %u", i);
      fclose(file);

      if(i & 1) {
        utimbuf utb { modified, modified };
        g_utime(file_name.c_str(), &utb);
      }
    }

    auto files = sharp::directory_get_files(dir);
    CHECK_EQUAL(10, files.size());

    auto old = current_time.add_days(-5);
    test::NoteManager::delete_old_backups(dir, old);

    files = sharp::directory_get_files(dir);
    CHECK_EQUAL(5, files.size());
    std::sort(files.begin(), files.end());
    for(unsigned i = 0, j = 0; i < 10; i += 2, ++j) {
      CHECK_EQUAL(Glib::ustring::compose("%1/file%2.txt", dir.c_str(), i), files[j]);
    }
  }
}

