/*
 * gnote
 *
 * Copyright (C) 2012-2014,2016-2017,2019,2021,2023-2025 Aurimas Cernius
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


#include "notemanagerbase.hpp"
#include "syncutils.hpp"
#include "sharp/xmlreader.hpp"

namespace gnote {
namespace sync {

  NoteUpdate::NoteUpdate(const Glib::ustring & xml_content, const Glib::ustring & title, const Glib::ustring & uuid, int latest_revision)
  {
    m_xml_content = xml_content;
    m_title = title;
    m_uuid = uuid;
    m_latest_revision = latest_revision;

    // TODO: Clean this up (and remove title parameter?)
    if(m_xml_content.length() > 0) {
      sharp::XmlReader xml;
      xml.load_buffer(m_xml_content);
      //xml.Namespaces = false;

      while(xml.read()) {
        if(xml.get_node_type() == XML_READER_TYPE_ELEMENT) {
          if(xml.get_name() == "title") {
            m_title = xml.read_string();
          }
        }
      }
    }
  }


  bool NoteUpdate::basically_equal_to(const NoteBase &existing_note) const
  {
    // NOTE: This would be so much easier if NoteUpdate
    //       was not just a container for a big XML string
    sharp::XmlReader xml;
    xml.load_buffer(m_xml_content);
    NoteData update_data{Glib::ustring(m_uuid)};
    const_cast<NoteManagerBase&>(existing_note.manager()).note_archiver().read(xml, update_data);
    xml.close();

    // NOTE: Mostly a hack to ignore missing version attributes
    Glib::ustring existing_inner_content = get_inner_content(existing_note.data().text());
    Glib::ustring update_inner_content = get_inner_content(update_data.text());

    return existing_inner_content == update_inner_content &&
           existing_note.data().title() == update_data.title() &&
           compare_tags(existing_note.data().tags(), update_data.tags());
           // TODO: Compare open-on-startup, pinned
  }


  Glib::ustring NoteUpdate::get_inner_content(const Glib::ustring & full_content_element) const
  {
    sharp::XmlReader xml;
    xml.load_buffer(full_content_element);
    if(xml.read() && xml.get_name() == "note-content") {
      return xml.read_inner_xml();
    }
    return "";
  }


  bool NoteUpdate::compare_tags(const NoteData::TagSet &set1, const NoteData::TagSet &set2) const
  {
    if(set1.size() != set2.size()) {
      return false;
    }
    for(const auto &iter : set1) {
      if(set2.find(iter) != set1.end()) {
        return true;
      }
    }
    return false;
  }

}
}
