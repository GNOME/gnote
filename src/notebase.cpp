/*
 * gnote
 *
 * Copyright (C) 2011-2014,2017,2019-2020,2022-2024 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
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


#include <algorithm>
#include <functional>

#include <glibmm/i18n.h>

#include "config.h"
#include "debug.hpp"
#include "itagmanager.hpp"
#include "notebase.hpp"
#include "notemanagerbase.hpp"
#include "base/hash.hpp"
#include "sharp/exception.hpp"
#include "sharp/files.hpp"
#include "sharp/map.hpp"
#include "sharp/string.hpp"
#include "sharp/xml.hpp"
#include "sharp/xmlconvert.hpp"



namespace gnote {

NoteDataBufferSynchronizerBase::~NoteDataBufferSynchronizerBase()
{
}

const Glib::ustring & NoteDataBufferSynchronizerBase::text() const
{
  return data().text();
}

void NoteDataBufferSynchronizerBase::set_text(Glib::ustring && t)
{
  data().text() = std::move(t);
}



Glib::ustring NoteBase::url_from_path(const Glib::ustring & filepath)
{
  return "note://gnote/" + sharp::file_basename(filepath);
}

std::vector<Glib::ustring> NoteBase::parse_tags(const xmlNodePtr tagnodes)
{
  std::vector<Glib::ustring> tags;
  sharp::XmlNodeSet nodes = sharp::xml_node_xpath_find(tagnodes, "//*");

  if(nodes.empty()) {
    return tags;
  }
  for(auto & node : nodes) {
    if(xmlStrEqual(node->name, (const xmlChar*)"tag") && (node->type == XML_ELEMENT_NODE)) {
      xmlChar * content = xmlNodeGetContent(node);
      if(content) {
        DBG_OUT("found tag %s", content);
        tags.push_back((const char*)content);
        xmlFree(content);
      }
    }
  }

  return tags;
}

Glib::ustring NoteBase::parse_text_content(const Glib::ustring & content)
{
  xmlDocPtr doc = xmlParseDoc((const xmlChar*)content.c_str());
  if(!doc) {
    return "";
  }

  Glib::ustring ret;
  sharp::XmlReader reader(doc);
  while(reader.read()) {
    switch(reader.get_node_type()) {
    case XML_READER_TYPE_ELEMENT:
      if(reader.get_name() == "list-item") {
        ret += "\n";
      }
      break;
    case XML_READER_TYPE_TEXT:
    case XML_READER_TYPE_WHITESPACE:
    case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
      ret += reader.get_value();
      break;
    default:
      break;
    }
  }

  return ret;
}


NoteBase::NoteBase(const Glib::ustring && filepath, NoteManagerBase & _manager)
  : m_manager(_manager)
  , m_file_path(std::move(filepath))
  , m_enabled(true)
{
}

int NoteBase::get_hash_code() const
{
  Hash<Glib::ustring> h;
  return h(get_title());
}

const Glib::ustring & NoteBase::uri() const
{
  return data_synchronizer().data().uri();
}

const Glib::ustring NoteBase::id() const
{
  return sharp::string_replace_first(data_synchronizer().data().uri(), "note://gnote/","");
}

const Glib::ustring & NoteBase::get_title() const
{
  return data_synchronizer().data().title();
}

void NoteBase::set_title(Glib::ustring && new_title)
{
  set_title(std::move(new_title), false);
}

void NoteBase::set_title(Glib::ustring && new_title, bool from_user_action)
{
  if(data_synchronizer().data().title() != new_title) {
    Glib::ustring old_title = std::move(data_synchronizer().data().title());
    data_synchronizer().data().title() = std::move(new_title);

    if(from_user_action) {
      process_rename_link_update(old_title);
    }
    else {
      signal_renamed(*this, old_title);
      queue_save(CONTENT_CHANGED);
    }
  }
}

void NoteBase::process_rename_link_update(const Glib::ustring & old_title)
{
  for(NoteBase & note : m_manager.get_notes_linking_to(old_title)) {
    note.rename_links(old_title, *this);
  }

  signal_renamed(*this, old_title);
  queue_save(CONTENT_CHANGED);
}

void NoteBase::rename_without_link_update(Glib::ustring && newTitle)
{
  if(data_synchronizer().data().title() != newTitle) {
    data_synchronizer().data().title() = std::move(newTitle);

    // HACK:
    signal_renamed(*this, data_synchronizer().data().title());

    queue_save(CONTENT_CHANGED); // TODO: Right place for this?
  }
}

void NoteBase::queue_save(ChangeType c)
{
  set_change_type(c);
  save();
}

void NoteBase::set_change_type(ChangeType c)
{
  switch(c)
  {
  case CONTENT_CHANGED:
    // NOTE: Updating ChangeDate automatically updates MetdataChangeDate to match.
    data_synchronizer().data().set_change_date(Glib::DateTime::create_now_local());
    break;
  case OTHER_DATA_CHANGED:
    // Only update MetadataChangeDate.  Used by sync/etc
    // to know when non-content note data has changed,
    // but order of notes in menu and search UI is
    // unaffected.
    data_synchronizer().data().metadata_change_date() = Glib::DateTime::create_now_local();
    break;
  default:
    break;
  }
}

void NoteBase::save()
{
  try {
    m_manager.note_archiver().write_file(m_file_path, data_synchronizer().data());
  } 
  catch (const sharp::Exception & e) {
    // Probably IOException or UnauthorizedAccessException?
    ERR_OUT(_("Exception while saving note: %s"), e.what());
  }

  signal_saved(*this);
}

void NoteBase::rename_links(const Glib::ustring & old_title, const NoteBase & renamed)
{
  handle_link_rename(old_title, renamed, true);
}

void NoteBase::remove_links(const Glib::ustring & old_title, const NoteBase & renamed)
{
  handle_link_rename(old_title, renamed, false);
}

void NoteBase::handle_link_rename(const Glib::ustring &, const NoteBase &, bool)
{
}

void NoteBase::delete_note()
{
  // Remove the note from all the tags
  // remove_tag modifies map, so always iterate from start
  auto thetags = data_synchronizer().data().tags();
  auto &tag_manager = m_manager.tag_manager();
  for(auto &thetag : thetags) {
    if(auto tag = tag_manager.get_tag(thetag)) {
      remove_tag(*tag);
    }
  }
}

void NoteBase::add_tag(Tag &tag)
{
  tag.add_note(*this);

  auto &thetags = data_synchronizer().data().tags();
  auto tag_name = tag.normalized_name();
  if(thetags.find(tag_name) != thetags.end()) {
    return;
  }

  thetags.insert(tag_name);

  signal_tag_added(*this, tag);

  DBG_OUT ("Tag added, queueing save");
  queue_save(OTHER_DATA_CHANGED);
}

void NoteBase::remove_tag(Tag & tag)
{
  Glib::ustring tag_name = tag.normalized_name();
  auto & thetags(data_synchronizer().data().tags());

  {
    Tag::ORef iter;
    auto t = thetags.find(tag_name);
    if(t != thetags.end()) {
      iter = manager().tag_manager().get_tag(*t);
    }
    if(!iter) {
      return;
    }
  }

  signal_tag_removing(*this, tag);

  thetags.erase(tag_name);
  tag.remove_note(*this);

  signal_tag_removed(*this, tag_name);

  DBG_OUT("Tag removed, queueing save");
  queue_save(OTHER_DATA_CHANGED);
}

bool NoteBase::contains_tag(const Tag &tag) const
{
  const auto tag_name = tag.normalized_name();
  const auto &tags = data_synchronizer().data().tags();
  return tags.find(tag_name) != tags.end();
}

Glib::ustring NoteBase::get_complete_note_xml()
{
  return m_manager.note_archiver().write_string(data_synchronizer().synchronized_data());
}

void NoteBase::set_xml_content(Glib::ustring && xml)
{
  data_synchronizer().set_text(std::move(xml));
}

Glib::ustring NoteBase::text_content()
{
  return parse_text_content(xml_content());
}

void NoteBase::load_foreign_note_xml(const Glib::ustring & foreignNoteXml, ChangeType changeType)
{
  if(foreignNoteXml.empty())
    throw sharp::Exception("foreignNoteXml");

  // Arguments to this method cannot be trusted.  If this method
  // were to throw an XmlException in the middle of processing,
  // a note could be damaged.  Therefore, we check for parseability
  // ahead of time, and throw early.
  xmlDocPtr doc = xmlParseDoc((const xmlChar *)foreignNoteXml.c_str());

  if(!doc) {
    throw sharp::Exception("invalid XML in foreignNoteXml");
  }
  xmlFreeDoc(doc);

  sharp::XmlReader xml;
  xml.load_buffer(foreignNoteXml);

  // Remove tags now, since a note with no tags has
  // no "tags" element in the XML
  std::vector<Tag::Ref> new_tags;
  Glib::ustring name;

  while(xml.read()) {
    switch(xml.get_node_type()) {
    case XML_READER_TYPE_ELEMENT:
      name = xml.get_name();
      if(name == "title") {
        set_title(xml.read_string());
      }
      else if(name == "text") {
        set_xml_content(xml.read_inner_xml());
      }
      else if(name == "last-change-date") {
        data_synchronizer().data().set_change_date(sharp::XmlConvert::to_date_time(xml.read_string()));
      }
      else if(name == "last-metadata-change-date") {
        data_synchronizer().data().metadata_change_date() = sharp::XmlConvert::to_date_time(xml.read_string());
      }
      else if(name == "create-date") {
        data_synchronizer().data().create_date() = sharp::XmlConvert::to_date_time(xml.read_string());
      }
      else if(name == "tags") {
        xmlDocPtr doc2 = xmlParseDoc((const xmlChar*)xml.read_outer_xml().c_str());
        if(doc2) {
          std::vector<Glib::ustring> tag_strings = parse_tags(doc2->children);
          for(const Glib::ustring & tag_str : tag_strings) {
            new_tags.emplace_back(m_manager.tag_manager().get_or_create_tag(tag_str));
          }
          xmlFreeDoc(doc2);
        }
        else {
          DBG_OUT("loading tag subtree failed");
        }
      }
      break;
    default:
      break;
    }
  }

  xml.close();

  auto tag_list = get_tags();

  for(Tag &iter : tag_list) {
    if(std::any_of(new_tags.begin(), new_tags.end(), [&iter](const Tag::Ref &tag) { return &iter == &tag.get(); })) {
      remove_tag(iter);
    }
  }
  for(Tag &tag : new_tags) {
    add_tag(tag);
  }
    
  // Allow method caller to specify ChangeType (mostly needed by sync)
  queue_save(changeType);
}

std::vector<Tag::Ref> NoteBase::get_tags() const
{
  std::vector<Tag::Ref> ret;
  for(const auto &tag_name : data_synchronizer().data().tags()) {
    if(auto tag = manager().tag_manager().get_tag(tag_name)) {
      ret.push_back(*tag);
    }
  }

  return ret;
}

const NoteData & NoteBase::data() const
{
  return data_synchronizer().synchronized_data();
}

NoteData & NoteBase::data()
{
  return data_synchronizer().synchronized_data();
}

const Glib::DateTime & NoteBase::create_date() const
{
  return data_synchronizer().data().create_date();
}

const Glib::DateTime & NoteBase::change_date() const
{
  return data_synchronizer().data().change_date();
}

const Glib::DateTime & NoteBase::metadata_change_date() const
{
  return data_synchronizer().data().metadata_change_date();
}

bool NoteBase::is_new() const
{
  const NoteDataBufferSynchronizerBase & sync(data_synchronizer());
  return sync.data().create_date() && (sync.data().create_date() > Glib::DateTime::create_now_local().add_hours(-24));
}

void NoteBase::enabled(bool is_enabled)
{
  m_enabled = is_enabled;
}



const char *NoteArchiver::CURRENT_VERSION = "0.3";

void NoteArchiver::read_file(const Glib::ustring & file, NoteData & data)
{
  Glib::ustring version;
  sharp::XmlReader xml(file);
  _read(xml, data, version);
  if(version != NoteArchiver::CURRENT_VERSION) {
    try {
      // Note has old format, so rewrite it.  No need
      // to reread, since we are not adding anything.
      DBG_OUT("Updating note XML from %s to newest format...", version.c_str());
      write_file(file, data);
    }
    catch(sharp::Exception & e) {
      // write failure, but not critical
      ERR_OUT(_("Failed to update note format: %s"), e.what());
    }
  }
}

void NoteArchiver::read(sharp::XmlReader & xml, NoteData & data)
{
  Glib::ustring version; // discarded
  _read(xml, data, version);
}


void NoteArchiver::_read(sharp::XmlReader & xml, NoteData & data, Glib::ustring & version)
{
  Glib::ustring name;

  while(xml.read ()) {
    switch(xml.get_node_type()) {
    case XML_READER_TYPE_ELEMENT:
      name = xml.get_name();

      if(name == "note") {
        version = xml.get_attribute("version");
      }
      else if(name == "title") {
        data.title() = xml.read_string();
      } 
      else if(name == "text") {
        // <text> is just a wrapper around <note-content>
        // NOTE: Use .text here to avoid triggering a save.
        data.text() = xml.read_inner_xml();
      }
      else if(name == "last-change-date") {
        data.set_change_date(sharp::XmlConvert::to_date_time (xml.read_string()));
      }
      else if(name == "last-metadata-change-date") {
        data.metadata_change_date() = sharp::XmlConvert::to_date_time(xml.read_string());
      }
      else if(name == "create-date") {
        data.create_date() = sharp::XmlConvert::to_date_time (xml.read_string());
      }
      else if(name == "cursor-position") {
        data.set_cursor_position(STRING_TO_INT(xml.read_string()));
      }
      else if(name == "selection-bound-position") {
        data.set_selection_bound_position(STRING_TO_INT(xml.read_string()));
      }
      else if(name == "width") {
        data.width() = STRING_TO_INT(xml.read_string());
      }
      else if(name == "height") {
        data.height() = STRING_TO_INT(xml.read_string());
      }
      else if(name == "tags") {
        xmlDocPtr doc2 = xmlParseDoc((const xmlChar*)xml.read_outer_xml().c_str());

        if(doc2) {
          std::vector<Glib::ustring> tag_strings = NoteBase::parse_tags(doc2->children);
          for(const Glib::ustring & tag_str : tag_strings) {
            Tag &tag = m_manager.tag_manager().get_or_create_tag(tag_str);
            data.tags().insert(tag.normalized_name());
          }
          xmlFreeDoc(doc2);
        }
        else {
          DBG_OUT("loading tag subtree failed");
        }
      }
      break;

    default:
      break;
    }
  }
  xml.close ();
}

Glib::ustring NoteArchiver::write_string(const NoteData & note)
{
  Glib::ustring str;
  sharp::XmlWriter xml;
  write(xml, note);
  xml.close();
  str = xml.to_string();
  return str;
}
  

void NoteArchiver::write_file(const Glib::ustring & _write_file, const NoteData & data)
{
  try {
    Glib::ustring tmp_file = _write_file + ".tmp";
    // TODO Xml doc settings
    sharp::XmlWriter xml(tmp_file); //, XmlEncoder::DocumentSettings);
    write(xml, data);
    xml.close();

    if(sharp::file_exists(_write_file)) {
      Glib::ustring backup_path = _write_file + "~";
      if(sharp::file_exists(backup_path)) {
        sharp::file_delete(backup_path);
      }

      // Backup the to a ~ file, just in case
      sharp::file_move(_write_file, backup_path);

      // Move the temp file to write_file
      sharp::file_move(tmp_file, _write_file);

      // Delete the ~ file
      sharp::file_delete(backup_path);
    } 
    else {
      // Move the temp file to write_file
      sharp::file_move(tmp_file, _write_file);
    }
  }
  catch(const std::exception & e) {
    ERR_OUT(_("Filesystem error: %s"), e.what());
  }
}

void NoteArchiver::write(sharp::XmlWriter & xml, const NoteData & data)
{
  xml.write_start_document();
  xml.write_start_element("", "note", "http://beatniksoftware.com/tomboy");
  xml.write_attribute_string("", "version", "", CURRENT_VERSION);
  xml.write_attribute_string("xmlns", "link", "", "http://beatniksoftware.com/tomboy/link");
  xml.write_attribute_string("xmlns", "size", "", "http://beatniksoftware.com/tomboy/size");

  xml.write_start_element("", "title", "");
  xml.write_string(data.title());
  xml.write_end_element();

  xml.write_start_element("", "text", "");
  xml.write_attribute_string("xml", "space", "", "preserve");
  // Insert <note-content> blob...
  xml.write_raw(data.text());
  xml.write_end_element();

  xml.write_start_element("", "last-change-date", "");
  xml.write_string(sharp::XmlConvert::to_string(data.change_date()));
  xml.write_end_element();

  xml.write_start_element("", "last-metadata-change-date", "");
  xml.write_string(sharp::XmlConvert::to_string(data.metadata_change_date()));
  xml.write_end_element();

  if(data.create_date()) {
    xml.write_start_element("", "create-date", "");
    xml.write_string(sharp::XmlConvert::to_string(data.create_date()));
    xml.write_end_element();
  }

  xml.write_start_element("", "cursor-position", "");
  xml.write_string(TO_STRING(data.cursor_position()));
  xml.write_end_element();

  xml.write_start_element("", "selection-bound-position", "");
  xml.write_string(TO_STRING(data.selection_bound_position()));
  xml.write_end_element();

  xml.write_start_element ("", "width", "");
  xml.write_string(TO_STRING(data.width()));
  xml.write_end_element ();

  xml.write_start_element("", "height", "");
  xml.write_string(TO_STRING(data.height()));
  xml.write_end_element();

  if(data.tags().size() > 0) {
    xml.write_start_element("", "tags", "");
    for(const auto & tag_name : data.tags()) {
      xml.write_start_element("", "tag", "");
      xml.write_string(tag_name);
      xml.write_end_element();
    }
    xml.write_end_element();
  }

  xml.write_end_element(); // Note
  xml.write_end_document();
}
  
Glib::ustring NoteArchiver::get_renamed_note_xml(const Glib::ustring & note_xml, 
                                                 const Glib::ustring & old_title,
                                                 const Glib::ustring & new_title) const
{
  Glib::ustring updated_xml;
  // Replace occurences of oldTitle with newTitle in noteXml
  Glib::ustring titleTagPattern = Glib::ustring::compose("<title>%1</title>", old_title);
  Glib::ustring titleTagReplacement = Glib::ustring::compose("<title>%1</title>", new_title);
  updated_xml = sharp::string_replace_regex(note_xml, titleTagPattern, titleTagReplacement);

  Glib::ustring titleContentPattern = "<note-content([^>]*)>\\s*" + old_title;
  Glib::ustring titleContentReplacement = "<note-content\\1>" + new_title;
  Glib::ustring updated_xml2 = sharp::string_replace_regex(updated_xml, titleContentPattern, titleContentReplacement);

  return updated_xml2;
}

Glib::ustring NoteArchiver::get_title_from_note_xml(const Glib::ustring & noteXml) const
{
  if(!noteXml.empty()) {
    sharp::XmlReader xml;

    xml.load_buffer(noteXml);

    while(xml.read ()) {
      switch(xml.get_node_type()) {
      case XML_READER_TYPE_ELEMENT:
        if(xml.get_name() == "title") {
          return xml.read_string ();
        }
        break;
      default:
        break;
      }
    }
  }

  return "";
}
 
}

