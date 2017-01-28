/*
 * gnote
 *
 * Copyright (C) 2017 Aurimas Cernius
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



#include <list>

#include <libxml/tree.h>
#include <UnitTest++/UnitTest++.h>

#include "note.hpp"

SUITE(Note)
{
  TEST(tags)
  {
    Glib::ustring markup = "<tags xmlns=\"http://beatniksoftware.com/tomboy\"><tag>system:notebook:ggoiiii</tag><tag>system:template</tag></tags>";

    xmlDocPtr doc = xmlParseDoc((const xmlChar*)markup.c_str());
    CHECK(doc);

    if(doc) {
      std::list<Glib::ustring> tags;
      gnote::NoteBase::parse_tags(xmlDocGetRootElement(doc), tags);
      CHECK(!tags.empty());

      xmlFreeDoc(doc);
    }
  }
}

