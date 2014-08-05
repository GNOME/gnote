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

#include "base/macros.hpp"
#include "notemanagerbase.hpp"


namespace test {

class NoteManager
  : public gnote::NoteManagerBase
{
public:
  static Glib::ustring test_notes_dir();

  explicit NoteManager(const Glib::ustring & notes_dir);
protected:
  virtual gnote::NoteBase::Ptr note_create_new(const Glib::ustring & title, const Glib::ustring & file_name) override;
  virtual gnote::NoteBase::Ptr note_load(const Glib::ustring & file_name) override;
};

}
