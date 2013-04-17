/*
 * gnote
 *
 * Copyright (C) 2012-2013 Aurimas Cernius
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


#ifndef _SYNCHRONIZATION_GNOTESYNCCLIENT_HPP_
#define _SYNCHRONIZATION_GNOTESYNCCLIENT_HPP_


#include <boost/lexical_cast.hpp>
#include <glibmm/i18n.h>

#include "debug.hpp"
#include "ignote.hpp"
#include "gnotesyncclient.hpp"
#include "notemanager.hpp"
#include "sharp/files.hpp"
#include "sharp/xmlreader.hpp"
#include "sharp/xmlwriter.hpp"


namespace gnote {
namespace sync {

  const char * GnoteSyncClient::LOCAL_MANIFEST_FILE_NAME = "manifest.xml";

  GnoteSyncClient::GnoteSyncClient(NoteManager & manager)
  {
    m_local_manifest_file_path = Glib::build_filename(IGnote::conf_dir(), LOCAL_MANIFEST_FILE_NAME);
    // TODO: Why doesn't OnChanged ever get fired?!
    Glib::RefPtr<Gio::File> manifest = Gio::File::create_for_path(m_local_manifest_file_path);
    if(manifest != 0) {
      m_file_watcher = manifest->monitor_file();
      m_file_watcher->signal_changed()
        .connect(sigc::mem_fun(*this, &GnoteSyncClient::on_changed));
    }

    parse(m_local_manifest_file_path);

    manager.signal_note_deleted
      .connect(sigc::mem_fun(*this, &GnoteSyncClient::note_deleted_handler));
  }


  void GnoteSyncClient::note_deleted_handler(const Note::Ptr & deletedNote)
  {
    m_deleted_notes[deletedNote->id()] = deletedNote->get_title();
    m_file_revisions.erase(deletedNote->id());

    write(m_local_manifest_file_path);
  }


  void GnoteSyncClient::on_changed(const Glib::RefPtr<Gio::File>&, const Glib::RefPtr<Gio::File>&,
                                   Gio::FileMonitorEvent)
  {
    parse(m_local_manifest_file_path);
  }


  void GnoteSyncClient::read_updated_note_atts(sharp::XmlReader & reader)
  {
    std::string guid, rev;
    while(reader.move_to_next_attribute()) {
      if(reader.get_name() == "guid") {
	guid = reader.get_value();
      }
      else if(reader.get_name() == "latest-revision") {
	rev = reader.get_value();
      }
    }
    int revision = -1;
    try {
      revision = boost::lexical_cast<int>(rev);
    }
    catch(...) {}
    if(guid != "") {
      m_file_revisions[guid] = revision;
    }
  }


  void GnoteSyncClient::read_deleted_note_atts(sharp::XmlReader & reader)
  {
    std::string guid, title;
    while(reader.move_to_next_attribute()) {
      if(reader.get_name() == "guid") {
	guid = reader.get_value();
      }
      else if(reader.get_name() == "title") {
	title = reader.get_value();
      }
    }
    if(guid != "") {
      m_deleted_notes[guid] = title;
    }
  }


  void GnoteSyncClient::read_notes(sharp::XmlReader & reader, void (GnoteSyncClient::*read_note_atts)(sharp::XmlReader&))
  {
    while(reader.read()) {
      if(reader.get_node_type() == XML_READER_TYPE_END_ELEMENT) {
	return;
      }
      if(reader.get_node_type() == XML_READER_TYPE_ELEMENT) {
	if(reader.get_name() == "note") {
	  std::string guid, rev;
	  (this->*read_note_atts)(reader);
	}
      }
    }
  }


  void GnoteSyncClient::parse(const std::string & manifest_path)
  {
    // Set defaults before parsing
    m_last_sync_date = sharp::DateTime::now().add_days(-1);
    m_last_sync_rev = -1;
    m_file_revisions.clear();
    m_deleted_notes.clear();
    m_server_id = "";

    if(!sharp::file_exists(manifest_path)) {
      m_last_sync_date = sharp::DateTime();
      write(manifest_path);
    }

    sharp::XmlReader reader(manifest_path);
    while(reader.read()) {
      if(reader.get_node_type() == XML_READER_TYPE_ELEMENT) {
	if(reader.get_name() == "last-sync-date") {
	  std::string value = reader.read_string();
	  try {
	    m_last_sync_date = sharp::DateTime::from_iso8601(value);
	  }
	  catch(...) {
            /* TRANSLATORS: %s is file */
	    ERR_OUT(_("Unparsable last-sync-date element in %s"), manifest_path.c_str());
	  }
	}
	else if(reader.get_name() == "last-sync-rev") {
	  std::string value = reader.read_string();
	  try {
	    m_last_sync_rev = boost::lexical_cast<int>(value);
	  }
	  catch(...) {
            /* TRANSLATORS: %s is file */
	    ERR_OUT(_("Unparsable last-sync-rev element in %s"), manifest_path.c_str());
	  }
	}
	else if(reader.get_name() == "server-id") {
	  m_server_id = reader.read_string();
	}
	else if(reader.get_name() == "note-revisions") {
	  read_notes(reader, &GnoteSyncClient::read_updated_note_atts);
	}
	else if(reader.get_name() == "note-deletions") {
	  read_notes(reader, &GnoteSyncClient::read_deleted_note_atts);
	}
      }
    }
  }


  void GnoteSyncClient::write(const std::string & manifest_path)
  {
    sharp::XmlWriter xml(manifest_path);

    try {
      xml.write_start_document();
      xml.write_start_element("", "manifest", "http://beatniksoftware.com/tomboy");

      xml.write_start_element("", "last-sync-date", "");
      xml.write_string(m_last_sync_date.to_iso8601());
      xml.write_end_element();

      xml.write_start_element("", "last-sync-rev", "");
      xml.write_string(boost::lexical_cast<std::string>(m_last_sync_rev));
      xml.write_end_element();

      xml.write_start_element("", "server-id", "");
      xml.write_string(m_server_id);
      xml.write_end_element();

      xml.write_start_element("", "note-revisions", "");

      for(std::map<std::string, int>::iterator noteGuid = m_file_revisions.begin();
          noteGuid != m_file_revisions.end(); ++noteGuid) {
	xml.write_start_element("", "note", "");
	xml.write_attribute_string("", "guid", "", noteGuid->first);
	xml.write_attribute_string("", "latest-revision", "", boost::lexical_cast<std::string>(noteGuid->second));
	xml.write_end_element();
      }

      xml.write_end_element(); // </note-revisons>

      xml.write_start_element("", "note-deletions", "");

      for(std::map<std::string, std::string>::iterator noteGuid = m_deleted_notes.begin();
          noteGuid != m_deleted_notes.end(); ++noteGuid) {
	xml.write_start_element("", "note", "");
	xml.write_attribute_string("", "guid", "", noteGuid->first);
	xml.write_attribute_string("", "title", "", noteGuid->second);
	xml.write_end_element();
      }

      xml.write_end_element(); // </note-deletions>

      xml.write_end_element(); // </manifest>
      xml.close();
    }
    catch(...) {
      xml.close();
      throw;
    }
  }


  void GnoteSyncClient::last_sync_date(const sharp::DateTime & date)
  {
    m_last_sync_date = date;
    // If we just did a sync, we should be able to forget older deleted notes
    m_deleted_notes.clear();
    write(m_local_manifest_file_path);
  }


  void GnoteSyncClient::last_synchronized_revision(int revision)
  {
    m_last_sync_rev = revision;
    write(m_local_manifest_file_path);
  }


  int GnoteSyncClient::get_revision(const Note::Ptr & note)
  {
    std::string note_guid = note->id();
    std::map<std::string, int>::const_iterator iter = m_file_revisions.find(note_guid);
    if(iter != m_file_revisions.end()) {
      return iter->second;
    }
    else {
      return -1;
    }
  }


  void GnoteSyncClient::set_revision(const Note::Ptr & note, int revision)
  {
    m_file_revisions[note->id()] = revision;
    // TODO: Should we write on each of these or no?
    write(m_local_manifest_file_path);
  }


  void GnoteSyncClient::reset()
  {
    if(sharp::file_exists(m_local_manifest_file_path)) {
      sharp::file_delete(m_local_manifest_file_path);
    }
    parse(m_local_manifest_file_path);
  }


  void GnoteSyncClient::associated_server_id(const std::string & server_id)
  {
    if(m_server_id != server_id) {
      m_server_id = server_id;
      write(m_local_manifest_file_path);
    }
  }

}
}


#endif
