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



#include "preferences.hpp"



namespace gnote {


  const char * Preferences::SCHEMA_GNOTE = "org.gnome.gnote";
  const char * Preferences::SCHEMA_KEYBINDINGS = "org.gnome.gnote.global-keybindings";
  const char * Preferences::SCHEMA_SYNC = "org.gnome.gnote.sync";
  const char * Preferences::SCHEMA_SYNC_WDFS = "org.gnome.gnote.sync.wdfs";
  const char * Preferences::SCHEMA_DESKTOP_GNOME_INTERFACE = "org.gnome.desktop.interface";

  const char * Preferences::ENABLE_SPELLCHECKING = "enable-spellchecking";
  const char * Preferences::ENABLE_WIKIWORDS = "enable-wikiwords";
  const char * Preferences::ENABLE_CUSTOM_FONT = "enable-custom-font";
  const char * Preferences::ENABLE_KEYBINDINGS = "enable-keybindings";
  const char * Preferences::ENABLE_AUTO_BULLETED_LISTS = "enable-bulleted-lists";
  const char * Preferences::ENABLE_ICON_PASTE = "enable-icon-paste";
  const char * Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE = "enable-close-note-on-escape";

  const char * Preferences::START_NOTE_URI = "start-note";
  const char * Preferences::CUSTOM_FONT_FACE = "custom-font-face";
  const char * Preferences::MENU_NOTE_COUNT = "menu-note-count";
  const char * Preferences::MENU_PINNED_NOTES = "menu-pinned-notes";

  const char * Preferences::KEYBINDING_SHOW_NOTE_MENU = "show-note-menu";
  const char * Preferences::KEYBINDING_OPEN_START_HERE = "open-start-here";
  const char * Preferences::KEYBINDING_CREATE_NEW_NOTE = "create-new-note";
  const char * Preferences::KEYBINDING_OPEN_SEARCH = "open-search";
  const char * Preferences::KEYBINDING_OPEN_RECENT_CHANGES = "open-recent-changes";

  const char * Preferences::SYNC_CLIENT_ID = "sync-guid";
  const char * Preferences::SYNC_LOCAL_PATH = "sync-local-path";
  const char * Preferences::SYNC_SELECTED_SERVICE_ADDIN = "sync-selected-service-addin";
  const char * Preferences::SYNC_CONFIGURED_CONFLICT_BEHAVIOR = "sync-conflict-behavior";
  const char * Preferences::SYNC_AUTOSYNC_TIMEOUT = "autosync-timeout";

  const char * Preferences::NOTE_RENAME_BEHAVIOR = "note-rename-behavior";
  const char * Preferences::USE_STATUS_ICON = "use-status-icon";

  const char * Preferences::MAIN_WINDOW_MAXIMIZED = "main-window-maximized";
  const char * Preferences::SEARCH_WINDOW_X_POS = "search-window-x-pos";
  const char * Preferences::SEARCH_WINDOW_Y_POS = "search-window-y-pos";
  const char * Preferences::SEARCH_WINDOW_WIDTH = "search-window-width";
  const char * Preferences::SEARCH_WINDOW_HEIGHT = "search-window-height";
  const char * Preferences::SEARCH_WINDOW_SPLITTER_POS = "search-window-splitter-pos";

  const char * Preferences::SYNC_FUSE_MOUNT_TIMEOUT = "sync-fuse-mount-timeout-ms";
  const char * Preferences::SYNC_FUSE_WDFS_ACCEPT_SSLCERT = "accept-sslcert";
  const char * Preferences::SYNC_FUSE_WDFS_URL = "url";
  const char * Preferences::SYNC_FUSE_WDFS_USERNAME = "username";

  const char * Preferences::DESKTOP_GNOME_FONT = "document-font-name";
  const char * Preferences::DESKTOP_GNOME_KEY_THEME = "gtk-key-theme";


  Preferences::Preferences()
  {
    m_schemas[SCHEMA_GNOTE] = Gio::Settings::create(SCHEMA_GNOTE);
    m_schemas[SCHEMA_KEYBINDINGS] = Gio::Settings::create(SCHEMA_KEYBINDINGS);
  }
  
  Glib::RefPtr<Gio::Settings> Preferences::get_schema_settings(const std::string & schema)
  {
    std::map<std::string, Glib::RefPtr<Gio::Settings> >::iterator iter = m_schemas.find(schema);
    if(iter != m_schemas.end()) {
      return iter->second;
    }

    Glib::RefPtr<Gio::Settings> settings = Gio::Settings::create(schema);
    if(settings) {
      m_schemas[schema] = settings;
    }

    return settings;
  }

}
