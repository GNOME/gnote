/*
 * gnote
 *
 * Copyright (C) 2011-2015,2017,2019-2022,2024 Aurimas Cernius
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


#define GNOTE_PREFERENCES_SETTING(key, rettype, paramtype) \
  rettype key() const; \
  void key(paramtype);

#define GNOTE_PREFERENCES_SETTING_BOOL(key) GNOTE_PREFERENCES_SETTING(key, bool, bool)
#define GNOTE_PREFERENCES_SETTING_INT(key) GNOTE_PREFERENCES_SETTING(key, int, int)
#define GNOTE_PREFERENCES_SETTING_STRING(key) GNOTE_PREFERENCES_SETTING(key, Glib::ustring, const Glib::ustring &)


#define GNOTE_PREFERENCES_CACHING_SETTING_RO(key, type) \
  type key() const \
    { \
      return m_##key; \
    } \
  sigc::signal<void()> signal_##key##_changed;


#define GNOTE_PREFERENCES_CACHING_SETTING(key, type) \
  GNOTE_PREFERENCES_CACHING_SETTING_RO(key, type) \
  void key(type);


namespace gnote {

  class Preferences 
  {
  public:
    static const char *COLOR_SCHEME_DARK_VAL;
    static const char *COLOR_SCHEME_LIGHT_VAL;

    Preferences() {}
    void init();

    GNOTE_PREFERENCES_CACHING_SETTING(enable_spellchecking, bool)
    GNOTE_PREFERENCES_CACHING_SETTING(enable_auto_links, bool)
    GNOTE_PREFERENCES_CACHING_SETTING(enable_url_links, bool)
    GNOTE_PREFERENCES_CACHING_SETTING(enable_wikiwords, bool)
    GNOTE_PREFERENCES_CACHING_SETTING(enable_custom_font, bool)
    GNOTE_PREFERENCES_CACHING_SETTING(highlight_background_color, const Glib::ustring &)
    GNOTE_PREFERENCES_CACHING_SETTING(highlight_foreground_color, const Glib::ustring &)
    GNOTE_PREFERENCES_SETTING_BOOL(enable_auto_bulleted_lists)
    GNOTE_PREFERENCES_CACHING_SETTING(note_rename_behavior, int)
    GNOTE_PREFERENCES_SETTING_STRING(start_note_uri)
    GNOTE_PREFERENCES_CACHING_SETTING(custom_font_face, const Glib::ustring &)
    GNOTE_PREFERENCES_SETTING_STRING(menu_pinned_notes)
    GNOTE_PREFERENCES_SETTING_BOOL(main_window_maximized)
    GNOTE_PREFERENCES_SETTING_INT(search_window_width)
    GNOTE_PREFERENCES_SETTING_INT(search_window_height)
    GNOTE_PREFERENCES_SETTING_INT(search_window_splitter_pos)
    GNOTE_PREFERENCES_SETTING_STRING(search_sorting)
    GNOTE_PREFERENCES_SETTING_STRING(use_client_side_decorations)
    GNOTE_PREFERENCES_CACHING_SETTING(color_scheme, const Glib::ustring&)

    GNOTE_PREFERENCES_CACHING_SETTING_RO(desktop_gnome_clock_format, const Glib::ustring &)

    Glib::ustring sync_client_id() const;
    Glib::ustring sync_local_path() const;
    void sync_local_path(const Glib::ustring &);
    GNOTE_PREFERENCES_CACHING_SETTING(sync_selected_service_addin, const Glib::ustring &)
    GNOTE_PREFERENCES_SETTING_INT(sync_configured_conflict_behavior)
    GNOTE_PREFERENCES_CACHING_SETTING(sync_autosync_timeout, int)

    GNOTE_PREFERENCES_SETTING_INT(sync_fuse_mount_timeout)
    GNOTE_PREFERENCES_SETTING_BOOL(sync_fuse_wdfs_accept_sllcert)
    GNOTE_PREFERENCES_SETTING_STRING(sync_fuse_wdfs_url)
    GNOTE_PREFERENCES_SETTING_STRING(sync_fuse_wdfs_username)
  private:
    Preferences(const Preferences &) = delete;

    Glib::RefPtr<Gio::Settings> m_schema_gnote;
    Glib::RefPtr<Gio::Settings> m_schema_gnome_interface;
    Glib::RefPtr<Gio::Settings> m_schema_sync;
    Glib::RefPtr<Gio::Settings> m_schema_sync_wdfs;

    Glib::ustring m_custom_font_face;
    Glib::ustring m_highlight_background_color;
    Glib::ustring m_highlight_foreground_color;
    Glib::ustring m_color_scheme;

    Glib::ustring m_desktop_gnome_clock_format;
    Glib::ustring m_desktop_gnome_font;

    Glib::ustring m_sync_selected_service_addin;

    int m_note_rename_behavior;
    int m_sync_autosync_timeout;

    bool m_enable_spellchecking;
    bool m_enable_auto_links;
    bool m_enable_url_links;
    bool m_enable_wikiwords;
    bool m_enable_custom_font;
    bool m_open_notes_in_new_window;
  };


}


#endif
