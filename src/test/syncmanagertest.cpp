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
#include <iostream>

#include <boost/format.hpp>
#include <boost/test/minimal.hpp>
#include <glib/gstdio.h>

#include "notemanager.hpp"
#include "testnotemanager.hpp"
#include "testsyncmanager.hpp"
#include "testtagmanager.hpp"
#include "synchronization/silentui.hpp"

using namespace gnote;


void create_note(test::NoteManager & manager, const std::string & title, const std::string & body)
{
  std::string content = str(boost::format("<note-content><note-title>%1%</note-title>\n\n%2%</note-content>")
                            % title % body);
  manager.create(title, content);
}

int test_main(int /*argc*/, char ** /*argv*/)
{
  char notes_dir_tmpl[] = "/tmp/gnotetestnotesXXXXXX";
  char notes_dir_tmpl2[] = "/tmp/gnotetestnotesXXXXXX";
  char *notes_dir = g_mkdtemp(notes_dir_tmpl);
  BOOST_CHECK(notes_dir != NULL);
  char *notes_dir2 = g_mkdtemp(notes_dir_tmpl2);
  BOOST_CHECK(notes_dir2 != NULL);
  std::string notesdir = std::string(notes_dir) + "/notes";
  std::string notesdir2 = std::string(notes_dir2) + "/notes";
  std::string syncdir = std::string(notes_dir) + "/sync";
  if(g_mkdir(syncdir.c_str(), 0x755)) {
    std::cerr << "Failed to create test directory: " << syncdir << std::endl;
    return 1;
  }
  std::string manifest = std::string(notes_dir) + "/manifest.xml";

  new test::TagManager;
  test::NoteManager manager1(notes_dir);
  create_note(manager1, "note1", "content1");
  create_note(manager1, "note2", "content2");
  create_note(manager1, "note3", "content3");

  test::NoteManager manager2(notesdir2);

  test::SyncManager sync_manager1(manager1);
  test::SyncManager sync_manager2(manager2);
  test::SyncClient::Ptr sync_client1 = dynamic_pointer_cast<test::SyncClient>(sync_manager1.get_client(manifest));
  gnote::sync::SilentUI::Ptr sync_ui = gnote::sync::SilentUI::create(manager1);
  //sync_manager1.perform_synchronization(sync_ui); //TODO: fails, return proper server from test SyncAddin

  return 0;
}

