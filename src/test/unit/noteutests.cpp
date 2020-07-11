/*
 * gnote
 *
 * Copyright (C) 2017,2019-2020 Aurimas Cernius
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
      std::vector<Glib::ustring> tags = gnote::NoteBase::parse_tags(xmlDocGetRootElement(doc));
      CHECK(!tags.empty());

      xmlFreeDoc(doc);
    }
  }

  TEST(parse_text_content_simple)
  {
    Glib::ustring content = "<note-content><note-title>note_title</note-title>\n\ntext content</note-content>";
    auto text = gnote::NoteBase::parse_text_content(std::move(content));
    CHECK_EQUAL("note_title\n\ntext content", text);
  }

  TEST(parse_text_content_whitespace)
  {
    Glib::ustring content = "<note-content><note-title>note_title</note-title>\n\n   </note-content>";
    auto text = gnote::NoteBase::parse_text_content(std::move(content));
    CHECK_EQUAL("note_title\n\n   ", text);
  }

  TEST(parse_text_content_tags)
  {
    Glib::ustring content = "<note-content><note-title>note_title</note-title>\n\ntext <b>cont</b>ent</note-content>";
    auto text = gnote::NoteBase::parse_text_content(std::move(content));
    CHECK_EQUAL("note_title\n\ntext content", text);
  }

  TEST(parse_text_content_list)
  {
    Glib::ustring content = "<note-content><note-title>note_title</note-title>\n\ntext content:<list><list-item>item1</list-item><list-item>item2</list-item></list></note-content>";
    auto text = gnote::NoteBase::parse_text_content(std::move(content));
    CHECK_EQUAL("note_title\n\ntext content:\nitem1\nitem2", text);
  }
}

