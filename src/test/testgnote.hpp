/*
 * gnote
 *
 * Copyright (C) 2019,2023 Aurimas Cernius
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


#ifndef _TEST_TESTGNOTE_HPP_
#define _TEST_TESTGNOTE_HPP_

#include "ignote.hpp"

namespace test {

class Gnote
  : public gnote::IGnote
{
public:
  Gnote();
  virtual ~Gnote()
  {}

  virtual gnote::IActionManager & action_manager() override;
  virtual gnote::notebooks::NotebookManager & notebook_manager() override;
  void notebook_manager(gnote::notebooks::NotebookManager *manager);
  virtual gnote::sync::ISyncManager & sync_manager() override;
  void sync_manager(gnote::sync::ISyncManager* manager);
  virtual gnote::Preferences & preferences() override;

  virtual gnote::MainWindow & get_main_window() override;
  virtual gnote::MainWindow & get_window_for_note() override;
  virtual gnote::MainWindow & new_main_window() override;
  void open_note(const gnote::NoteBase & note) override;
  virtual gnote::MainWindow & open_search_all() override;
private:
  gnote::notebooks::NotebookManager *m_notebook_manager;
  gnote::sync::ISyncManager *m_sync_manager;
};

}

#endif

