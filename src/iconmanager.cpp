/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017,2019,2021 Aurimas Cernius
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
#include <gtkmm/icontheme.h>

#include "debug.hpp"
#include "iconmanager.hpp"



namespace gnote {

const char *IconManager::BUG = "bug";
const char *IconManager::EMBLEM_PACKAGE = "emblem-package";
const char *IconManager::FILTER_NOTE_ALL = "filter-note-all";
const char *IconManager::FILTER_NOTE_UNFILED = "filter-note-unfiled";
const char *IconManager::GNOTE = "org.gnome.Gnote";
const char *IconManager::NOTE = "note";
const char *IconManager::NOTE_NEW = "note-new";
const char *IconManager::NOTEBOOK = "notebook";
const char *IconManager::NOTEBOOK_NEW = "notebook-new";
const char *IconManager::PIN_ACTIVE = "pin-active";
const char *IconManager::PIN_DOWN = "pin-down";
const char *IconManager::PIN_UP = "pin-up";
const char *IconManager::ACTIVE_NOTES = "active-notes";
const char *IconManager::SPECIAL_NOTES = "special-notes";


Glib::RefPtr<Gdk::Pixbuf> IconManager::get_icon(const Glib::ustring & name, int size)
{
  try {
    IconDef icon = std::make_pair(name, size);
    IconMap::iterator iter = m_icons.find(icon);
    if(iter != m_icons.end()) {
      return iter->second;
    }

    Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gtk::IconTheme::get_default()->load_icon(
        name, size, (Gtk::IconLookupFlags) 0);
    m_icons[icon] = pixbuf;
    return pixbuf;
  }
  catch(const Glib::Exception & e) {
    /* TRANSLATORS: the first %s is icon name, %d is for size, last %s is error description */
    ERR_OUT(_("Failed to load icon (%s, %d): %s"), name.c_str(), size, e.what().c_str());
  }
  return Glib::RefPtr<Gdk::Pixbuf>();
}

Gtk::IconInfo IconManager::lookup_icon(const Glib::ustring & name, int size)
{
  return Gtk::IconTheme::get_default()->lookup_icon(name, size, (Gtk::IconLookupFlags) 0);
}

}

