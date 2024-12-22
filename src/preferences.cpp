/*
 * gnote
 *
 * Copyright (C) 2011-2015,2017,2019-2021,2024 Aurimas Cernius
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

#define SETUP_CACHED_KEY(schema, key, KEY, type) \
  do { \
    schema->signal_changed(KEY).connect([this](const Glib::ustring &) { \
      m_##key = schema->get_##type(KEY); \
      signal_##key##_changed(); \
    }); \
    m_##key = schema->get_##type(KEY); \
  } while(0)


#define DEFINE_GETTER(schema, key, KEY, type, rettype, paramtype) \
  rettype Preferences::key() const \
  { \
    return schema->get_##type(KEY); \
  }

#define DEFINE_GETTER_BOOL(schema, key, KEY) DEFINE_GETTER(schema, key, KEY, boolean, bool, bool)
#define DEFINE_GETTER_STRING(schema, key, KEY) DEFINE_GETTER(schema, key, KEY, string, Glib::ustring, const Glib::ustring)


#define DEFINE_GETTER_SETTER(schema, key, KEY, type, rettype, paramtype) \
  DEFINE_GETTER(schema, key, KEY, type, rettype, paramtype) \
  void Preferences::key(paramtype value) \
  { \
    schema->set_##type(KEY, value); \
  }

#define DEFINE_GETTER_SETTER_BOOL(schema, key, KEY) DEFINE_GETTER_SETTER(schema, key, KEY, boolean, bool, bool)
#define DEFINE_GETTER_SETTER_INT(schema, key, KEY) DEFINE_GETTER_SETTER(schema, key, KEY, int, int, int)
#define DEFINE_GETTER_SETTER_STRING(schema, key, KEY) DEFINE_GETTER_SETTER(schema, key, KEY, string, Glib::ustring, const Glib::ustring &)


#define DEFINE_CACHING_SETTER(schema, key, KEY, type, cpptype) \
  void Preferences::key(cpptype value) \
  { \
    m_##key = value; \
    schema->set_##type(KEY, value); \
  }

#define DEFINE_CACHING_SETTER_BOOL(schema, key, KEY) DEFINE_CACHING_SETTER(schema, key, KEY, boolean, bool)
#define DEFINE_CACHING_SETTER_INT(schema, key, KEY) DEFINE_CACHING_SETTER(schema, key, KEY, int, int)
#define DEFINE_CACHING_SETTER_STRING(schema, key, KEY) DEFINE_CACHING_SETTER(schema, key, KEY, string, const Glib::ustring &)


namespace {

const char *SCHEMA_GNOTE = "org.gnome.gnote";
const char *SCHEMA_DESKTOP_GNOME_INTERFACE = "org.gnome.desktop.interface";
const char *SCHEMA_SYNC = "org.gnome.gnote.sync";
const char *SCHEMA_SYNC_WDFS = "org.gnome.gnote.sync.wdfs";

const Glib::ustring ENABLE_SPELLCHECKING = "enable-spellchecking";
const Glib::ustring ENABLE_AUTO_LINKS = "enable-auto-links";
const Glib::ustring ENABLE_URL_LINKS = "enable-url-links";
const Glib::ustring ENABLE_WIKIWORDS = "enable-wikiwords";
const Glib::ustring ENABLE_CUSTOM_FONT = "enable-custom-font";
const Glib::ustring HIGHLIGH_BACKGROUND_COLOR = "highlight-background-color";
const Glib::ustring HIGHLIGH_FOREGROUND_COLOR = "highlight-foreground-color";
const Glib::ustring ENABLE_AUTO_BULLETED_LISTS = "enable-bulleted-lists";
//const Glib::ustring ENABLE_ICON_PASTE = "enable-icon-paste";  NOT USED CURRENTLY
const Glib::ustring ENABLE_CLOSE_NOTE_ON_ESCAPE = "enable-close-note-on-escape";
const Glib::ustring NOTE_RENAME_BEHAVIOR = "note-rename-behavior";
const Glib::ustring START_NOTE_URI = "start-note";
const Glib::ustring CUSTOM_FONT_FACE = "custom-font-face";
const Glib::ustring MENU_PINNED_NOTES = "menu-pinned-notes";
const Glib::ustring OPEN_NOTES_IN_NEW_WINDOW = "open-notes-in-new-window";
const Glib::ustring AUTOSIZE_NOTE_WINDOW = "autosize-note-window";
const Glib::ustring MAIN_WINDOW_MAXIMIZED = "main-window-maximized";
const Glib::ustring SEARCH_WINDOW_WIDTH = "search-window-width";
const Glib::ustring SEARCH_WINDOW_HEIGHT = "search-window-height";
const Glib::ustring SEARCH_WINDOW_SPLITTER_POS = "search-window-splitter-pos";
const Glib::ustring SEARCH_SORTING = "search-sorting";
const Glib::ustring USE_CLIENT_SIDE_DECORATIONS = "use-client-side-decorations";
const Glib::ustring COLOR_SCHEME = "color-scheme";

const Glib::ustring DESKTOP_GNOME_CLOCK_FORMAT = "clock-format";
const Glib::ustring DESKTOP_GNOME_FONT = "document-font-name";

const Glib::ustring SYNC_CLIENT_ID = "sync-guid";
const Glib::ustring SYNC_LOCAL_PATH = "sync-local-path";
const Glib::ustring SYNC_SELECTED_SERVICE_ADDIN = "sync-selected-service-addin";
const Glib::ustring SYNC_CONFIGURED_CONFLICT_BEHAVIOR = "sync-conflict-behavior";
const Glib::ustring SYNC_AUTOSYNC_TIMEOUT = "autosync-timeout";

const Glib::ustring SYNC_FUSE_MOUNT_TIMEOUT = "sync-fuse-mount-timeout-ms";
const Glib::ustring SYNC_FUSE_WDFS_ACCEPT_SSLCERT = "accept-sslcert";
const Glib::ustring SYNC_FUSE_WDFS_URL = "url";
const Glib::ustring SYNC_FUSE_WDFS_USERNAME = "username";

}

namespace gnote {

  const char *Preferences::COLOR_SCHEME_DARK_VAL = "dark";
  const char *Preferences::COLOR_SCHEME_LIGHT_VAL = "light";

  void Preferences::init()
  {
    m_schema_gnote = Gio::Settings::create(SCHEMA_GNOTE);
    m_schema_gnome_interface = Gio::Settings::create(SCHEMA_DESKTOP_GNOME_INTERFACE);
    m_schema_sync = Gio::Settings::create(SCHEMA_SYNC);
    m_schema_sync_wdfs = Gio::Settings::create(SCHEMA_SYNC_WDFS);

    SETUP_CACHED_KEY(m_schema_gnote, enable_spellchecking, ENABLE_SPELLCHECKING, boolean);
    SETUP_CACHED_KEY(m_schema_gnote, enable_auto_links, ENABLE_AUTO_LINKS, boolean);
    SETUP_CACHED_KEY(m_schema_gnote, enable_url_links, ENABLE_URL_LINKS, boolean);
    SETUP_CACHED_KEY(m_schema_gnote, enable_wikiwords, ENABLE_WIKIWORDS, boolean);
    SETUP_CACHED_KEY(m_schema_gnote, enable_custom_font, ENABLE_CUSTOM_FONT, boolean);
    SETUP_CACHED_KEY(m_schema_gnote, highlight_background_color, HIGHLIGH_BACKGROUND_COLOR, string);
    SETUP_CACHED_KEY(m_schema_gnote, highlight_foreground_color, HIGHLIGH_FOREGROUND_COLOR, string);
    SETUP_CACHED_KEY(m_schema_gnote, note_rename_behavior, NOTE_RENAME_BEHAVIOR, int);
    SETUP_CACHED_KEY(m_schema_gnote, custom_font_face, CUSTOM_FONT_FACE, string);
    SETUP_CACHED_KEY(m_schema_gnote, color_scheme, COLOR_SCHEME, string);

    SETUP_CACHED_KEY(m_schema_gnome_interface, desktop_gnome_clock_format, DESKTOP_GNOME_CLOCK_FORMAT, string);

    SETUP_CACHED_KEY(m_schema_sync, sync_selected_service_addin, SYNC_SELECTED_SERVICE_ADDIN, string);
    SETUP_CACHED_KEY(m_schema_sync, sync_autosync_timeout, SYNC_AUTOSYNC_TIMEOUT, int);
  }
  
  DEFINE_CACHING_SETTER_BOOL(m_schema_gnote, enable_spellchecking, ENABLE_SPELLCHECKING)
  DEFINE_CACHING_SETTER_BOOL(m_schema_gnote, enable_auto_links, ENABLE_AUTO_LINKS)
  DEFINE_CACHING_SETTER_BOOL(m_schema_gnote, enable_url_links, ENABLE_URL_LINKS)
  DEFINE_CACHING_SETTER_BOOL(m_schema_gnote, enable_wikiwords, ENABLE_WIKIWORDS)
  DEFINE_CACHING_SETTER_BOOL(m_schema_gnote, enable_custom_font, ENABLE_CUSTOM_FONT)
  DEFINE_CACHING_SETTER_STRING(m_schema_gnote, highlight_background_color, HIGHLIGH_BACKGROUND_COLOR)
  DEFINE_CACHING_SETTER_STRING(m_schema_gnote, highlight_foreground_color, HIGHLIGH_FOREGROUND_COLOR)
  DEFINE_GETTER_SETTER_BOOL(m_schema_gnote, enable_auto_bulleted_lists, ENABLE_AUTO_BULLETED_LISTS)
  DEFINE_CACHING_SETTER_INT(m_schema_gnote, note_rename_behavior, NOTE_RENAME_BEHAVIOR)
  DEFINE_GETTER_SETTER_STRING(m_schema_gnote, start_note_uri, START_NOTE_URI)
  DEFINE_CACHING_SETTER_STRING(m_schema_gnote, custom_font_face, CUSTOM_FONT_FACE)
  DEFINE_GETTER_SETTER_STRING(m_schema_gnote, menu_pinned_notes, MENU_PINNED_NOTES)
  DEFINE_GETTER_SETTER_BOOL(m_schema_gnote, main_window_maximized, MAIN_WINDOW_MAXIMIZED)
  DEFINE_GETTER_SETTER_INT(m_schema_gnote, search_window_width, SEARCH_WINDOW_WIDTH)
  DEFINE_GETTER_SETTER_INT(m_schema_gnote, search_window_height, SEARCH_WINDOW_HEIGHT)
  DEFINE_GETTER_SETTER_INT(m_schema_gnote, search_window_splitter_pos, SEARCH_WINDOW_SPLITTER_POS)
  DEFINE_GETTER_SETTER_STRING(m_schema_gnote, search_sorting, SEARCH_SORTING)
  DEFINE_GETTER_SETTER_STRING(m_schema_gnote, use_client_side_decorations, USE_CLIENT_SIDE_DECORATIONS)
  DEFINE_CACHING_SETTER_STRING(m_schema_gnote, color_scheme, COLOR_SCHEME)

  DEFINE_GETTER_STRING(m_schema_sync, sync_client_id, SYNC_CLIENT_ID)
  DEFINE_GETTER_SETTER_STRING(m_schema_sync, sync_local_path, SYNC_LOCAL_PATH)
  DEFINE_CACHING_SETTER_STRING(m_schema_sync, sync_selected_service_addin, SYNC_SELECTED_SERVICE_ADDIN)
  DEFINE_GETTER_SETTER_INT(m_schema_sync, sync_configured_conflict_behavior, SYNC_CONFIGURED_CONFLICT_BEHAVIOR)
  DEFINE_CACHING_SETTER_INT(m_schema_sync, sync_autosync_timeout, SYNC_AUTOSYNC_TIMEOUT)

  DEFINE_GETTER_SETTER_INT(m_schema_sync_wdfs, sync_fuse_mount_timeout, SYNC_FUSE_MOUNT_TIMEOUT)
  DEFINE_GETTER_SETTER_BOOL(m_schema_sync_wdfs, sync_fuse_wdfs_accept_sllcert, SYNC_FUSE_WDFS_ACCEPT_SSLCERT)
  DEFINE_GETTER_SETTER_STRING(m_schema_sync_wdfs, sync_fuse_wdfs_url, SYNC_FUSE_WDFS_URL)
  DEFINE_GETTER_SETTER_STRING(m_schema_sync_wdfs, sync_fuse_wdfs_username, SYNC_FUSE_WDFS_USERNAME)

}

