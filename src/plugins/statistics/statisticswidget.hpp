/*
 * gnote
 *
 * Copyright (C) 2013,2017,2019,2023 Aurimas Cernius
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


#ifndef _STATISTICS_WIDGET_HPP_
#define _STATISTICS_WIDGET_HPP_

#include <gtkmm/listview.h>

#include "mainwindowembeds.hpp"
#include "notemanager.hpp"


namespace statistics {

class StatisticsWidget
  : public Gtk::ListView
  , public gnote::EmbeddableWidget
{
public:
  StatisticsWidget(gnote::IGnote & g, gnote::NoteManager & nm);
  virtual Glib::ustring get_name() const override;
  virtual void foreground() override;
  virtual void background() override;
};

}

#endif

