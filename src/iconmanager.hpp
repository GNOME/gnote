/*
 * gnote
 *
 * Copyright (C) 2012-2013 Aurimas Cernius
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

#include <map>
#include <string>

#include <gdkmm/pixbuf.h>
#include <glibmm/refptr.h>
#include <gtkmm/iconinfo.h>

#include "base/singleton.hpp"


namespace gnote {

class IconManager
  : public base::Singleton<IconManager>
{
public:
  static const char *BUG;
  static const char *EMBLEM_PACKAGE;
  static const char *FILTER_NOTE_ALL;
  static const char *FILTER_NOTE_UNFILED;
  static const char *GNOTE;
  static const char *NOTE;
  static const char *NOTE_NEW;
  static const char *NOTEBOOK;
  static const char *NOTEBOOK_NEW;
  static const char *PIN_ACTIVE;
  static const char *PIN_DOWN;
  static const char *PIN_UP;

  Glib::RefPtr<Gdk::Pixbuf> get_icon(const std::string &, int);
  Gtk::IconInfo lookup_icon(const std::string &, int);
private:
  typedef std::pair<std::string, int> IconDef;
  typedef std::map<IconDef, Glib::RefPtr<Gdk::Pixbuf> > IconMap;

  IconMap m_icons;

  static IconManager s_obj;
};

}

