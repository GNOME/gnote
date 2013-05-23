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


#ifndef _STATISTICS_WIDGET_HPP_
#define _STATISTICS_WIDGET_HPP_

#include <gtkmm/treeview.h>

#include "mainwindowembeds.hpp"
#include "notemanager.hpp"


namespace statistics {

class StatisticsWidget
  : public Gtk::TreeView
  , public gnote::EmbeddableWidget
{
public:
  StatisticsWidget(gnote::NoteManager & nm);
  virtual std::string get_name() const;
  virtual void foreground();
  virtual void background();
private:
  void col1_data_func(Gtk::CellRenderer *renderer, const Gtk::TreeIter & iter);
  void col2_data_func(Gtk::CellRenderer *renderer, const Gtk::TreeIter & iter);
};

}

#endif

