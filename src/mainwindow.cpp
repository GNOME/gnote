/*
 * gnote
 *
 * Copyright (C) 2013,2015-2017,2019-2023 Aurimas Cernius
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


#include "ignote.hpp"
#include "mainwindow.hpp"
#include "notewindow.hpp"
#include "sharp/string.hpp"

namespace gnote {

int MainWindow::s_use_client_side_decorations = -1;


void MainWindow::present_in(MainWindow & win, Note & note)
{
  win.present_note(note);
  win.present();
}

MainWindow & MainWindow::present_in_new_window(IGnote & g, Note & note)
{
  MainWindow & window = g.new_main_window();
  window.present_note(note);
  window.present();
  return window;
}

MainWindow & MainWindow::present_default(IGnote & g, Note & note)
{
  if(note.has_window()) {
    auto note_window = note.get_window();
    if(note_window->host()) {
      MainWindow *win = dynamic_cast<MainWindow*>(note_window->host());
      if(win) {
        win->present_note(note);
        return *win;
      }
    }
  }

  MainWindow & win = g.get_window_for_note();
  win.present_note(note);
  win.present();
  return win;
}

bool MainWindow::use_client_side_decorations(Preferences & prefs)
{
  if(s_use_client_side_decorations < 0) {
    auto setting = prefs.use_client_side_decorations();
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
      const char *current_desktop = std::getenv("XDG_CURRENT_DESKTOP");
      if (current_desktop) {
        std::vector<Glib::ustring> current_desktops;
        sharp::string_split(current_desktops, current_desktop, ":");
        for(const Glib::ustring & cd : current_desktops) {
          Glib::ustring current_de = cd.lowercase();
          for(const Glib::ustring & de : desktops) {
            if (current_de == de) {
              s_use_client_side_decorations = 1;
              return s_use_client_side_decorations;
            }
          }
        }
      }
    }
  }

  return s_use_client_side_decorations;
}


MainWindow::MainWindow(Glib::ustring && title)
{
  set_title(std::move(title));
}

}

