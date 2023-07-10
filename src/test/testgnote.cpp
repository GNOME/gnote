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


#include "testgnote.hpp"

namespace test {

Gnote::Gnote()
  : m_notebook_manager(nullptr)
  , m_sync_manager(nullptr)
{
}

gnote::IActionManager & Gnote::action_manager()
{
  throw std::logic_error(std::string("Not implemented ") + __FUNCTION__);
}

gnote::notebooks::NotebookManager & Gnote::notebook_manager()
{
  if(m_notebook_manager == nullptr) {
    throw std::logic_error("NotebookManager not set");
  }

  return *m_notebook_manager;
}

void Gnote::notebook_manager(gnote::notebooks::NotebookManager *manager)
{
  m_notebook_manager = manager;
}

gnote::sync::ISyncManager & Gnote::sync_manager()
{
  if(m_sync_manager == nullptr) {
    throw std::logic_error("SyncManager not set");
  }

  return *m_sync_manager;
}

void Gnote::sync_manager(gnote::sync::ISyncManager* manager)
{
  m_sync_manager = manager;
}

gnote::Preferences & Gnote::preferences()
{
  throw std::logic_error(std::string("Not implemented ") + __FUNCTION__);
}

gnote::MainWindow & Gnote::get_main_window()
{
  throw std::logic_error(std::string("Not implemented ") + __FUNCTION__);
}

gnote::MainWindow & Gnote::get_window_for_note()
{
  throw std::logic_error(std::string("Not implemented ") + __FUNCTION__);
}

gnote::MainWindow & Gnote::new_main_window()
{
  throw std::logic_error(std::string("Not implemented ") + __FUNCTION__);
}

void Gnote::open_note(const gnote::NoteBase & /*note*/)
{
  throw std::logic_error(std::string("Not implemented ") + __FUNCTION__);
}

gnote::MainWindow & Gnote::open_search_all()
{
  throw std::logic_error(std::string("Not implemented ") + __FUNCTION__);
}

}

