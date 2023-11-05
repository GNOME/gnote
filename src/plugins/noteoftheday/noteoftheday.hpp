/*
 * gnote
 *
 * Copyright (C) 2014,2017,2023 Aurimas Cernius
 * Copyright (C) 2009 Debarshi Ray
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

#ifndef __NOTE_OF_THE_DAY_HPP_
#define __NOTE_OF_THE_DAY_HPP_

#include <glibmm/date.h>

#include "sharp/dynamicmodule.hpp"
#include "applicationaddin.hpp"
#include "note.hpp"

namespace noteoftheday {

class NoteOfTheDay
{
public:

  static gnote::NoteBase::ORef create(gnote::NoteManagerBase & manager, const Glib::Date & date);
  static void cleanup_old(gnote::NoteManagerBase & manager);
  static Glib::ustring get_content(const Glib::Date & date, const gnote::NoteManagerBase & manager);
  static gnote::NoteBase::ORef get_note_by_date(gnote::NoteManagerBase & manager, const Glib::Date & date);
  static Glib::ustring get_template_content(const Glib::ustring & title);
  static Glib::ustring get_title(const Glib::Date & date);
  static bool has_changed(gnote::NoteBase & note);

  static const Glib::ustring s_template_title;

private:

  static Glib::ustring get_content_without_title(const Glib::ustring & content);

  static const Glib::ustring s_title_prefix;
};

}

#endif
