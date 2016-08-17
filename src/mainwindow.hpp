/*
 * gnote
 *
 * Copyright (C) 2013,2015-2016 Aurimas Cernius
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

#include "mainwindowembeds.hpp"
#include "note.hpp"
#include "utils.hpp"


namespace gnote {

class MainWindow
  : public Gtk::ApplicationWindow
  , public EmbeddableWidgetHost
{
public:
  static MainWindow *get_owning(Gtk::Widget & widget);
  static void present_in(MainWindow & win, const Note::Ptr & note);
  static MainWindow *present_active(const Note::Ptr & note);
  static MainWindow *present_in_new_window(const Note::Ptr & note, bool close_on_esacpe);
  static MainWindow *present_default(const Note::Ptr & note);
  static bool use_client_side_decorations();

  explicit MainWindow(const std::string & title);

  virtual void set_search_text(const std::string & value) = 0;
  virtual void show_search_bar(bool grab_focus = true) = 0;
  virtual void present_search() = 0;
  virtual void new_note() = 0;
  virtual void close_window() = 0;
  virtual bool is_search() = 0;

  void close_on_escape(bool close_win)
    {
      m_close_on_esc = close_win;
    }
  bool close_on_escape() const
    {
      return m_close_on_esc;
    }
protected:
  virtual void present_note(const Note::Ptr & note) = 0;
private:
  static int s_use_client_side_decorations;

  bool m_close_on_esc;
};

}

#endif

