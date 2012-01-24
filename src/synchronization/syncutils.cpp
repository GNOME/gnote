/*
 * gnote
 *
 * Copyright (C) 2012 Aurimas Cernius
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


#include "syncutils.hpp"
#include "sharp/xmlreader.hpp"

namespace gnote {
namespace sync {

  NoteUpdate::NoteUpdate(const std::string & xml_content, const std::string & title, const std::string & uuid, int latest_revision)
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


  bool NoteUpdate::basically_equal_to(const Note::Ptr & existing_note)
  {
    // NOTE: This would be so much easier if NoteUpdate
    //       was not just a container for a big XML string
    sharp::XmlReader xml;
    xml.load_buffer(m_xml_content);
    std::auto_ptr<NoteData> update_data(NoteArchiver::obj().read(xml, m_uuid));
    xml.close();

    // NOTE: Mostly a hack to ignore missing version attributes
    std::string existing_inner_content = get_inner_content(existing_note->data().text());
    std::string update_inner_content = get_inner_content(update_data->text());

    return existing_inner_content == update_inner_content &&
           existing_note->data().title() == update_data->title() &&
           compare_tags(existing_note->data().tags(), update_data->tags());
           // TODO: Compare open-on-startup, pinned
  }


  std::string NoteUpdate::get_inner_content(const std::string & full_content_element) const
  {
    /*const string noteContentRegex =
				"^<note-content([^>]+version=""(?<contentVersion>[^""]*)"")?[^>]*((/>)|(>(?<innerContent>.*)</note-content>))$";
			Match m = Regex.Match (fullContentElement, noteContentRegex, RegexOptions.Singleline);
			Group contentGroup = m.Groups ["innerContent"];
			if (!contentGroup.Success)
				return null;
			return contentGroup.Value;*/
    sharp::XmlReader xml;
    xml.load_buffer(full_content_element);
    if(xml.read() && xml.get_name() == "note-content") {
      return xml.read_inner_xml();
    }
    return "";
  }


  bool NoteUpdate::compare_tags(const std::map<std::string, Tag::Ptr> set1, const std::map<std::string, Tag::Ptr> set2) const
  {
    if(set1.size() != set2.size()) {
      return false;
    }
    for(std::map<std::string, Tag::Ptr>::const_iterator iter = set1.begin(); iter != set1.end(); ++iter) {
      if(set2.find(iter->first) == set2.end()) {
        return false;
      }
    }
    return true;
  }

}
}
