/*
 * "Table of Contents" is a Note add-in for Gnote.
 *  It lists note's table of contents in a menu.
 *
 * Copyright (C) 2013,2015,2023 Aurimas Cernius
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


#include "notewindow.hpp"
#include "tableofcontentsutils.hpp"


namespace tableofcontents {

void goto_heading(gnote::Note & note, int heading_position)
{
  // Scroll the TextView and place the cursor on the heading
  Gtk::TextIter heading_iter;
  heading_iter = note.get_buffer()->get_iter_at_offset(heading_position);
  note.get_window()->editor()->scroll_to(heading_iter,
                                          0.0, //[0.0,0.5] within_margin, margin as a fraction of screen size.
                                          0.0, //[0.0,1.0] horizontal alignment of mark within visible area.
                                          0.0);//[0.0,1.0] vertical alignment of mark within visible area.
  note.get_buffer()->place_cursor(heading_iter);
}

}

