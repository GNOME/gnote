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

    m_schema_gnote->signal_changed(ENABLE_SPELLCHECKING).connect([this](const Glib::ustring &) {
      m_enable_spellchecking = m_schema_gnote->get_boolean(ENABLE_SPELLCHECKING);
      signal_enable_spellchecking_changed();
    });

    m_enable_spellchecking = m_schema_gnote->get_boolean(ENABLE_SPELLCHECKING);

    m_schema_gnome_interface->signal_changed(DESKTOP_GNOME_CLOCK_FORMAT).connect([this](const Glib::ustring &) {
      m_desktop_gnome_clock_format = m_schema_gnome_interface->get_string(DESKTOP_GNOME_CLOCK_FORMAT);
      signal_desktop_gnome_clock_format_changed();
    });
    m_schema_gnome_interface->signal_changed(DESKTOP_GNOME_FONT).connect([this](const Glib::ustring &) {
      m_desktop_gnome_font = m_schema_gnome_interface->get_string(DESKTOP_GNOME_FONT);
      signal_desktop_gnome_font_changed();
    });

    m_desktop_gnome_clock_format = m_schema_gnome_interface->get_string(DESKTOP_GNOME_CLOCK_FORMAT);
    m_desktop_gnome_font = m_schema_gnome_interface->get_string(DESKTOP_GNOME_FONT);


    m_schema_sync->signal_changed(SYNC_SELECTED_SERVICE_ADDIN).connect([this](const Glib::ustring &) {
      m_sync_selected_service_addin = m_schema_sync->get_string(SYNC_SELECTED_SERVICE_ADDIN);
      signal_sync_selected_service_addin_changed();
    });
    m_schema_sync->signal_changed(SYNC_AUTOSYNC_TIMEOUT).connect([this](const Glib::ustring &) {
      m_sync_autosync_timeout = m_schema_sync->get_int(SYNC_AUTOSYNC_TIMEOUT);
      signal_sync_autosync_timeout_changed();
    });

    m_sync_selected_service_addin = m_schema_sync->get_string(SYNC_SELECTED_SERVICE_ADDIN);
    m_sync_autosync_timeout = m_schema_sync->get_int(SYNC_AUTOSYNC_TIMEOUT);
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

  void Preferences::enable_spellchecking(bool value)
  {
    m_enable_spellchecking = value;
    m_schema_gnote->set_boolean(ENABLE_SPELLCHECKING, value);
  }

  bool Preferences::enable_auto_links() const
  {
    return m_schema_gnote->get_boolean(ENABLE_AUTO_LINKS);
  }

  void Preferences::enable_auto_links(bool value)
  {
    m_schema_gnote->set_boolean(ENABLE_AUTO_LINKS, value);
  }

  bool Preferences::enable_url_links() const
  {
    return m_schema_gnote->get_boolean(ENABLE_URL_LINKS);
  }

  void Preferences::enable_url_links(bool value)
  {
    m_schema_gnote->set_boolean(ENABLE_URL_LINKS, value);
  }

  bool Preferences::enable_wikiwords() const
  {
    return m_schema_gnote->get_boolean(ENABLE_WIKIWORDS);
  }

  void Preferences::enable_wikiwords(bool value)
  {
    m_schema_gnote->set_boolean(ENABLE_WIKIWORDS, value);
  }

  bool Preferences::open_notes_in_new_window() const
  {
    return m_schema_gnote->get_boolean(OPEN_NOTES_IN_NEW_WINDOW);
  }

  bool Preferences::enable_custom_font() const
  {
    return m_schema_gnote->get_boolean(ENABLE_CUSTOM_FONT);
  }

  void Preferences::enable_custom_font(bool value)
  {
    m_schema_gnote->set_boolean(ENABLE_CUSTOM_FONT, value);
  }

  bool Preferences::enable_auto_bulleted_lists() const
  {
    return m_schema_gnote->get_boolean(ENABLE_AUTO_BULLETED_LISTS);
  }

  void Preferences::enable_auto_bulleted_lists(bool value)
  {
    m_schema_gnote->set_boolean(ENABLE_AUTO_BULLETED_LISTS, value);
  }

  void Preferences::open_notes_in_new_window(bool value)
  {
    m_schema_gnote->set_boolean(OPEN_NOTES_IN_NEW_WINDOW, value);
  }

  Glib::ustring Preferences::sync_client_id() const
  {
    return m_schema_sync->get_string(SYNC_CLIENT_ID);
  }

  Glib::ustring Preferences::sync_local_path() const
  {
    return m_schema_sync->get_string(SYNC_LOCAL_PATH);
  }

  void Preferences::sync_local_path(const Glib::ustring & value)
  {
    m_schema_sync->set_string(SYNC_LOCAL_PATH, value);
  }

  void Preferences::sync_selected_service_addin(const Glib::ustring & value)
  {
    m_sync_selected_service_addin = value;
    m_schema_sync->set_string(SYNC_SELECTED_SERVICE_ADDIN, value);
  }

  int Preferences::sync_configured_conflict_behavior() const
  {
    return m_schema_sync->get_int(SYNC_CONFIGURED_CONFLICT_BEHAVIOR);
  }

  void Preferences::sync_configured_conflict_behavior(int value)
  {
    m_schema_sync->set_int(SYNC_CONFIGURED_CONFLICT_BEHAVIOR, value);
  }

  void Preferences::sync_autosync_timeout(int value)
  {
    m_sync_autosync_timeout = value;
    m_schema_sync->set_int(SYNC_AUTOSYNC_TIMEOUT, value);
  }

  int Preferences::sync_fuse_mount_timeout() const
  {
    return m_schema_sync_wdfs->get_int(SYNC_FUSE_MOUNT_TIMEOUT);
  }

  void Preferences::sync_fuse_mount_timeout(int value)
  {
    m_schema_sync_wdfs->set_int(SYNC_FUSE_MOUNT_TIMEOUT, value);
  }

  bool Preferences::sync_fuse_wdfs_accept_sllcert() const
  {
    return m_schema_sync_wdfs->get_boolean(SYNC_FUSE_WDFS_ACCEPT_SSLCERT);
  }

  void Preferences::sync_fuse_wdfs_accept_sllcert(bool value)
  {
    m_schema_sync_wdfs->set_boolean(SYNC_FUSE_WDFS_ACCEPT_SSLCERT, value);
  }

  Glib::ustring Preferences::sync_fuse_wdfs_url() const
  {
    return m_schema_sync_wdfs->get_string(SYNC_FUSE_WDFS_URL);
  }

  void Preferences::sync_fuse_wdfs_url(const Glib::ustring & value)
  {
    m_schema_sync_wdfs->set_string(SYNC_FUSE_WDFS_URL, value);
  }

  Glib::ustring Preferences::sync_fuse_wdfs_username() const
  {
    return m_schema_sync_wdfs->get_string(SYNC_FUSE_WDFS_USERNAME);
  }

  void Preferences::sync_fuse_wdfs_username(const Glib::ustring & value) const
  {
    m_schema_sync_wdfs->set_string(SYNC_FUSE_WDFS_USERNAME, value);
  }

}

