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




#ifndef __PREFERENCES_HPP_
#define __PREFERENCES_HPP_

#include <map>
#include <giomm/settings.h>

namespace gnote {

  class Preferences 
  {
  public:
    static const char *SCHEMA_GNOTE;
    static const char *SCHEMA_SYNC_GVFS;
    static const char *SCHEMA_SYNC_WDFS;

    static const char *ENABLE_SPELLCHECKING;
    static const char *ENABLE_AUTO_LINKS;
    static const char *ENABLE_URL_LINKS;
    static const char *ENABLE_WIKIWORDS;
    static const char *ENABLE_CUSTOM_FONT;
    static const char *ENABLE_AUTO_BULLETED_LISTS;
    static const char *ENABLE_ICON_PASTE;
    static const char *ENABLE_CLOSE_NOTE_ON_ESCAPE;

    static const char *START_NOTE_URI;
    static const char *CUSTOM_FONT_FACE;
    static const char *MENU_NOTE_COUNT;
    static const char *MENU_PINNED_NOTES;

    static const char *NOTE_RENAME_BEHAVIOR;
    static const char *USE_STATUS_ICON;
    static const char *OPEN_NOTES_IN_NEW_WINDOW;
    static const char *AUTOSIZE_NOTE_WINDOW;

    static const char *MAIN_WINDOW_MAXIMIZED;
    static const char *SEARCH_WINDOW_WIDTH;
    static const char *SEARCH_WINDOW_HEIGHT;
    static const char *SEARCH_WINDOW_SPLITTER_POS;
    static const char *SEARCH_SORTING;
    static const char *USE_CLIENT_SIDE_DECORATIONS;

    static const char *SYNC_GVFS_URI;

    static const char *SYNC_FUSE_MOUNT_TIMEOUT;
    static const char *SYNC_FUSE_WDFS_ACCEPT_SSLCERT;
    static const char *SYNC_FUSE_WDFS_URL;
    static const char *SYNC_FUSE_WDFS_USERNAME;

    Preferences() {}
    void init();

    Glib::RefPtr<Gio::Settings> get_schema_settings(const Glib::ustring & schema);

    const Glib::ustring & desktop_gnome_clock_format() const
      {
        return m_desktop_gnome_clock_format;
      }
    sigc::signal<void> signal_desktop_gnome_clock_format_changed;
    const Glib::ustring & desktop_gnome_font() const
      {
        return m_desktop_gnome_font;
      }
    sigc::signal<void> signal_desktop_gnome_font_changed;

    Glib::ustring sync_client_id() const;
    Glib::ustring sync_local_path() const;
    void sync_local_path(const Glib::ustring &);
    Glib::ustring sync_selected_service_addin() const
      {
        return m_sync_selected_service_addin;
      }
    void sync_selected_service_addin(const Glib::ustring & value);
    sigc::signal<void> signal_sync_selected_service_addin_changed;
    int sync_configured_conflict_behavior() const;
    void sync_configured_conflict_behavior(int);
    int sync_autosync_timeout() const
      {
        return m_sync_autosync_timeout;
      }
    void sync_autosync_timeout(int value);
    sigc::signal<void> signal_sync_autosync_timeout_changed;
  private:
    Preferences(const Preferences &) = delete;
    std::map<Glib::ustring, Glib::RefPtr<Gio::Settings> > m_schemas;
    Glib::RefPtr<Gio::Settings> m_schema_gnome_interface;
    Glib::RefPtr<Gio::Settings> m_schema_sync;

    Glib::ustring m_desktop_gnome_clock_format;
    Glib::ustring m_desktop_gnome_font;

    Glib::ustring m_sync_selected_service_addin;

    int m_sync_autosync_timeout;
  };


}


#endif
