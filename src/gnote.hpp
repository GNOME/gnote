/*
 * gnote
 *
 * Copyright (C) 2010-2013 Aurimas Cernius
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




#ifndef _GNOTE_HPP_
#define _GNOTE_HPP_

#include <string>

#include <glibmm/optioncontext.h>
#include <glibmm/ustring.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/statusicon.h>

#include "actionmanager.hpp"
#include "ignote.hpp"
#include "keybinder.hpp"
#include "remotecontrolproxy.hpp"
#include "tray.hpp"
#include "synchronization/syncdialog.hpp"

namespace gnote {

class PreferencesDialog;
class NoteManager;
class RemoteControlClient;

class GnoteCommandLine
{
public:
  GnoteCommandLine();
  ~GnoteCommandLine();
  int execute();
  int immediate_execute();

  const gchar * note_path() const
    {
      return m_note_path ? m_note_path : "";
    }
  bool        needs_execute() const;
  bool        needs_immediate_execute() const;
  bool        background()
    {
      return m_background;
    }
  void parse(int &argc, gchar ** & argv);

  static gboolean parse_func(const gchar *option_name,
                             const gchar *value,
                             gpointer data,
                             GError **error);
private:
  void        print_version();
  template <typename T>
  bool        display_note(T & remote, std::string uri);
  template <typename T>
  void execute(T & remote);

  GOptionContext *m_context;

  bool        m_use_panel;
  bool        m_background;
  gchar *     m_note_path;
  bool        m_do_search;
  std::string m_search;
  bool        m_show_version;
  bool        m_do_new_note;
  std::string m_new_note_name;
  gchar*      m_open_note;
  bool        m_open_start_here;
  gchar*      m_highlight_search;


  // depend on m_open_note, set in on_post_parse
  std::string m_open_note_name;
  std::string m_open_note_uri;
  std::string m_open_external_note_path;
};


class Gnote
  : public Gtk::Application
  , public IGnote
{
public:
  static Glib::RefPtr<Gnote> create()
    {
      return Glib::RefPtr<Gnote>(new Gnote);
    }

  ~Gnote();
  int main(int argc, char **argv);
  NoteManager & default_note_manager()
    {
      return *m_manager;
    }
  IKeybinder & keybinder()
    {
      return *m_keybinder;
    }

  void setup_global_actions();
  void start_tray_icon();

  void on_quit_gnote_action(const Glib::VariantBase&);
  void on_preferences_response(int res);
  void on_show_preferences_action(const Glib::VariantBase&);
  void on_show_help_action(const Glib::VariantBase&);
  void on_show_about_action(const Glib::VariantBase&);
  virtual MainWindow & new_main_window();
  virtual MainWindow & get_main_window();
  virtual MainWindow & get_window_for_note();
  virtual void open_search_all();
  void open_note_sync_window(const Glib::VariantBase&);

  bool tray_icon_showing()
    {
      return m_tray_icon && m_tray_icon->is_embedded() && m_tray_icon->get_visible();
    }
  bool is_background() const
    {
      return m_is_background;
    }
  bool windowed()
    {
      return !tray_icon_showing() && !is_background();
    }
  void set_tray(const Tray::Ptr & tray)
    {
      m_tray = tray;
    }
  static void register_remote_control(NoteManager & manager, RemoteControlProxy::slot_name_acquire_finish on_finish);
  virtual void open_note(const Note::Ptr & note);
protected:
  virtual int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine> & command_line);
  virtual void on_startup();
  virtual void on_window_removed(Gtk::Window *window);
private:
  Gnote();
  std::string get_note_path(const std::string & override_path);
  void on_setting_changed(const Glib::ustring & key);
  void common_init();
  void end_main(bool bus_aquired, bool name_acquired);
  void on_sync_dialog_response(int response_id);
  void on_main_window_closed();
  void make_app_actions();
  void add_app_actions(const std::vector<Glib::RefPtr<Gio::SimpleAction> > & actions);
  void make_app_menu();
  void on_new_window_action(const Glib::VariantBase&);
  void on_new_note_app_action(const Glib::VariantBase&);
  MainWindow *get_active_window();
  bool show_tray_icon_timeout();
  void register_object();

  NoteManager *m_manager;
  IKeybinder  *m_keybinder;
  Glib::RefPtr<Gtk::IconTheme> m_icon_theme;
  Glib::RefPtr<TrayIcon> m_tray_icon;
  Tray::Ptr m_tray;
  bool m_is_background;
  PreferencesDialog *m_prefsdlg;
  GnoteCommandLine cmd_line;
  sync::SyncDialog::Ptr m_sync_dlg;
};


}

#endif
