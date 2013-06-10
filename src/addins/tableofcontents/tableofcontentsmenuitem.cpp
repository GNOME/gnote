/*
 * "Table of Contents" is a Note add-in for Gnote.
 *  It lists note's table of contents in a menu.
 *
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
  if (!m_note) {
    return;
  }

  // Scroll the TextView and place the cursor on the heading
  Gtk::TextIter heading_iter;
  heading_iter = m_note->get_buffer()->get_iter_at_offset (m_heading_position);
  m_note->get_window()->editor()->scroll_to (heading_iter,
                                              0.0, //[0.0,0.5] within_margin, margin as a fraction of screen size.
                                              0.0, //[0.0,1.0] horizontal alignment of mark within visible area.
                                              0.0);//[0.0,1.0] vertical alignment of mark within visible area.
  m_note->get_buffer()->place_cursor (heading_iter);
}


} //namespace
