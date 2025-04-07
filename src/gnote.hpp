/*
 * gnote
 *
 * Copyright (C) 2010-2019,2021-2025 Aurimas Cernius
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

#include "actionmanager.hpp"
#include "iconmanager.hpp"
#include "ignote.hpp"
#include "preferences.hpp"
#include "remotecontrolproxy.hpp"
#include "synchronization/syncdialog.hpp"
#include "synchronization/syncmanager.hpp"

namespace gnote {

class PreferencesDialog;
class NoteManager;

class GnoteCommandLine
{
public:
  GnoteCommandLine(IGnote & ignote);
  ~GnoteCommandLine();
  void set_note_manager(NoteManagerBase & manager)
    {
      m_manager = &manager;
    }
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
  bool do_new_window()
    {
      return m_do_new_window;
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

  IGnote    & m_gnote;
  NoteManagerBase *m_manager;
  bool        m_use_panel;
  bool        m_background;
  bool        m_shell_search;
  gchar *     m_note_path;
  bool        m_do_search;
  Glib::ustring m_search;
  bool        m_show_version;
  bool        m_do_new_note;
  Glib::ustring m_new_note_name;
  bool        m_do_new_window;
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
  virtual IActionManager & action_manager() override
    {
      return m_action_manager;
    }
  virtual notebooks::NotebookManager & notebook_manager() override;
  virtual sync::SyncManager & sync_manager() override
    {
      return *m_sync_manager;
    }
  virtual Preferences & preferences()
    {
      return m_preferences;
    }
  RemoteControlProxy & remote_control()
    {
      return m_remote_control;
    }

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
  void open_note(const NoteBase & note) override;
protected:
  virtual int on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine> & command_line) override;
  virtual void on_startup() override;
  virtual void on_activate() override;
private:
  Gnote();
  Glib::ustring get_note_path(const Glib::ustring & override_path);
  void common_init();
  void end_main();
  void on_sync_dialog_response(int response_id);
  void on_main_window_closed(Gtk::Window*);
  void make_app_actions();
  void add_app_actions(const std::vector<Glib::RefPtr<Gio::SimpleAction> > & actions);
  void on_new_window_action(const Glib::VariantBase&);
  void on_new_note_app_action(const Glib::VariantBase&);
  void on_show_help_shortcuts_action(const Glib::VariantBase&);
  void on_color_scheme_pref_changed();
  MainWindow *get_active_window();
  void register_object();

  NoteManager *m_manager;
  Preferences m_preferences;
  ActionManager m_action_manager;
  sync::SyncManager *m_sync_manager;
  Glib::RefPtr<Gtk::IconTheme> m_icon_theme;
  bool m_is_background;
  bool m_is_shell_search;
  RemoteControlProxy m_remote_control;
  std::unique_ptr<PreferencesDialog> m_prefsdlg;
  GnoteCommandLine m_cmd_line;
  sync::SyncDialog::Ptr m_sync_dlg;
};


}

#endif
