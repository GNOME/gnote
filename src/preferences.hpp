/*
 * gnote
 *
 * Copyright (C) 2011-2015,2017,2019-2022,2024-2026 Aurimas Cernius
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

  template<typename T>
  class ReadableSetting
  {
  protected:
    static T get_value(Gio::Settings &schema, const Glib::ustring &key);
  };

  template<>
  class ReadableSetting<bool>
  {
  protected:
    static bool get_value(Gio::Settings &schema, const Glib::ustring &key)
      {
        return schema.get_boolean(key);
      }
  };

  template<>
  class ReadableSetting<int>
  {
  protected:
    static int get_value(Gio::Settings &schema, const Glib::ustring &key)
      {
        return schema.get_int(key);
      }
  };

  template<>
  class ReadableSetting<unsigned>
  {
  protected:
    static unsigned get_value(Gio::Settings &schema, const Glib::ustring &key)
      {
        return schema.get_uint(key);
      }
  };

  template<>
  class ReadableSetting<Glib::ustring>
  {
  protected:
    static Glib::ustring get_value(Gio::Settings &schema, const Glib::ustring &key)
      {
        return schema.get_string(key);
      }
  };

  template<typename T>
  class WritableSetting
  {
  protected:
    static void set_value(Gio::Settings &schema, const Glib::ustring &key, const T &value);
  };

  template<>
  class WritableSetting<bool>
  {
  protected:
    static void set_value(Gio::Settings &schema, const Glib::ustring &key, const bool &value)
      {
        schema.set_boolean(key, value);
      }
  };

  template<>
  class WritableSetting<int>
  {
  protected:
    static void set_value(Gio::Settings &schema, const Glib::ustring &key, const int &value)
      {
        schema.set_int(key, value);
      }
  };

  template<>
  class WritableSetting<unsigned>
  {
  protected:
    static void set_value(Gio::Settings &schema, const Glib::ustring &key, const unsigned &value)
      {
        schema.set_uint(key, value);
      }
  };

  template<>
  class WritableSetting<Glib::ustring>
  {
  protected:
    static void set_value(Gio::Settings &schema, const Glib::ustring &key, const Glib::ustring &value)
      {
        schema.set_string(key, value);
      }
  };

  class MonitoredSettingBase
  {
  public:
    sigc::signal<void()> signal_changed;
  protected:
    MonitoredSettingBase(Gio::Settings &schema, const Glib::ustring &key)
      {
        schema.signal_changed(key).connect(sigc::mem_fun(*this, &MonitoredSettingBase::on_changed));
      }

    void on_changed(const Glib::ustring&)
      {
        signal_changed();
      }
  };


  class Preferences 
  {
  public:
    static const char *COLOR_SCHEME_DARK_VAL;
    static const char *COLOR_SCHEME_LIGHT_VAL;

    template<typename T>
    class ReadOnlySetting
      : protected ReadableSetting<T>
    {
    public:
      ReadOnlySetting(Gio::Settings &schema, Glib::ustring &&key)
        : m_schema(schema)
        , m_key(std::move(key))
        {}

      operator T() const
        {
          return ReadableSetting<T>::get_value(m_schema, m_key);
        }
    protected:
      ReadOnlySetting(const ReadOnlySetting&) = delete;
      ReadOnlySetting &operator=(const ReadOnlySetting&) = delete;

      Gio::Settings &m_schema;
      Glib::ustring m_key;
    };

    template<typename T>
    class ReadOnlyMonitoredSetting
      : public ReadOnlySetting<T>
      , public MonitoredSettingBase
    {
    public:
      ReadOnlyMonitoredSetting(Gio::Settings &schema, Glib::ustring &&key)
        : ReadOnlySetting<T>(schema, std::move(key))
        , MonitoredSettingBase(schema, this->m_key)
        {}
    };

    template<typename T>
    class Setting
      : public ReadOnlySetting<T>
      , protected WritableSetting<T>
    {
    public:
      Setting(Gio::Settings &schema, Glib::ustring &&key)
        : ReadOnlySetting<T>(schema, std::move(key))
        {}

      Setting &operator=(const T &value)
        {
          WritableSetting<T>::set_value(this->m_schema, this->m_key, value);
          return *this;
        }
    };

    template<typename T>
    class MonitoredSetting
      : public Setting<T>
      , public MonitoredSettingBase
    {
    public:
      MonitoredSetting(Gio::Settings &schema, Glib::ustring &&key)
        : Setting<T>(schema, std::move(key))
        , MonitoredSettingBase(schema, this->m_key)
        {}

       using Setting<T>::operator=;
    };

    class GnoteSettings
    {
    public:
      friend Preferences;

      MonitoredSetting<bool> enable_spellchecking;
      MonitoredSetting<bool> enable_auto_links;
      MonitoredSetting<bool> enable_url_links;
      MonitoredSetting<bool> enable_wikiwords;
      MonitoredSetting<bool> enable_custom_font;
      MonitoredSetting<bool> highlight_accent_color_based;
      MonitoredSetting<int> note_rename_behavior;
      MonitoredSetting<unsigned> editor_tab_width;
      MonitoredSetting<Glib::ustring> highlight_background_color;
      MonitoredSetting<Glib::ustring> highlight_foreground_color;
      MonitoredSetting<Glib::ustring> custom_font_face;
      MonitoredSetting<Glib::ustring> color_scheme;
      Setting<bool> enable_auto_bulleted_lists;
      Setting<bool> main_window_maximized;
      Setting<int> search_window_width;
      Setting<int> search_window_height;
      Setting<int> search_window_splitter_pos;
      Setting<Glib::ustring> start_note_uri;
      Setting<Glib::ustring> menu_pinned_notes;
      Setting<Glib::ustring> search_sorting;
      Setting<Glib::ustring> use_client_side_decorations;
    private:
      explicit GnoteSettings(const Glib::RefPtr<Gio::Settings> &schema);

      Glib::RefPtr<Gio::Settings> m_schema;
    };

    class GnomeDesktopSettings
    {
    public:
      friend Preferences;

      ReadOnlyMonitoredSetting<Glib::ustring> clock_format;
    private:
      explicit GnomeDesktopSettings(const Glib::RefPtr<Gio::Settings> &schema);

      Glib::RefPtr<Gio::Settings> m_schema;
    };

    class SyncSettings
    {
    public:
      friend Preferences;

      MonitoredSetting<Glib::ustring> selected_service_addin;
      MonitoredSetting<int> autosync_timeout;
      ReadOnlySetting<Glib::ustring> client_id;
      Setting<int> configured_conflict_behavior;
      Setting<Glib::ustring> local_path;
    private:
      explicit SyncSettings(const Glib::RefPtr<Gio::Settings> &schema);

      Glib::RefPtr<Gio::Settings> m_schema;
    };

    class SyncWebDavSettings
    {
    public:
      friend Preferences;

      Setting<int> mount_timeout;
    private:
      explicit SyncWebDavSettings(const Glib::RefPtr<Gio::Settings> &schema);

      Glib::RefPtr<Gio::Settings> m_schema;
    };

    Preferences();
    void init();

    GnoteSettings gnote;
    GnomeDesktopSettings gnome_desktop;
    SyncSettings synchronization;
    SyncWebDavSettings web_dav;

    GNOTE_PREFERENCES_SETTING_BOOL(sync_fuse_wdfs_accept_sllcert)
    GNOTE_PREFERENCES_SETTING_STRING(sync_fuse_wdfs_url)
    GNOTE_PREFERENCES_SETTING_STRING(sync_fuse_wdfs_username)
    GNOTE_PREFERENCES_SETTING_INT(replace_title_clipboard)
  private:
    Preferences(const Preferences &) = delete;

    Glib::RefPtr<Gio::Settings> m_schema_replace_title;
    Glib::RefPtr<Gio::Settings> m_schema_sync_wdfs;

    Glib::ustring m_custom_font_face;
    bool m_highlight_accent_color_based;
    Glib::ustring m_highlight_background_color;
    Glib::ustring m_highlight_foreground_color;
    Glib::ustring m_color_scheme;
    unsigned m_editor_tab_width;

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
