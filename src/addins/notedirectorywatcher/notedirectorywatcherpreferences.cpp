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


#include <glibmm/i18n.h>

#include "notedirectorywatcherpreferences.hpp"
#include "preferences.hpp"

namespace notedirectorywatcher {

const char *SCHEMA_NOTE_DIRECTORY_WATCHER = "org.gnome.gnote.note-directory-watcher";
const char *CHECK_INTERVAL = "check-interval";


NoteDirectoryWatcherPreferences::NoteDirectoryWatcherPreferences(gnote::NoteManager &)
  : m_check_interval(1)
{
  Gtk::Label *label = manage(new Gtk::Label(_("_Directory check interval:"), true));
  attach(*label, 0, 0, 1, 1);
  m_check_interval.set_range(5, 300);
  m_check_interval.set_increments(1, 5);
  m_check_interval.signal_value_changed()
    .connect(sigc::mem_fun(*this, &NoteDirectoryWatcherPreferences::on_interval_changed));
  m_check_interval.set_value(
    gnote::Preferences::obj().get_schema_settings(SCHEMA_NOTE_DIRECTORY_WATCHER)->get_int(CHECK_INTERVAL));
  attach(m_check_interval, 1, 0, 1, 1);
}

void NoteDirectoryWatcherPreferences::on_interval_changed()
{
  gnote::Preferences::obj().get_schema_settings(SCHEMA_NOTE_DIRECTORY_WATCHER)->set_int(
    CHECK_INTERVAL, m_check_interval.get_value_as_int());
}

}

