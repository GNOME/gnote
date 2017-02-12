/*
 * "Table of Contents" is a Note add-in for Gnote.
 *  It lists note's table of contents in a menu.
 *
 * Copyright (C) 2013 Luc Pionchon <pionchon.luc@gmail.com>
 * Copyright (C) 2013,2017 Aurimas Cernius
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

/* A subclass of ImageMenuItem to show a toc menu item */

#ifndef __TABLEOFCONTENT_MENU_ITEM_HPP_
#define __TABLEOFCONTENT_MENU_ITEM_HPP_

#include <gtkmm/imagemenuitem.h>

#include "base/macros.hpp"
#include "note.hpp"
#include "tableofcontents.hpp"


namespace tableofcontents {


class TableofcontentsMenuItem : public Gtk::ImageMenuItem
{
public:
  TableofcontentsMenuItem ( const gnote::Note::Ptr & note,
                           const Glib::ustring     & heading,
                           Heading::Type            heading_level,
                           int                      heading_position
                         );

protected:
  virtual void on_activate() override;

private:
  gnote::Note::Ptr m_note;            //the Note referenced by the menu item
  int              m_heading_position; //the position of the heading in the Note
                                        // == offset in the GtkTextBuffer
};


}
#endif
