/*
 * gnote
 *
 * Copyright (C) 2011-2015,2017,2019-2021,2024-2026 Aurimas Cernius
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

const char *SCHEMA_REPLACE_TITLE = "org.gnome.gnote.replace-title";
const char *SCHEMA_SYNC_WDFS = "org.gnome.gnote.sync.wdfs";

//const Glib::ustring ENABLE_ICON_PASTE = "enable-icon-paste";  NOT USED CURRENTLY
const Glib::ustring ENABLE_CLOSE_NOTE_ON_ESCAPE = "enable-close-note-on-escape";
const Glib::ustring OPEN_NOTES_IN_NEW_WINDOW = "open-notes-in-new-window";
const Glib::ustring AUTOSIZE_NOTE_WINDOW = "autosize-note-window";

const Glib::ustring DESKTOP_GNOME_FONT = "document-font-name";

const Glib::ustring SYNC_AUTOSYNC_TIMEOUT = "autosync-timeout";

const Glib::ustring SYNC_FUSE_MOUNT_TIMEOUT = "sync-fuse-mount-timeout-ms";
const Glib::ustring SYNC_FUSE_WDFS_ACCEPT_SSLCERT = "accept-sslcert";
const Glib::ustring SYNC_FUSE_WDFS_URL = "url";
const Glib::ustring SYNC_FUSE_WDFS_USERNAME = "username";

const Glib::ustring REPLACE_TITLE_CLIPBOARD = "clipboard";

}

namespace gnote {

  const char *Preferences::COLOR_SCHEME_DARK_VAL = "dark";
  const char *Preferences::COLOR_SCHEME_LIGHT_VAL = "light";

  Preferences::GnoteSettings::GnoteSettings(const Glib::RefPtr<Gio::Settings> &schema)
    : enable_spellchecking(*schema, "enable-spellchecking")
    , enable_auto_links(*schema, "enable-auto-links")
    , enable_url_links(*schema, "enable-url-links")
    , enable_wikiwords(*schema, "enable-wikiwords")
    , enable_custom_font(*schema, "enable-custom-font")
    , highlight_accent_color_based(*schema, "highlight-accent-color-based")
    , note_rename_behavior(*schema, "note-rename-behavior")
    , editor_tab_width(*schema, "editor-tab-width")
    , highlight_background_color(*schema, "highlight-background-color")
    , highlight_foreground_color(*schema, "highlight-foreground-color")
    , custom_font_face(*schema, "custom-font-face")
    , color_scheme(*schema, "color-scheme")
    , enable_auto_bulleted_lists(*schema, "enable-bulleted-lists")
    , main_window_maximized(*schema, "main-window-maximized")
    , search_window_width(*schema, "search-window-width")
    , search_window_height(*schema, "search-window-height")
    , search_window_splitter_pos(*schema, "search-window-splitter-pos")
    , start_note_uri(*schema, "start-note")
    , menu_pinned_notes(*schema, "menu-pinned-notes")
    , search_sorting(*schema, "search-sorting")
    , use_client_side_decorations(*schema, "use-client-side-decorations")
    , m_schema(schema)
  {
  }

  Preferences::GnomeDesktopSettings::GnomeDesktopSettings(const Glib::RefPtr<Gio::Settings> &schema)
    : clock_format(*schema, "clock-format")
    , m_schema(schema)
  {
  }

  Preferences::SyncSettings::SyncSettings(const Glib::RefPtr<Gio::Settings> &schema)
    : selected_service_addin(*schema, "sync-selected-service-addin")
    , client_id(*schema, "sync-guid")
    , configured_conflict_behavior(*schema, "sync-conflict-behavior")
    , local_path(*schema, "sync-local-path")
    , m_schema(schema)
  {
  }


  Preferences::Preferences()
    : gnote(Gio::Settings::create("org.gnome.gnote"))
    , gnome_desktop(Gio::Settings::create("org.gnome.desktop.interface"))
    , synchronization(Gio::Settings::create("org.gnome.gnote.sync"))
  {
    init();
  }

  void Preferences::init()
  {
    m_schema_replace_title = Gio::Settings::create(SCHEMA_REPLACE_TITLE);
    m_schema_sync = synchronization.m_schema;
    m_schema_sync_wdfs = Gio::Settings::create(SCHEMA_SYNC_WDFS);

    SETUP_CACHED_KEY(m_schema_sync, sync_autosync_timeout, SYNC_AUTOSYNC_TIMEOUT, int);
  }
  
  DEFINE_GETTER_SETTER_INT(m_schema_replace_title, replace_title_clipboard, REPLACE_TITLE_CLIPBOARD)

  DEFINE_CACHING_SETTER_INT(m_schema_sync, sync_autosync_timeout, SYNC_AUTOSYNC_TIMEOUT)

  DEFINE_GETTER_SETTER_INT(m_schema_sync_wdfs, sync_fuse_mount_timeout, SYNC_FUSE_MOUNT_TIMEOUT)
  DEFINE_GETTER_SETTER_BOOL(m_schema_sync_wdfs, sync_fuse_wdfs_accept_sllcert, SYNC_FUSE_WDFS_ACCEPT_SSLCERT)
  DEFINE_GETTER_SETTER_STRING(m_schema_sync_wdfs, sync_fuse_wdfs_url, SYNC_FUSE_WDFS_URL)
  DEFINE_GETTER_SETTER_STRING(m_schema_sync_wdfs, sync_fuse_wdfs_username, SYNC_FUSE_WDFS_USERNAME)

}

