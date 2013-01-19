/*
 * gnote
 *
 * Copyright (C) 2013 Aurimas Cernius
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

#include "note.hpp"
#include "utils.hpp"


namespace gnote {

class MainWindow
  : public utils::ForcedPresentWindow
  , public utils::EmbeddableWidgetHost
{
public:
  static MainWindow *get_owning(Gtk::Widget & widget);

  explicit MainWindow(const std::string & title);

  virtual void set_search_text(const std::string & value) = 0;
  virtual void present_note(const Note::Ptr & note) = 0;
  virtual void present_search() = 0;
  virtual void new_note() = 0;
};

}

#endif

