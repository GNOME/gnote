/*
 * gnote
 *
 * Copyright (C) 2010-2017 Aurimas Cernius
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

#include <glibmm/optioncontext.h>
#include <glibmm/ustring.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/statusicon.h>

#include "base/macros.hpp"
#include "actionmanager.hpp"
#include "ignote.hpp"
#include "remotecontrolproxy.hpp"
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
  bool shell_search()
    {
      return m_shell_search;
    }
  void parse(int &argc, gchar ** & argv);

  static gboolean parse_func(const gchar *option_name,
                             const gchar *value,
                             gpointer data,
                             GError **error);
private:
  void        print_version();
  template <typename T>
  bool        display_note(T & remote, Glib::ustring uri);
  template <typename T>
  void execute(T & remote);

  GOptionContext *m_context;

  bool        m_use_panel;
  bool        m_background;
  bool        m_shell_search;
  gchar *     m_note_path;
  bool        m_do_search;
  Glib::ustring m_search;
  bool        m_show_version;
  bool        m_do_new_note;
  Glib::ustring m_new_note_name;
  gchar*      m_open_note;
  bool        m_open_start_here;
  gchar*      m_highlight_search;


  // depend on m_open_note, set in on_post_parse
  Glib::ustring m_open_note_name;
  Glib::ustring m_open_note_uri;
  Glib::ustring m_open_external_note_path;
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

  void on_quit_gnote_action(const Glib::VariantBase&);
  void on_preferences_response(int res);
  void on_show_preferences_action(const Glib::VariantBase&);
  void on_show_help_action(const Glib::VariantBase&);
  void on_show_about_action(const Glib::VariantBase&);
  virtual MainWindow & new_main_window() override;
  virtual MainWindow & get_main_window() override;
  virtual MainWindow & get_window_for_note() override;
  virtual MainWindow & open_search_all() override;
  void open_note_sync_window(const Glib::VariantBase&);

  bool is_background() const
    {
      return m_is_background || m_is_shell_search;
    }
  bool windowed()
    {
      return !is_background();
    }
  static void register_remote_control(NoteManager & manager, RemoteControlProxy::slot_name_acquire_finish on_finish);
  virtual void open_note(const Note::Ptr & note) override;
protected:
  virtual int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine> & command_line) override;
  virtual void on_startup() override;
private:
  Gnote();
  Glib::ustring get_note_path(const Glib::ustring & override_path);
  void common_init();
  void end_main(bool bus_aquired, bool name_acquired);
  void on_sync_dialog_response(int response_id);
  void on_main_window_closed(Gtk::Window*);
  void make_app_actions();
  void add_app_actions(const std::vector<Glib::RefPtr<Gio::SimpleAction> > & actions);
  void make_app_menu();
  void on_new_window_action(const Glib::VariantBase&);
  void on_new_note_app_action(const Glib::VariantBase&);
  void on_show_help_shortcust_action(const Glib::VariantBase&);
  MainWindow *get_active_window();
  void register_object();

  NoteManager *m_manager;
  Glib::RefPtr<Gtk::IconTheme> m_icon_theme;
  bool m_is_background;
  bool m_is_shell_search;
  PreferencesDialog *m_prefsdlg;
  GnoteCommandLine cmd_line;
  sync::SyncDialog::Ptr m_sync_dlg;
};


}

#endif
