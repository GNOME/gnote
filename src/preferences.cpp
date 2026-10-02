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

  Preferences::ReplaceTitleSettings::ReplaceTitleSettings(const Glib::RefPtr<Gio::Settings> &schema)
    : clipboard(*schema, "clipboard")
    , m_schema(schema)
  {
  }

  Preferences::SyncSettings::SyncSettings(const Glib::RefPtr<Gio::Settings> &schema)
    : selected_service_addin(*schema, "sync-selected-service-addin")
    , autosync_timeout(*schema, "autosync-timeout")
    , client_id(*schema, "sync-guid")
    , configured_conflict_behavior(*schema, "sync-conflict-behavior")
    , local_path(*schema, "sync-local-path")
    , m_schema(schema)
  {
  }

  Preferences::SyncWebDavSettings::SyncWebDavSettings(const Glib::RefPtr<Gio::Settings> &schema)
    : url(*schema, "url")
    , username(*schema, "username")
    , mount_timeout(*schema, "sync-fuse-mount-timeout-ms")
    , m_schema(schema)
  {
  }


  Preferences::Preferences()
    : gnote(Gio::Settings::create("org.gnome.gnote"))
    , gnome_desktop(Gio::Settings::create("org.gnome.desktop.interface"))
    , replace_title(Gio::Settings::create("org.gnome.gnote.replace-title"))
    , synchronization(Gio::Settings::create("org.gnome.gnote.sync"))
    , web_dav(Gio::Settings::create("org.gnome.gnote.sync.wdfs"))
  {
  }
}

