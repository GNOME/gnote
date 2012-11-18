/*
 * gnote
 *
 * Copyright (C) 2011-2012 Aurimas Cernius
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




#ifndef __PREFERENCES_HPP_
#define __PREFERENCES_HPP_

#include <map>
#include <string>
#include <sigc++/signal.h>
#include <giomm/settings.h>

#include "base/singleton.hpp"

namespace gnote {

  class Preferences 
    : public base::Singleton<Preferences>
  {
  public:
    static const char *SCHEMA_GNOTE;
    static const char *SCHEMA_KEYBINDINGS;
    static const char *SCHEMA_SYNC;
    static const char *SCHEMA_SYNC_WDFS;
    static const char *SCHEMA_DESKTOP_GNOME_INTERFACE;

    static const char *ENABLE_SPELLCHECKING;
    static const char *ENABLE_WIKIWORDS;
    static const char *ENABLE_CUSTOM_FONT;
    static const char *ENABLE_KEYBINDINGS;
    static const char *ENABLE_AUTO_BULLETED_LISTS;
    static const char *ENABLE_ICON_PASTE;
    static const char *ENABLE_CLOSE_NOTE_ON_ESCAPE;

    static const char *START_NOTE_URI;
    static const char *CUSTOM_FONT_FACE;
    static const char *MENU_NOTE_COUNT;
    static const char *MENU_PINNED_NOTES;

    static const char *NOTE_RENAME_BEHAVIOR;
    static const char *USE_STATUS_ICON;

    static const char *MAIN_WINDOW_MAXIMIZED;
    static const char *SEARCH_WINDOW_X_POS;
    static const char *SEARCH_WINDOW_Y_POS;
    static const char *SEARCH_WINDOW_WIDTH;
    static const char *SEARCH_WINDOW_HEIGHT;
    static const char *SEARCH_WINDOW_SPLITTER_POS;

    static const char *KEYBINDING_SHOW_NOTE_MENU;
    static const char *KEYBINDING_OPEN_START_HERE;
    static const char *KEYBINDING_CREATE_NEW_NOTE;
    static const char *KEYBINDING_OPEN_SEARCH;
    static const char *KEYBINDING_OPEN_RECENT_CHANGES;

    static const char *SYNC_CLIENT_ID;
    static const char *SYNC_LOCAL_PATH;
    static const char *SYNC_SELECTED_SERVICE_ADDIN;
    static const char *SYNC_CONFIGURED_CONFLICT_BEHAVIOR;
    static const char *SYNC_AUTOSYNC_TIMEOUT;

    static const char *SYNC_FUSE_MOUNT_TIMEOUT;
    static const char *SYNC_FUSE_WDFS_ACCEPT_SSLCERT;
    static const char *SYNC_FUSE_WDFS_URL;
    static const char *SYNC_FUSE_WDFS_USERNAME;

    static const char *DESKTOP_GNOME_FONT;
    static const char *DESKTOP_GNOME_KEY_THEME;

    Preferences();

    Glib::RefPtr<Gio::Settings> get_schema_settings(const std::string & schema);
  private:
    Preferences(const Preferences &); // non implemented
    std::map<std::string, Glib::RefPtr<Gio::Settings> > m_schemas;
  };


}


#endif
