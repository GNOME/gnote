/*
 * gnote
 *
 * Copyright (C) 2013,2015-2017 Aurimas Cernius
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


#include "base/macros.hpp"
#include "ignote.hpp"
#include "mainwindow.hpp"
#include "notewindow.hpp"
#include "sharp/string.hpp"

namespace gnote {

int MainWindow::s_use_client_side_decorations = -1;


MainWindow *MainWindow::get_owning(Gtk::Widget & widget)
{
  Gtk::Container *container = widget.get_parent();
  if(!container) {
    return dynamic_cast<MainWindow*>(&widget);
  }

  Gtk::Container *cntr = container->get_parent();
  while(cntr) {
    container = cntr;
    cntr = container->get_parent();
  }

  return dynamic_cast<MainWindow*>(container);
}

void MainWindow::present_in(MainWindow & win, const Note::Ptr & note)
{
  win.present_note(note);
  win.present();
}

MainWindow *MainWindow::present_active(const Note::Ptr & note)
{
  if(note && note->has_window() && note->get_window()->host()
     && note->get_window()->host()->is_foreground(*note->get_window())) {
    MainWindow *win = dynamic_cast<MainWindow*>(note->get_window()->host());
    win->present();
    return win;
  }

  return NULL;
}

MainWindow *MainWindow::present_in_new_window(const Note::Ptr & note, bool close_on_esc)
{
  if(!note) {
    return NULL;
  }
  if(!MainWindow::present_active(note)) {
    MainWindow & window = IGnote::obj().new_main_window();
    window.present_note(note);
    window.present();
    window.close_on_escape(close_on_esc);
    return &window;
  }

  return NULL;
}

MainWindow *MainWindow::present_default(const Note::Ptr & note)
{
  if(!note) {
    return NULL;
  }
  MainWindow *win = MainWindow::present_active(note);
  if(win) {
    return win;
  }
  Glib::RefPtr<Gio::Settings> settings = Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE);
  if(false == settings->get_boolean(Preferences::OPEN_NOTES_IN_NEW_WINDOW)) {
    if (note->has_window()) {
      win = dynamic_cast<MainWindow*>(note->get_window()->host());
    }
    else {
      win = &IGnote::obj().get_window_for_note();
    }
  }
  if(!win) {
    win = &IGnote::obj().new_main_window();
    win->close_on_escape(settings->get_boolean(Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE));
  }
  win->present_note(note);
  win->present();
  return win;
}

bool MainWindow::use_client_side_decorations()
{
  if(s_use_client_side_decorations < 0) {
    Glib::ustring setting = Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE)->get_string(
      Preferences::USE_CLIENT_SIDE_DECORATIONS);
    if(setting == "enabled") {
      s_use_client_side_decorations = 1;
    }
    else if(setting == "disabled") {
      s_use_client_side_decorations = 0;
    }
    else {
      s_use_client_side_decorations = 0;
      std::vector<Glib::ustring> desktops;
      sharp::string_split(desktops, setting, ",");
      const char *current_desktop = std::getenv("DESKTOP_SESSION");
      if (current_desktop) {
        Glib::ustring current_de = Glib::ustring(current_desktop).lowercase();
        FOREACH(Glib::ustring de, desktops) {
          Glib::ustring denv = Glib::ustring(de).lowercase();
          if(current_de.find(denv) != Glib::ustring::npos) {
            s_use_client_side_decorations = 1;
          }
        }
      }
    }
  }

  return s_use_client_side_decorations;
}


MainWindow::MainWindow(const Glib::ustring & title)
  : m_close_on_esc(false)
{
  set_title(title);
}

}

