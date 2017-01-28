/*
 * gnote
 *
 * Copyright (C) 2017 Aurimas Cernius
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

#include "test/testnotemanager.hpp"
#include "test/testtagmanager.hpp"


SUITE(NoteManager)
{
  TEST(create_and_find)
  {
    char notes_dir_tmpl[] = "/tmp/gnotetestnotesXXXXXX";
    char *notes_dir = g_mkdtemp(notes_dir_tmpl);
    CHECK(notes_dir != NULL);

    new test::TagManager;
    test::NoteManager manager(notes_dir);
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

