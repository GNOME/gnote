/*
 * gnote
 *
 * Copyright (C) 2017,2019-2020 Aurimas Cernius
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


#include <UnitTest++/UnitTest++.h>

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

    Glib::ustring make_notes_dir()
    {
      char notes_dir_tmpl[] = "/tmp/gnotetestnotesXXXXXX";
      char *notes_dir = g_mkdtemp(notes_dir_tmpl);
      return notes_dir;
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
    auto note1 = manager.create();
    auto note2 = manager.create();

    CHECK_EQUAL("New Note 1", note1->get_title());
    CHECK(note1->data().text().find("Describe your new note here.") != Glib::ustring::npos);
    CHECK_EQUAL("New Note 2", note2->get_title());
    CHECK(note2->data().text().find("Describe your new note here.") != Glib::ustring::npos);
    CHECK_EQUAL(2, manager.get_notes().size());
  }

  TEST_FIXTURE(Fixture, create_with_title)
  {
    auto note = manager.create("test");
    CHECK_EQUAL("test", note->get_title());
    CHECK(note->data().text().find("Describe your new note here.") != Glib::ustring::npos);
    CHECK_EQUAL(1, manager.get_notes().size());
  }

  TEST_FIXTURE(Fixture, create_with_text_content)
  {
    auto note = manager.create("test\ntest content");
    CHECK_EQUAL("test", note->get_title());
    CHECK(note->data().text().find("test content") != Glib::ustring::npos);
    CHECK_EQUAL(1, manager.get_notes().size());
  }

  TEST_FIXTURE(Fixture, create_and_find)
  {
    manager.create();
    manager.create();
    gnote::NoteBase::Ptr test_note = manager.create("test note");
    CHECK(test_note != NULL);
    CHECK_EQUAL(3, manager.get_notes().size());
    CHECK(manager.find("test note") == test_note);
    CHECK(manager.find_by_uri(test_note->uri()) == test_note);
  }

  TEST_FIXTURE(Fixture, create_with_xml)
  {
    auto note = manager.create("test", "<note-content><note-title>test</note-title>\n\ntest content");
    CHECK_EQUAL("test", note->get_title());
    CHECK(note->data().text().find("test content") != Glib::ustring::npos);
    CHECK_EQUAL(1, manager.get_notes().size());
  }

  TEST_FIXTURE(Fixture, create_with_guid)
  {
    auto note = manager.create_with_guid("test", "93b3f3ef-9eea-4cdc-9f78-76af1629987a");
    CHECK_EQUAL("test", note->get_title());
    CHECK(note->data().text().find("Describe your new note here.") != Glib::ustring::npos);
    CHECK_EQUAL("93b3f3ef-9eea-4cdc-9f78-76af1629987a", note->id());
    CHECK_EQUAL("note://gnote/93b3f3ef-9eea-4cdc-9f78-76af1629987a", note->uri());
    CHECK_EQUAL(1, manager.get_notes().size());
    auto other = manager.find_by_uri("note://gnote/93b3f3ef-9eea-4cdc-9f78-76af1629987a");
    CHECK_EQUAL(note, other);
  }


  TEST_FIXTURE(Fixture, create_with_guid_multiline_title)
  {
    auto note = manager.create_with_guid("test\ntest content", "93b3f3ef-9eea-4cdc-9f78-76af1629987a");
    CHECK_EQUAL("test", note->get_title());
    CHECK(note->data().text().find("test content") != Glib::ustring::npos);
    CHECK_EQUAL("93b3f3ef-9eea-4cdc-9f78-76af1629987a", note->id());
    CHECK_EQUAL("note://gnote/93b3f3ef-9eea-4cdc-9f78-76af1629987a", note->uri());
    CHECK_EQUAL(1, manager.get_notes().size());
  }
}

