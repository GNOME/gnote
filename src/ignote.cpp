/*
 * gnote
 *
 * Copyright (C) 2013,2017 Aurimas Cernius
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


#include <glibmm/miscutils.h>

#include "ignote.hpp"


namespace gnote {

IGnote::~IGnote()
{}

Glib::ustring IGnote::cache_dir()
{
  return Glib::get_user_cache_dir() + "/gnote";
}

Glib::ustring IGnote::conf_dir()
{
  return Glib::get_user_config_dir() + "/gnote";
}

Glib::ustring IGnote::data_dir()
{
  return Glib::get_user_data_dir() + "/gnote";
}

Glib::ustring IGnote::old_note_dir()
{
  Glib::ustring home_dir = Glib::get_home_dir();

  if(home_dir.empty()) {
    home_dir = Glib::get_current_dir();
  }

  return home_dir + "/.gnote";
}

}

