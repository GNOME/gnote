/*
 * gnote
 *
 * Copyright (C) 2012-2014,2017,2019-2020,2023,2025 Aurimas Cernius
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


#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>

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

  SyncClient *GnoteSyncClient::create(NoteManagerBase & manager)
  {
    GnoteSyncClient *ptr = new GnoteSyncClient;
    ptr->init(manager);
    return ptr;
  }

  GnoteSyncClient::GnoteSyncClient()
    : m_synchronizing(false)
  {
  }


  void GnoteSyncClient::init(NoteManagerBase & manager)
  {
    m_local_manifest_file_path = Glib::build_filename(IGnote::conf_dir(), LOCAL_MANIFEST_FILE_NAME);
    Glib::RefPtr<Gio::File> manifest = Gio::File::create_for_path(m_local_manifest_file_path);
    parse(m_local_manifest_file_path);

    manager.signal_note_deleted
      .connect(sigc::mem_fun(*this, &GnoteSyncClient::note_deleted_handler));
  }


  void GnoteSyncClient::note_deleted_handler(NoteBase & deleted_note)
  {
    if(m_synchronizing) {
      return;
    }
    m_deleted_notes[deleted_note.id()] = deleted_note.get_title();
    m_file_revisions.erase(deleted_note.id());

    write(m_local_manifest_file_path);
  }


  void GnoteSyncClient::read_updated_note_atts(sharp::XmlReader & reader)
  {
    Glib::ustring guid, rev;
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
      revision = STRING_TO_INT(rev);
    }
    catch(...) {}
    if(guid != "") {
      m_file_revisions[guid] = revision;
    }
  }


  void GnoteSyncClient::read_deleted_note_atts(sharp::XmlReader & reader)
  {
    Glib::ustring guid, title;
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
	  (this->*read_note_atts)(reader);
	}
      }
    }
  }


  void GnoteSyncClient::parse(const Glib::ustring & manifest_path)
  {
    // Set defaults before parsing
    m_last_sync_date = Glib::DateTime::create_now_local().add_days(-1);
    m_last_sync_rev = -1;
    m_file_revisions.clear();
    m_deleted_notes.clear();
    m_server_id = "";

    if(!sharp::file_exists(manifest_path)) {
      m_last_sync_date = Glib::DateTime::create_now_utc();
      write(manifest_path);
      return;
    }

    sharp::XmlReader reader(manifest_path);
    while(reader.read()) {
      if(reader.get_node_type() == XML_READER_TYPE_ELEMENT) {
	if(reader.get_name() == "last-sync-date") {
	  Glib::ustring value = reader.read_string();
	  try {
	    m_last_sync_date = sharp::date_time_from_iso8601(value);
	  }
	  catch(...) {
            /* TRANSLATORS: %s is file */
	    ERR_OUT(_("Unparsable last-sync-date element in %s"), manifest_path.c_str());
	  }
	}
	else if(reader.get_name() == "last-sync-rev") {
	  Glib::ustring value = reader.read_string();
	  try {
	    m_last_sync_rev = STRING_TO_INT(value);
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


  void GnoteSyncClient::write(const Glib::ustring & manifest_path)
  {
    if(!m_last_sync_date) {
      m_last_sync_date = Glib::DateTime::create_now_utc();
    }

    sharp::XmlWriter xml(manifest_path);

    try {
      xml.write_start_document();
      xml.write_start_element("", "manifest", "http://beatniksoftware.com/tomboy");

      xml.write_start_element("", "last-sync-date", "");
      xml.write_string(sharp::date_time_to_iso8601(m_last_sync_date));
      xml.write_end_element();

      xml.write_start_element("", "last-sync-rev", "");
      xml.write_string(TO_STRING(m_last_sync_rev));
      xml.write_end_element();

      xml.write_start_element("", "server-id", "");
      xml.write_string(m_server_id);
      xml.write_end_element();

      xml.write_start_element("", "note-revisions", "");

      for(auto & noteGuid : m_file_revisions) {
	xml.write_start_element("", "note", "");
	xml.write_attribute_string("", "guid", "", noteGuid.first);
	xml.write_attribute_string("", "latest-revision", "", TO_STRING(noteGuid.second));
	xml.write_end_element();
      }

      xml.write_end_element(); // </note-revisons>

      xml.write_start_element("", "note-deletions", "");

      for(auto & noteGuid : m_deleted_notes) {
	xml.write_start_element("", "note", "");
	xml.write_attribute_string("", "guid", "", noteGuid.first);
	xml.write_attribute_string("", "title", "", noteGuid.second);
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


  void GnoteSyncClient::last_sync_date(const Glib::DateTime & date)
  {
    m_last_sync_date = date;
  }


  void GnoteSyncClient::last_synchronized_revision(int revision)
  {
    m_last_sync_rev = revision;
  }


  int GnoteSyncClient::get_revision(const NoteBase &note) const
  {
    Glib::ustring note_guid = note.id();
    auto iter = m_file_revisions.find(note_guid);
    if(iter != m_file_revisions.end()) {
      return iter->second;
    }
    else {
      return -1;
    }
  }


  void GnoteSyncClient::set_revision(const NoteBase & note, int revision)
  {
    m_file_revisions[note.id()] = revision;
  }


  void GnoteSyncClient::reset()
  {
    if(sharp::file_exists(m_local_manifest_file_path)) {
      sharp::file_delete(m_local_manifest_file_path);
    }
    parse(m_local_manifest_file_path);
  }


  void GnoteSyncClient::associated_server_id(const Glib::ustring & server_id)
  {
    if(m_server_id != server_id) {
      m_server_id = server_id;
      write(m_local_manifest_file_path);
    }
  }


  void GnoteSyncClient::begin_synchronization()
  {
    m_synchronizing = true;
  }


  void GnoteSyncClient::end_synchronization()
  {
    if(m_synchronizing) {
      m_synchronizing = false;
      m_deleted_notes.clear();
      write(m_local_manifest_file_path);
    }
  }


  void GnoteSyncClient::cancel_synchronization()
  {
    m_synchronizing = false;
  }

}
}


#endif
