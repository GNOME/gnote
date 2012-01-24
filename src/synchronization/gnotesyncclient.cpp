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


#include "gnotesyncclient.hpp"
#include "sharp/files.hpp"


namespace gnote {
namespace sync {

  const char * GnoteSyncClient::LOCAL_MANIFEST_FILE_NAME = "manifest.xml";

  GnoteSyncClient::GnoteSyncClient()
  {
  }


  void GnoteSyncClient::last_sync_date(const sharp::DateTime & date)
  {
    m_last_sync_date = date;
    // If we just did a sync, we should be able to forget older deleted notes
    m_deleted_notes.clear();
    //Write(localManifestFilePath);  TODO
  }


  void GnoteSyncClient::last_synchronized_revision(int revision)
  {
    m_last_sync_rev = revision;
    //Write(localManifestFilePath);  TODO
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
    //Write(localManifestFilePath);  TODO
  }


  void GnoteSyncClient::reset()
  {
    if(sharp::file_exists(m_local_manifest_file_path)) {
      sharp::file_delete(m_local_manifest_file_path);
    }
    //Parse(localManifestFilePath);  TODO
  }


  void GnoteSyncClient::associated_server_id(const std::string & server_id)
  {
    if(m_server_id != server_id) {
      m_server_id = server_id;
      //Write(localManifestFilePath);  TODO
    }
  }

}
}


#endif
