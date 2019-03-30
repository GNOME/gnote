/*
 * gnote
 *
 * Copyright (C) 2019 Aurimas Cernius
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

#ifndef _POPOVERWIDGETS_HPP_
#define _POPOVERWIDGETS_HPP_

namespace gnote {

struct PopoverWidget
{
  Gtk::Widget *widget;
  int section;
  int order;
  int secondary_order;

  static PopoverWidget create_for_app(int ord, Gtk::Widget *w);

  PopoverWidget(int sec, int ord, Gtk::Widget *w)
    : widget(w)
    , section(sec)
    , order(ord)
    {}

  bool operator< (const PopoverWidget & other)
    {
      if(section != other.section)
        return section < other.section;
      if(order != other.order)
        return order < other.order;
      return secondary_order < other.secondary_order;
    }
};

}

#endif

