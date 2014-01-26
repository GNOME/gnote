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


#ifndef _ADDINS_NOTE_DIRECTORY_WATCHER_PREFERENCES_
#define _ADDINS_NOTE_DIRECTORY_WATCHER_PREFERENCES_

#include <gtkmm/grid.h>
#include <gtkmm/spinbutton.h>

#include "notemanager.hpp"


namespace notedirectorywatcher {

extern const char *SCHEMA_NOTE_DIRECTORY_WATCHER;
extern const char *CHECK_INTERVAL;


class NoteDirectoryWatcherPreferences
  : public Gtk::Grid
{
public:
  NoteDirectoryWatcherPreferences(gnote::NoteManager &);
private:
  void on_interval_changed();

  Gtk::SpinButton m_check_interval;
};

}

#endif

