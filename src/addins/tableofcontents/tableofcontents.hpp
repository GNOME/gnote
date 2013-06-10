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

/* An enum for different header levels in the toc */


/* Note: the enum is in this file because
         when it was in tableofcontentsnoteaddin.hpp,
         I could not use it also in tableofcontentsmenuitem.cpp,
         there was a scope error, which I could not solve.
 */

#ifndef __TABLEOFCONTENT_HPP_
#define __TABLEOFCONTENT_HPP_

namespace tableofcontents {

namespace Heading { // Heading level,
  enum Type {      //  Heading::Type     (can be used as a type)
    Title,         //  Heading::Title    == Note title
    Level_1,       //  Heading::Level_1  == 1st level heading == Ctrl-1
    Level_2,       //  Heading::Level_2  == 2nd level heading == Ctrl-2
    None           //  Heading::None
  };
}


}

#endif