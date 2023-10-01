/*
 * gnote
 *
 * Copyright (C) 2013,2015-2017,2019,2021-2023 Aurimas Cernius
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


#ifndef _MAINWINDOW_HPP_
#define _MAINWINDOW_HPP_

#include <gtkmm/applicationwindow.h>

#include "mainwindowembeds.hpp"
#include "note.hpp"


namespace gnote {

class IGnote;
class Preferences;

class MainWindow
  : public Gtk::ApplicationWindow
  , public EmbeddableWidgetHost
{
public:
  static void present_in(MainWindow & win, Note & note);
  static MainWindow & present_in_new_window(IGnote & g, Note & note);
  static MainWindow & present_default(IGnote & g, Note & note);
  static bool use_client_side_decorations(Preferences & prefs);

  explicit MainWindow(Glib::ustring && title);

  virtual void set_search_text(Glib::ustring && value) = 0;
  virtual void show_search_bar(bool grab_focus = true) = 0;
  virtual void present_search() = 0;
  virtual void new_note() = 0;
  virtual void close_window() = 0;
  virtual bool is_search() = 0;
protected:
  virtual void present_note(Note & note) = 0;
private:
  static int s_use_client_side_decorations;
};

}

#endif

