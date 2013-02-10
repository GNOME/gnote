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

#ifndef _IGNOTE_HPP_
#define _IGNOTE_HPP_

#include "mainwindow.hpp"
#include "base/singleton.hpp"

namespace gnote {

class IGnote
  : public base::Singleton<IGnote>
{
public:
  static std::string cache_dir();
  static std::string conf_dir();
  static std::string data_dir();
  static std::string old_note_dir();

  virtual ~IGnote();
  virtual MainWindow & get_main_window() = 0;
  virtual MainWindow & get_window_for_note() = 0;
  virtual MainWindow & new_main_window() = 0;
  virtual void open_note(const Note::Ptr & note) = 0;
  virtual void open_search_all() = 0;

  sigc::signal<void> signal_quit;
};

}

#endif
