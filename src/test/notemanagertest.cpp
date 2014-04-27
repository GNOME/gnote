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


#include <boost/test/minimal.hpp>

#include "testnotemanager.hpp"
#include "testtagmanager.hpp"


int test_main(int /*argc*/, char ** /*argv*/)
{
  char notes_dir_tmpl[] = "/tmp/gnotetestnotesXXXXXX";
  char *notes_dir = g_mkdtemp(notes_dir_tmpl);
  BOOST_CHECK(notes_dir != NULL);

  new test::TagManager;
  test::NoteManager manager(notes_dir);
  manager.create();
  manager.create();
  gnote::NoteBase::Ptr test_note = manager.create("test note");
  BOOST_CHECK(test_note != 0);
  // 3 notes + template note
  BOOST_CHECK(manager.get_notes().size() == 4);
  BOOST_CHECK(manager.find("test note") == test_note);
  BOOST_CHECK(manager.find_by_uri(test_note->uri()) == test_note);

  return 0;
}

