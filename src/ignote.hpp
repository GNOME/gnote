/*
 * gnote
 *
 * Copyright (C) 2013,2016-2017,2019,2022-2023 Aurimas Cernius
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

namespace gnote {

class IActionManager;
class Preferences;
namespace notebooks {
  class NotebookManager;
}
namespace sync {
  class ISyncManager;
}


class IGnote
{
public:
  static Glib::ustring cache_dir();
  static Glib::ustring conf_dir();
  static Glib::ustring data_dir();
  static Glib::ustring old_note_dir();

  virtual ~IGnote();

  virtual IActionManager & action_manager() = 0;
  virtual notebooks::NotebookManager & notebook_manager() = 0;
  virtual sync::ISyncManager & sync_manager() = 0;
  virtual Preferences & preferences() = 0;

  virtual MainWindow & get_main_window() = 0;
  virtual MainWindow & get_window_for_note() = 0;
  virtual MainWindow & new_main_window() = 0;
  virtual void open_note(const NoteBase & note) = 0;
  virtual MainWindow & open_search_all() = 0;

  sigc::signal<void()> signal_quit;
};

}

#endif
