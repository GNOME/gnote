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

#ifndef _STATISTICS_APPLICATION_ADDIN_
#define _STATISTICS_APPLICATION_ADDIN_

#include <gtkmm/action.h>

#include "applicationaddin.hpp"
#include "statisticswidget.hpp"
#include "sharp/dynamicmodule.hpp"

namespace statistics {

class StatisticsModule
  : public sharp::DynamicModule
{
public:
  StatisticsModule();
};

DECLARE_MODULE(StatisticsModule);

class StatisticsApplicationAddin
  : public gnote::ApplicationAddin
{
public:
  static StatisticsApplicationAddin *create()
    {
      return new StatisticsApplicationAddin;
    }
  virtual void initialize();
  virtual void shutdown();
  virtual bool initialized();
private:
  StatisticsApplicationAddin();
  void on_show_statistics();

  bool m_initialized;
  Glib::RefPtr<Gtk::Action> m_action;
  StatisticsWidget *m_widget;
};

}

#endif

