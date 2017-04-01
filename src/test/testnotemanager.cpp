/*
 * gnote
 *
 * Copyright (C) 2014,2017 Aurimas Cernius
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

#include "testnote.hpp"
#include "testnotemanager.hpp"

namespace test {

Glib::ustring NoteManager::test_notes_dir()
{
  char notes_dir_tmpl[] = "/tmp/gnotetestnotesXXXXXX";
  char *notes_dir = g_mkdtemp(notes_dir_tmpl);
  return Glib::ustring(notes_dir) + "/notes";
}


NoteManager::NoteManager(const Glib::ustring & notesdir)
  : gnote::NoteManagerBase(notesdir)
{
  Glib::ustring backup = notesdir + "/Backup";
  _common_init(notesdir, backup);
}

gnote::NoteBase::Ptr NoteManager::note_create_new(const Glib::ustring & title, const Glib::ustring & file_name)
{
  gnote::NoteData *note_data = new gnote::NoteData(gnote::NoteBase::url_from_path(file_name));
  note_data->title() = title;
  sharp::DateTime date(sharp::DateTime::now());
  note_data->create_date() = date;
  note_data->set_change_date(date);

  return Note::Ptr(new Note(note_data, file_name, *this));
}

gnote::NoteBase::Ptr NoteManager::note_load(const Glib::ustring & file_name)
{
  return gnote::NoteBase::Ptr();
}

}

