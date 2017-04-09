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


#include <cstdio>
#include <iostream>

#include <glib/gstdio.h>
#include <UnitTest++/UnitTest++.h>

#include "notemanager.hpp"
#include "synchronization/silentui.hpp"
#include "test/testnotemanager.hpp"
#include "test/testsyncmanager.hpp"
#include "test/testtagmanager.hpp"

using namespace gnote;


void create_note(test::NoteManager & manager, const Glib::ustring & title, const Glib::ustring & body)
{
  Glib::ustring content = Glib::ustring::compose("<note-content><note-title>%1</note-title>\n\n%2</note-content>",
                            title, body);
  manager.create(title, content);
}

TEST(clean_sync)
{
  char notes_dir_tmpl[] = "/tmp/gnotetestnotesXXXXXX";
  char notes_dir_tmpl2[] = "/tmp/gnotetestnotesXXXXXX";
  char *notes_dir = g_mkdtemp(notes_dir_tmpl);
  CHECK(notes_dir != NULL);
  char *notes_dir2 = g_mkdtemp(notes_dir_tmpl2);
  CHECK(notes_dir2 != NULL);
  Glib::ustring notesdir = Glib::ustring(notes_dir) + "/notes";
  Glib::ustring notesdir2 = Glib::ustring(notes_dir2) + "/notes";
  Glib::ustring syncdir = Glib::ustring(notes_dir) + "/sync";
  REQUIRE CHECK(g_mkdir(syncdir.c_str(), 0x755) == 0);
  Glib::ustring manifest = Glib::ustring(notes_dir) + "/manifest.xml";

  new test::TagManager;
  test::NoteManager manager1(notes_dir);
  create_note(manager1, "note1", "content1");
  create_note(manager1, "note2", "content2");
  create_note(manager1, "note3", "content3");

  test::NoteManager manager2(notesdir2);

  test::SyncManager sync_manager1(manager1, syncdir);
  test::SyncManager sync_manager2(manager2, syncdir);
  test::SyncClient::Ptr sync_client1 = dynamic_pointer_cast<test::SyncClient>(sync_manager1.get_client(manifest));
  gnote::sync::SilentUI::Ptr sync_ui = gnote::sync::SilentUI::create(manager1);
  sync_manager1.perform_synchronization(sync_ui);
  // TODO: results in various errors and sync actually fails
}

