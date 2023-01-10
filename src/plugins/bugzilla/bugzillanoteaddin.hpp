/*
 * gnote
 *
 * Copyright (C) 2010,2013,2017,2019,2023 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
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




#ifndef _BUGZILLA_NOTE_ADDIN_HPP__
#define _BUGZILLA_NOTE_ADDIN_HPP__


#include "sharp/dynamicmodule.hpp"
#include "noteaddin.hpp"

namespace bugzilla {


class BugzillaModule
  : public sharp::DynamicModule
{
public:
  BugzillaModule();
};

class BugzillaNoteAddin
  : public gnote::NoteAddin
{
public:
  static BugzillaNoteAddin* create()
    {
      return new BugzillaNoteAddin;
    }
  static Glib::ustring images_dir();
  virtual void initialize() override;
  virtual void shutdown() override;
  virtual void on_note_opened() override;
private:
  BugzillaNoteAddin();
  void migrate_images(const Glib::ustring & old_images_dir);

  static const char * TAG_NAME;

  bool drop_string(const Glib::ustring & line, int x, int y);
  bool insert_bug (int x, int y, const Glib::ustring & uri, int id);
};


}


#endif
