/*
 * gnote
 *
 * Copyright (C) 2011-2015,2017,2019-2020 Aurimas Cernius
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

const char *SCHEMA_DESKTOP_GNOME_INTERFACE = "org.gnome.desktop.interface";
const char *SCHEMA_SYNC = "org.gnome.gnote.sync";
const char *SCHEMA_SYNC_WDFS = "org.gnome.gnote.sync.wdfs";

const Glib::ustring ENABLE_SPELLCHECKING = "enable-spellchecking";

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


  const char * Preferences::SCHEMA_GNOTE = "org.gnome.gnote";

  const char * Preferences::ENABLE_AUTO_LINKS = "enable-auto-links";
  const char * Preferences::ENABLE_URL_LINKS = "enable-url-links";
  const char * Preferences::ENABLE_WIKIWORDS = "enable-wikiwords";
  const char * Preferences::ENABLE_CUSTOM_FONT = "enable-custom-font";
  const char * Preferences::ENABLE_AUTO_BULLETED_LISTS = "enable-bulleted-lists";
  const char * Preferences::ENABLE_ICON_PASTE = "enable-icon-paste";
  const char * Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE = "enable-close-note-on-escape";

  const char * Preferences::START_NOTE_URI = "start-note";
  const char * Preferences::CUSTOM_FONT_FACE = "custom-font-face";
  const char * Preferences::MENU_NOTE_COUNT = "menu-note-count";
  const char * Preferences::MENU_PINNED_NOTES = "menu-pinned-notes";

  const char * Preferences::NOTE_RENAME_BEHAVIOR = "note-rename-behavior";
  const char * Preferences::USE_STATUS_ICON = "use-status-icon";
  const char * Preferences::OPEN_NOTES_IN_NEW_WINDOW = "open-notes-in-new-window";
  const char * Preferences::AUTOSIZE_NOTE_WINDOW = "autosize-note-window";
  const char * Preferences::USE_CLIENT_SIDE_DECORATIONS = "use-client-side-decorations";

  const char * Preferences::MAIN_WINDOW_MAXIMIZED = "main-window-maximized";
  const char * Preferences::SEARCH_WINDOW_WIDTH = "search-window-width";
  const char * Preferences::SEARCH_WINDOW_HEIGHT = "search-window-height";
  const char * Preferences::SEARCH_WINDOW_SPLITTER_POS = "search-window-splitter-pos";
  const char * Preferences::SEARCH_SORTING = "search-sorting";


  void Preferences::init()
  {
    m_schema_gnote = Gio::Settings::create(SCHEMA_GNOTE);
    m_schemas[SCHEMA_GNOTE] = m_schema_gnote;
    m_schema_gnome_interface = Gio::Settings::create(SCHEMA_DESKTOP_GNOME_INTERFACE);
    m_schema_sync = Gio::Settings::create(SCHEMA_SYNC);
    m_schema_sync_wdfs = Gio::Settings::create(SCHEMA_SYNC_WDFS);

    SETUP_CACHED_KEY(m_schema_gnote, enable_spellchecking, ENABLE_SPELLCHECKING, boolean);
    SETUP_CACHED_KEY(m_schema_gnote, enable_auto_links, ENABLE_AUTO_LINKS, boolean);
    SETUP_CACHED_KEY(m_schema_gnote, enable_url_links, ENABLE_URL_LINKS, boolean);
    SETUP_CACHED_KEY(m_schema_gnote, enable_wikiwords, ENABLE_WIKIWORDS, boolean);

    SETUP_CACHED_KEY(m_schema_gnome_interface, desktop_gnome_clock_format, DESKTOP_GNOME_CLOCK_FORMAT, string);
    SETUP_CACHED_KEY(m_schema_gnome_interface, desktop_gnome_font, DESKTOP_GNOME_FONT, string);

    SETUP_CACHED_KEY(m_schema_sync, sync_selected_service_addin, SYNC_SELECTED_SERVICE_ADDIN, string);
    SETUP_CACHED_KEY(m_schema_sync, sync_autosync_timeout, SYNC_AUTOSYNC_TIMEOUT, int);
  }
  
  Glib::RefPtr<Gio::Settings> Preferences::get_schema_settings(const Glib::ustring & schema)
  {
    auto iter = m_schemas.find(schema);
    if(iter != m_schemas.end()) {
      return iter->second;
    }

    Glib::RefPtr<Gio::Settings> settings = Gio::Settings::create(schema);
    if(settings) {
      m_schemas[schema] = settings;
    }

    return settings;
  }

  DEFINE_CACHING_SETTER_BOOL(m_schema_gnote, enable_spellchecking, ENABLE_SPELLCHECKING)
  DEFINE_CACHING_SETTER_BOOL(m_schema_gnote, enable_auto_links, ENABLE_AUTO_LINKS)
  DEFINE_CACHING_SETTER_BOOL(m_schema_gnote, enable_url_links, ENABLE_URL_LINKS)
  DEFINE_CACHING_SETTER_BOOL(m_schema_gnote, enable_wikiwords, ENABLE_WIKIWORDS)
  DEFINE_GETTER_SETTER_BOOL(m_schema_gnote, open_notes_in_new_window, OPEN_NOTES_IN_NEW_WINDOW)
  DEFINE_GETTER_SETTER_BOOL(m_schema_gnote, enable_custom_font, ENABLE_CUSTOM_FONT)
  DEFINE_GETTER_SETTER_BOOL(m_schema_gnote, enable_auto_bulleted_lists, ENABLE_AUTO_BULLETED_LISTS)

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

