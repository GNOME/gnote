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


#ifndef _SYNCHRONIZATION_GNOTESYNCCLIENT_HPP_
#define _SYNCHRONIZATION_GNOTESYNCCLIENT_HPP_


#include <boost/lexical_cast.hpp>

#include "debug.hpp"
#include "gnote.hpp"
#include "gnotesyncclient.hpp"
#include "notemanager.hpp"
#include "sharp/files.hpp"
#include "sharp/xml.hpp"
#include "sharp/xmlwriter.hpp"


namespace gnote {
namespace sync {

  const char * GnoteSyncClient::LOCAL_MANIFEST_FILE_NAME = "manifest.xml";

  GnoteSyncClient::GnoteSyncClient()
  {
    // TODO: Why doesn't OnChanged ever get fired?!
#if 0
    FileSystemWatcher w = new FileSystemWatcher ();
    w.Path = Services.NativeApplication.ConfigurationDirectory;
    w.Filter = localManifestFileName;
    w.Changed += OnChanged;
#endif

    m_local_manifest_file_path = Glib::build_filename(Gnote::conf_dir(), LOCAL_MANIFEST_FILE_NAME);
    parse(m_local_manifest_file_path);

    Gnote::obj().default_note_manager().signal_note_deleted
      .connect(sigc::mem_fun(*this, &GnoteSyncClient::note_deleted_handler));
  }


  void GnoteSyncClient::note_deleted_handler(const Note::Ptr & deletedNote)
  {
    m_deleted_notes[deletedNote->id()] = deletedNote->get_title();
    m_file_revisions.erase(deletedNote->id());

    write(m_local_manifest_file_path);
  }


  void GnoteSyncClient::parse(const std::string & manifest_path)
  {
    // Set defaults before parsing
    m_last_sync_date = sharp::DateTime::now().add_days(-1);
    m_last_sync_rev = -1;

    if(!sharp::file_exists(manifest_path)) {
      m_last_sync_date = sharp::DateTime();
      write(manifest_path);
    }

    xmlDocPtr xml_doc = xmlReadFile(manifest_path.c_str(), "UTF-8", 0);
    if(xml_doc == NULL) {
      DBG_OUT("Invalid XML in %s.  Recreating from scratch.", manifest_path.c_str());
      m_last_sync_date = sharp::DateTime();
      write(manifest_path);
      xml_doc = xmlReadFile(manifest_path.c_str(), "UTF-8", 0);
    }
    try {
      xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);
      sharp::XmlNodeSet note_revisions = sharp::xml_node_xpath_find(root_node, "//note-revisions");
      // TODO: Error checking
      for(sharp::XmlNodeSet::iterator revisionsNode = note_revisions.begin();
          revisionsNode != note_revisions.end(); ++revisionsNode) {
        if((*revisionsNode)->children) {
          int i = 0;
	  do {
	    xmlNodePtr noteNode = &(*revisionsNode)->children[i];
	    std::string guid = sharp::xml_node_get_attribute(noteNode, "guid");
	    int revision = -1;
	    try {
	      revision = boost::lexical_cast<int>(sharp::xml_node_get_attribute(noteNode, "latest-revision"));
	    }
            catch(...) {}

	    m_file_revisions[guid] = revision;
	  }
          while((*revisionsNode)->last != &(*revisionsNode)->children[i++]);
	}
      }

      sharp::XmlNodeSet note_deletions = sharp::xml_node_xpath_find(root_node, "//note-deletions");
      for(sharp::XmlNodeSet::iterator deletionsNode = note_deletions.begin();
          deletionsNode != note_deletions.end(); ++deletionsNode) {
        if((*deletionsNode)->children) {
          int i = 0;
          do {
            xmlNodePtr noteNode = &(*deletionsNode)->children[i];
	    std::string guid = sharp::xml_node_get_attribute(noteNode, "guid");
	    std::string title = sharp::xml_node_get_attribute(noteNode, "title");
	    m_deleted_notes[guid] = title;
	  }
          while((*deletionsNode)->last != &(*deletionsNode)->children[i++]);
        }
      }

      sharp::XmlNodeSet last_sync_rev = sharp::xml_node_xpath_find(root_node, "//last-sync-rev");
      for(sharp::XmlNodeSet::iterator node = last_sync_rev.begin();
          node != last_sync_rev.end(); ++node) {
	try {
	  m_last_sync_rev = boost::lexical_cast<int>(sharp::xml_node_content(*node));
	}
        catch(...) {
	  ERR_OUT("Unparsable last-sync-rev element in %s", manifest_path.c_str());
	}
      }

      sharp::XmlNodeSet server_id = sharp::xml_node_xpath_find(root_node, "//server-id");
      for(sharp::XmlNodeSet::iterator node = server_id.begin();
          node != server_id.end(); ++node) {
        m_server_id = sharp::xml_node_content(*node);
      }

      sharp::XmlNodeSet sync_date = sharp::xml_node_xpath_find(root_node, "//last-sync-date");
      for(sharp::XmlNodeSet::iterator node = sync_date.begin();
          node != sync_date.end(); ++node) {
	try {
	  m_last_sync_date = sharp::DateTime::from_iso8601(sharp::xml_node_content(*node));
	}
        catch(...) {
	  ERR_OUT("Unparsable last-sync-date element in %s", manifest_path.c_str());
	}
      }
      xmlFreeDoc(xml_doc);
    }
    catch(...) {
      xmlFreeDoc(xml_doc);
      throw;
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
