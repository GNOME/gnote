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


#include <glibmm/i18n.h>

#include "iactionmanager.hpp"
#include "ignote.hpp"
#include "statisticsapplicationaddin.hpp"


namespace statistics {

StatisticsModule::StatisticsModule()
{
  ADD_INTERFACE_IMPL(StatisticsApplicationAddin);
}



StatisticsApplicationAddin::StatisticsApplicationAddin()
  : m_initialized(false)
  , m_widget(NULL)
{
}

void StatisticsApplicationAddin::initialize()
{
  if(!m_initialized) {
    m_initialized = true;
    if(m_action == 0) {
      m_action = Gtk::Action::create();
      m_action->set_name("ShowStatistics");
      m_action->set_label(_("Show Statistics"));
      m_action->signal_activate()
        .connect(sigc::mem_fun(*this, &StatisticsApplicationAddin::on_show_statistics));
      gnote::IActionManager::obj().add_main_window_search_action(m_action, 100);
    }
  }
}

void StatisticsApplicationAddin::shutdown()
{
  m_initialized = false;
}

bool StatisticsApplicationAddin::initialized()
{
  return m_initialized;
}

void StatisticsApplicationAddin::on_show_statistics()
{
  if(!m_widget) {
    m_widget = new StatisticsWidget(note_manager());
  }
  gnote::MainWindow &main_window = gnote::IGnote::obj().get_main_window();
  if(m_widget->host()) {
    m_widget->host()->unembed_widget(*m_widget);
  }
  main_window.embed_widget(*m_widget);
}

}

