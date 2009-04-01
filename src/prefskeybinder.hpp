/*
 * gnote
 *
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



#ifndef _PREFSKEYBINDER_HPP_
#define _PREFSKEYBINDER_HPP_

#include <memory>

namespace gnote {

class TrayIcon;
class NoteManager;


class PrefsKeybinder
{
public:
  typedef std::tr1::shared_ptr<PrefsKeybinder> Ptr;
	PrefsKeybinder(NoteManager & manager, TrayIcon & trayicon)
		: m_manager(manager)
		, m_trayicon(trayicon)
		{
		}
private:
	NoteManager & m_manager;
	TrayIcon & m_trayicon;
};

}

#endif
