/*
 * gnote
 *
 * Copyright (C) 2013,2015 Aurimas Cernius
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
#include "utils.hpp"


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
    auto & manager(gnote::IActionManager::obj());
    manager.register_main_window_search_callback("statistics-show-cback",
      "statistics-show", sigc::mem_fun(*this, &StatisticsApplicationAddin::on_show_statistics));
    m_add_menu_item_cid = manager.signal_build_main_window_search_popover
      .connect(sigc::mem_fun(*this, &StatisticsApplicationAddin::add_menu_item));
  }
}

void StatisticsApplicationAddin::shutdown()
{
  gnote::IActionManager::obj().unregister_main_window_search_callback("statistics-show-cback");
  m_add_menu_item_cid.disconnect();
  m_initialized = false;
}

bool StatisticsApplicationAddin::initialized()
{
  return m_initialized;
}

void StatisticsApplicationAddin::add_menu_item(std::map<int, Gtk::Widget*> & widgets)
{
  auto item = gnote::utils::create_popover_button("win.statistics-show", _("Show Statistics"));
  gnote::utils::add_item_to_ordered_map(widgets, 100, item);
}

void StatisticsApplicationAddin::on_show_statistics(const Glib::VariantBase&)
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

