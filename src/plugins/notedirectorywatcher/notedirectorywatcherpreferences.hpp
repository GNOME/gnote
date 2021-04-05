/*
 * gnote
 *
 * Copyright (C) 2014,2019-2020 Aurimas Cernius
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


namespace gnote {
  class IGnote;
}

namespace notedirectorywatcher {

extern const char *SCHEMA_NOTE_DIRECTORY_WATCHER;
extern const char *CHECK_INTERVAL;


class NoteDirectoryWatcherPreferences
  : public Gtk::Grid
{
public:
  static Glib::RefPtr<Gio::Settings> & settings();
  NoteDirectoryWatcherPreferences(gnote::IGnote &, gnote::Preferences &, gnote::NoteManager &);
private:
  static Glib::RefPtr<Gio::Settings> s_settings;

  void on_interval_changed();

  Gtk::SpinButton m_check_interval;
};

}

#endif

