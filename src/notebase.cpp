/*
 * gnote
 *
 * Copyright (C) 2011-2014,2017 Aurimas Cernius
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
#include "sharp/exception.hpp"
#include "sharp/files.hpp"
#include "sharp/map.hpp"
#include "sharp/string.hpp"
#include "sharp/xml.hpp"
#include "sharp/xmlconvert.hpp"



namespace gnote {

NoteDataBufferSynchronizerBase::~NoteDataBufferSynchronizerBase()
{
  delete m_data;
}

const Glib::ustring & NoteDataBufferSynchronizerBase::text()
{
  return data().text();
}

void NoteDataBufferSynchronizerBase::set_text(const Glib::ustring & t)
{
  data().text() = t;
}



Glib::ustring NoteBase::url_from_path(const Glib::ustring & filepath)
{
  return "note://gnote/" + sharp::file_basename(filepath);
}

void NoteBase::parse_tags(const xmlNodePtr tagnodes, std::list<Glib::ustring> & tags)
{
  sharp::XmlNodeSet nodes = sharp::xml_node_xpath_find(tagnodes, "//*");

  if(nodes.empty()) {
    return;
  }
  for(sharp::XmlNodeSet::const_iterator iter = nodes.begin(); iter != nodes.end(); ++iter) {
    const xmlNodePtr node = *iter;
    if(xmlStrEqual(node->name, (const xmlChar*)"tag") && (node->type == XML_ELEMENT_NODE)) {
      xmlChar * content = xmlNodeGetContent(node);
      if(content) {
        DBG_OUT("found tag %s", content);
        tags.push_back((const char*)content);
        xmlFree(content);
      }
    }
  }
}


NoteBase::NoteBase(NoteData *, const Glib::ustring & filepath, NoteManagerBase & _manager)
  : m_manager(_manager)
  , m_file_path(filepath)
  , m_enabled(true)
{
}

int NoteBase::get_hash_code() const
{
  std::hash<std::string> h;
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

void NoteBase::set_title(const Glib::ustring & new_title)
{
  set_title(new_title, false);
}

void NoteBase::set_title(const Glib::ustring & new_title, bool from_user_action)
{
  if(data_synchronizer().data().title() != new_title) {
    Glib::ustring old_title = data_synchronizer().data().title();
    data_synchronizer().data().title() = new_title;

    if(from_user_action) {
      process_rename_link_update(old_title);
    }
    else {
      signal_renamed(shared_from_this(), old_title);
      queue_save(CONTENT_CHANGED);
    }
  }
}

void NoteBase::process_rename_link_update(const Glib::ustring & old_title)
{
  NoteBase::List linking_notes = m_manager.get_notes_linking_to(old_title);
  const NoteBase::Ptr self = shared_from_this();

  if(!linking_notes.empty()) {
    FOREACH(NoteBase::Ptr & note, linking_notes) {
      note->rename_links(old_title, self);
      signal_renamed(shared_from_this(), old_title);
      queue_save(CONTENT_CHANGED);
    }
  }
}

void NoteBase::rename_without_link_update(const Glib::ustring & newTitle)
{
  if(data_synchronizer().data().title() != newTitle) {
    data_synchronizer().data().title() = newTitle;

    // HACK:
    signal_renamed(shared_from_this(), newTitle);

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
    data_synchronizer().data().set_change_date(sharp::DateTime::now());
    break;
  case OTHER_DATA_CHANGED:
    // Only update MetadataChangeDate.  Used by sync/etc
    // to know when non-content note data has changed,
    // but order of notes in menu and search UI is
    // unaffected.
    data_synchronizer().data().metadata_change_date() = sharp::DateTime::now();
    break;
  default:
    break;
  }
}

void NoteBase::save()
{
  try {
    NoteArchiver::write(m_file_path, data_synchronizer().data());
  } 
  catch (const sharp::Exception & e) {
    // Probably IOException or UnauthorizedAccessException?
    ERR_OUT(_("Exception while saving note: %s"), e.what());
  }

  signal_saved(shared_from_this());
}

void NoteBase::rename_links(const Glib::ustring & old_title, const Ptr & renamed)
{
  handle_link_rename(old_title, renamed, true);
}

void NoteBase::remove_links(const Glib::ustring & old_title, const Ptr & renamed)
{
  handle_link_rename(old_title, renamed, false);
}

void NoteBase::handle_link_rename(const Glib::ustring &, const Ptr &, bool)
{
}

void NoteBase::delete_note()
{
  // Remove the note from all the tags
  for(NoteData::TagMap::const_iterator iter = data_synchronizer().data().tags().begin();
      iter != data_synchronizer().data().tags().end(); ++iter) {
    remove_tag(iter->second);
  }
}

void NoteBase::add_tag(const Tag::Ptr & tag)
{
  if(!tag) {
    throw sharp::Exception ("note::add_tag() called with a NULL tag.");
  }
  tag->add_note(*this);

  NoteData::TagMap & thetags(data_synchronizer().data().tags());
  if(thetags.find(tag->normalized_name()) == thetags.end()) {
    thetags[tag->normalized_name()] = tag;

    signal_tag_added(*this, tag);

    DBG_OUT ("Tag added, queueing save");
    queue_save(OTHER_DATA_CHANGED);
  }
}

void NoteBase::remove_tag(Tag & tag)
{
  Glib::ustring tag_name = tag.normalized_name();
  NoteData::TagMap & thetags(data_synchronizer().data().tags());
  NoteData::TagMap::iterator iter;

  iter = thetags.find(tag_name);
  if(iter == thetags.end())  {
    return;
  }

  signal_tag_removing(*this, tag);

  thetags.erase(iter);
  tag.remove_note(*this);

  signal_tag_removed(shared_from_this(), tag_name);

  DBG_OUT("Tag removed, queueing save");
  queue_save(OTHER_DATA_CHANGED);
}

void NoteBase::remove_tag(const Tag::Ptr & tag)
{
  if(!tag)
    throw sharp::Exception("Note.RemoveTag () called with a null tag.");
  remove_tag(*tag);
}

bool NoteBase::contains_tag(const Tag::Ptr & tag) const
{
  if(!tag) {
    return false;
  }
  const NoteData::TagMap & thetags(data_synchronizer().data().tags());
  return (thetags.find(tag->normalized_name()) != thetags.end());
}

Glib::ustring NoteBase::get_complete_note_xml()
{
  return NoteArchiver::write_string(data_synchronizer().synchronized_data());
}

void NoteBase::set_xml_content(const Glib::ustring & xml)
{
  data_synchronizer().set_text(xml);
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
  std::list<Tag::Ptr> new_tags;
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
          std::list<Glib::ustring> tag_strings;
          parse_tags(doc2->children, tag_strings);
          FOREACH(Glib::ustring & tag_str, tag_strings) {
            Tag::Ptr tag = ITagManager::obj().get_or_create_tag(tag_str);
            new_tags.push_back(tag);
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

  std::list<Tag::Ptr> tag_list;
  get_tags(tag_list);

  FOREACH(Tag::Ptr & iter, tag_list) {
    if(std::find(new_tags.begin(), new_tags.end(), iter) == new_tags.end()) {
      remove_tag(iter);
    }
  }
  FOREACH(Tag::Ptr & iter, new_tags) {
    add_tag(iter);
  }
    
  // Allow method caller to specify ChangeType (mostly needed by sync)
  queue_save(changeType);
}

void NoteBase::get_tags(std::list<Tag::Ptr> & l) const
{
  sharp::map_get_values(data_synchronizer().data().tags(), l);
}

const NoteData & NoteBase::data() const
{
  return data_synchronizer().synchronized_data();
}

NoteData & NoteBase::data()
{
  return data_synchronizer().synchronized_data();
}

const sharp::DateTime & NoteBase::create_date() const
{
  return data_synchronizer().data().create_date();
}

const sharp::DateTime & NoteBase::change_date() const
{
  return data_synchronizer().data().change_date();
}

const sharp::DateTime & NoteBase::metadata_change_date() const
{
  return data_synchronizer().data().metadata_change_date();
}

bool NoteBase::is_new() const
{
  const NoteDataBufferSynchronizerBase & sync(data_synchronizer());
  return sync.data().create_date().is_valid() && (sync.data().create_date() > sharp::DateTime::now().add_hours(-24));
}

void NoteBase::enabled(bool is_enabled)
{
  m_enabled = is_enabled;
}



const char *NoteArchiver::CURRENT_VERSION = "0.3";

//instance
NoteArchiver NoteArchiver::s_obj;

void NoteArchiver::read(const Glib::ustring & read_file, NoteData & data)
{
  return obj().read_file(read_file, data);
}

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
      NoteArchiver::write(file, data);
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
          std::list<Glib::ustring> tag_strings;
          NoteBase::parse_tags(doc2->children, tag_strings);
          FOREACH(Glib::ustring & tag_str, tag_strings) {
            Tag::Ptr tag = ITagManager::obj().get_or_create_tag(tag_str);
            data.tags()[tag->normalized_name()] = tag;
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
  obj().write(xml, note);
  xml.close();
  str = xml.to_string();
  return str;
}
  

void NoteArchiver::write(const Glib::ustring & write_file, const NoteData & data)
{
  obj().write_file(write_file, data);
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

  if(data.create_date().is_valid()) {
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
    for(NoteData::TagMap::const_iterator iter = data.tags().begin();
        iter != data.tags().end(); ++iter) {
      xml.write_start_element("", "tag", "");
      xml.write_string(iter->second->name());
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

