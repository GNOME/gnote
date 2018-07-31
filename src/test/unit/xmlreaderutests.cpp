/*
 * gnote
 *
 * Copyright (C) 2017-2018 Aurimas Cernius
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


#include <UnitTest++/UnitTest++.h>

#include "sharp/xmlreader.hpp"


SUITE(XmlReader)
{
  TEST(simple_test)
  {
    sharp::XmlReader xml;

    Glib::ustring content = "<note-content xmlns:link=\"http://beatniksoftware.com/tomboy/link\">"
                              "Start Here\n\n"
                              "<bold>Welcome to Gnote!</bold>\n\n"
                              "Use this \"Start Here\" note to begin organizing "
                              "your ideas and thoughts.\n\n"
                              "You can create new notes to hold your ideas by "
                              "selecting the \"Create New Note\" item from the "
                              "Gnote menu in your GNOME Panel. "
                              "Your note will be saved automatically.\n\n"
                              "Then organize the notes you create by linking "
                              "related notes and ideas together!\n\n"
                              "We've created a note called "
                              "<link:internal>Using Links in Gnote</link:internal>. "
                              "Notice how each time we type <link:internal>Using "
                              "Links in Gnote</link:internal> it automatically "
                              "gets underlined?  Click on the link to open the note."
                            "</note-content>";

    xml.load_buffer(content);

    CHECK(xml.read());
    CHECK_EQUAL(XML_READER_TYPE_ELEMENT, xml.get_node_type());
    CHECK_EQUAL("note-content", xml.get_name());
    CHECK_EQUAL("http://beatniksoftware.com/tomboy/link", xml.get_attribute("xmlns:link"));

    CHECK_EQUAL("Start Here\n\n"
                  "<bold>Welcome to Gnote!</bold>\n\n"
                  "Use this \"Start Here\" note to begin organizing "
                  "your ideas and thoughts.\n\n"
                  "You can create new notes to hold your ideas by "
                  "selecting the \"Create New Note\" item from the "
                  "Gnote menu in your GNOME Panel. "
                  "Your note will be saved automatically.\n\n"
                  "Then organize the notes you create by linking "
                  "related notes and ideas together!\n\n"
                  "We've created a note called <link:internal "
                  "xmlns:link=\"http://beatniksoftware.com/tomboy/link\">Using Links in Gnote</link:internal>. "
                  "Notice how each time we type <link:internal "
                  "xmlns:link=\"http://beatniksoftware.com/tomboy/link\">Using "
                  "Links in Gnote</link:internal> it automatically "
              "gets underlined?  Click on the link to open the note.", xml.read_inner_xml());
    CHECK(xml.read());
    CHECK_EQUAL(XML_READER_TYPE_TEXT, xml.get_node_type());
  }

  TEST(xmlDoc_test)
  {
    Glib::ustring content = "<note-content xmlns:link=\"http://beatniksoftware.com/tomboy/link\">"
                              "Start Here\n\n"
                              "<bold>Welcome to Gnote!</bold>\n\n"
                              "Use this \"Start Here\" note to begin organizing "
                              "your ideas and thoughts.\n\n"
                              "You can create new notes to hold your ideas by "
                              "selecting the \"Create New Note\" item from the "
                              "Gnote menu in your GNOME Panel. "
                              "Your note will be saved automatically.\n\n"
                              "Then organize the notes you create by linking "
                              "related notes and ideas together!\n\n"
                              "We've created a note called "
                              "<link:internal>Using Links in Gnote</link:internal>. "
                              "Notice how each time we type <link:internal>Using "
                              "Links in Gnote</link:internal> it automatically "
                              "gets underlined?  Click on the link to open the note."
                            "</note-content>";

    xmlDocPtr xml_doc = xmlReadMemory(content.c_str(), content.bytes(), "", "UTF-8", 0);
    CHECK(NULL != xml_doc);
    sharp::XmlReader xml(xml_doc);

    CHECK(xml.read());
    CHECK_EQUAL(XML_READER_TYPE_ELEMENT, xml.get_node_type());
    CHECK_EQUAL("note-content", xml.get_name());
    CHECK_EQUAL("http://beatniksoftware.com/tomboy/link", xml.get_attribute("xmlns:link"));

    CHECK_EQUAL("Start Here\n\n"
                  "<bold>Welcome to Gnote!</bold>\n\n"
                  "Use this \"Start Here\" note to begin organizing "
                  "your ideas and thoughts.\n\n"
                  "You can create new notes to hold your ideas by "
                  "selecting the \"Create New Note\" item from the "
                  "Gnote menu in your GNOME Panel. "
                  "Your note will be saved automatically.\n\n"
                  "Then organize the notes you create by linking "
                  "related notes and ideas together!\n\n"
                  "We've created a note called <link:internal "
                  "xmlns:link=\"http://beatniksoftware.com/tomboy/link\">Using Links in Gnote</link:internal>. "
                  "Notice how each time we type <link:internal "
                  "xmlns:link=\"http://beatniksoftware.com/tomboy/link\">Using "
                  "Links in Gnote</link:internal> it automatically "
              "gets underlined?  Click on the link to open the note.", xml.read_inner_xml());
    CHECK(xml.read());
    CHECK_EQUAL(XML_READER_TYPE_TEXT, xml.get_node_type());
  }

  TEST(unicode_test)
  {
    Glib::ustring content = "<note><title>ąčęėįšų</title><note-content>"
                            "ąčęėįšų\n\n"
                            "Contains some unicode characters"
                            "</note-content></note>";

    sharp::XmlReader xml;
    xml.load_buffer(content);

    CHECK(xml.read());
    CHECK_EQUAL(XML_READER_TYPE_ELEMENT, xml.get_node_type());
    CHECK_EQUAL("note", xml.get_name());
    CHECK(xml.read());
    CHECK_EQUAL(XML_READER_TYPE_ELEMENT, xml.get_node_type());
    CHECK_EQUAL("title", xml.get_name());
    CHECK(xml.read());
    CHECK_EQUAL(XML_READER_TYPE_TEXT, xml.get_node_type());
    CHECK_EQUAL("ąčęėįšų", xml.get_value());
    CHECK(xml.read());
    CHECK_EQUAL(XML_READER_TYPE_END_ELEMENT, xml.get_node_type());
    CHECK(xml.read());
    CHECK_EQUAL(XML_READER_TYPE_ELEMENT, xml.get_node_type());
    CHECK_EQUAL("note-content", xml.get_name());
    CHECK(xml.read());
    CHECK_EQUAL(XML_READER_TYPE_TEXT, xml.get_node_type());
    CHECK_EQUAL("ąčęėįšų\n\nContains some unicode characters", xml.get_value());
    CHECK(xml.read());
    CHECK_EQUAL(XML_READER_TYPE_END_ELEMENT, xml.get_node_type());
    CHECK(xml.read());
    CHECK_EQUAL(XML_READER_TYPE_END_ELEMENT, xml.get_node_type());
    CHECK(!xml.read());
  }
}

