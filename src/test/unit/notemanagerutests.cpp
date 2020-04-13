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


  TEST_FIXTURE(Fixture, create_and_find)
  {
    manager.create();
    manager.create();
    gnote::NoteBase::Ptr test_note = manager.create("test note");
    CHECK(test_note != NULL);
    // 3 notes + template note
    CHECK_EQUAL(4, manager.get_notes().size());
    CHECK(manager.find("test note") == test_note);
    CHECK(manager.find_by_uri(test_note->uri()) == test_note);
  }
}

