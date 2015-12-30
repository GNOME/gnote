/*
 * "Table of Contents" is a Note add-in for Gnote.
 *  It lists note's table of contents in a menu.
 *
 * Copyright (C) 2015 Aurimas Cernius <aurisc4@gmail.com>
 * Copyright (C) 2013 Luc Pionchon <pionchon.luc@gmail.com>
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

#include <glibmm/i18n.h>

#include <gtkmm/stock.h>

#include "iconmanager.hpp"
#include "notewindow.hpp"

#include "tableofcontentsmenuitem.hpp"
#include "tableofcontents.hpp"
#include "tableofcontentsutils.hpp"

namespace tableofcontents {


TableofcontentsMenuItem::TableofcontentsMenuItem (
                            const gnote::Note::Ptr & note,
                            const std::string      & heading,
                            Heading::Type            heading_level,
                            int                      heading_position)
  : m_note            (note)
  , m_heading_position (heading_position)
{
  //Create a new menu item, with style depending on the heading level:
  /* +-----------------+
     |[] NOTE TITLE    | <---- Title     == note icon  + bold note title
     | > Heading 1     | <---- Level_1   == arrow icon + heading title
     | > Heading 1     |
     |   → heading 2   | <---- Level_2   == (no icon)  + indent string + heading title
     |   → heading 2   |
     |   → heading 2   |
     | > Heading 1     |
     +-----------------+
   */

  set_use_underline (false); //we don't want potential '_' in the heading to be used as mnemonic

  if (heading_level == Heading::Title) {
    set_image(*manage(new Gtk::Image(gnote::IconManager::obj().get_icon(gnote::IconManager::NOTE, 16))));
    Gtk::Label *label = (Gtk::Label*)get_child();
    label->set_markup("<b>" + heading + "</b>");
  }
  else if (heading_level == Heading::Level_1) {
    set_image(*manage(new Gtk::Image(Gtk::Stock::GO_FORWARD, Gtk::ICON_SIZE_MENU)));
    set_label(heading);
  }
  else if (heading_level == Heading::Level_2) {
    set_label("→  " + heading);
  }
}


void TableofcontentsMenuItem::on_activate ()
{
  goto_heading(m_note, m_heading_position);
}


} //namespace
