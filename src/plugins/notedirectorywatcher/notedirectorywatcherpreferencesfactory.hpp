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


#ifndef _ADDINS_NOTE_DIRECTORY_WATCHER_PREFERENCES_FACTORY_
#define _ADDINS_NOTE_DIRECTORY_WATCHER_PREFERENCES_FACTORY_

#include "addinpreferencefactory.hpp"
#include "notedirectorywatcherpreferences.hpp"

namespace notedirectorywatcher {

class NoteDirectoryWatcherPreferencesFactory
  : public gnote::AddinPreferenceFactory<NoteDirectoryWatcherPreferences>
{};

}

#endif

