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


#include <algorithm>
#include <fstream>
#include <stdexcept>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <glibmm/i18n.h>

#include "debug.hpp"
#include "filesystemsyncserver.hpp"
#include "sharp/directory.hpp"
#include "sharp/files.hpp"
#include "sharp/uuid.hpp"
#include "sharp/xml.hpp"
#include "sharp/xmlreader.hpp"
#include "sharp/xmlwriter.hpp"


namespace {

int str_to_int(const std::string & s)
{
  try {
    return boost::lexical_cast<int>(s);
  }
  catch(...) {
    return 0;
  }
}

}


namespace gnote {
namespace sync {

SyncServer::Ptr FileSystemSyncServer::create(const std::string & path)
{
  return SyncServer::Ptr(new FileSystemSyncServer(path));
}


FileSystemSyncServer::FileSystemSyncServer(const std::string & localSyncPath)
  : m_server_path(localSyncPath)
  , m_cache_path(Glib::build_filename(Glib::get_tmp_dir(), Glib::get_user_name(), "gnote"))
{
  if(!sharp::directory_exists(m_server_path)) {
    throw std::invalid_argument(("Directory not found: " + m_server_path).c_str());
  }

  m_lock_path = Glib::build_filename(m_server_path, "lock");
  m_manifest_path = Glib::build_filename(m_server_path, "manifest.xml");

  m_new_revision = latest_revision() + 1;
  m_new_revision_path = get_revision_dir_path(m_new_revision);

  m_lock_timeout.signal_timeout
    .connect(sigc::mem_fun(*this, &FileSystemSyncServer::lock_timeout));
}


void FileSystemSyncServer::upload_notes(const std::list<Note::Ptr> & notes)
{
  if(sharp::directory_exists(m_new_revision_path) == false) {
    sharp::directory_create(m_new_revision_path);
  }
  DBG_OUT("UploadNotes: notes.Count = %d", int(notes.size()));
  for(std::list<Note::Ptr>::const_iterator iter = notes.begin(); iter != notes.end(); ++iter) {
    try {
      std::string serverNotePath = Glib::build_filename(m_new_revision_path, sharp::file_filename((*iter)->file_path()));
      sharp::file_copy((*iter)->file_path(), serverNotePath);
      m_updated_notes.push_back(sharp::file_basename((*iter)->file_path()));
    }
    catch(...) {
      DBG_OUT("Sync: Error uploading note \"%s\"", (*iter)->get_title().c_str());
    }
  }
}


void FileSystemSyncServer::delete_notes(const std::list<std::string> & deletedNoteUUIDs)
{
  m_deleted_notes.insert(m_deleted_notes.end(), deletedNoteUUIDs.begin(), deletedNoteUUIDs.end());
}


std::list<std::string> FileSystemSyncServer::get_all_note_uuids()
{
  std::list<std::string> noteUUIDs;

  if(is_valid_xml_file(m_manifest_path)) {
    // TODO: Permission errors
    xmlDocPtr xml_doc = xmlReadFile(m_manifest_path.c_str(), "UTF-8", 0);
    xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);
    sharp::XmlNodeSet noteIds = sharp::xml_node_xpath_find(root_node, "//note/@id");
    DBG_OUT("get_all_note_uuids has %d notes", int(noteIds.size()));
    for(sharp::XmlNodeSet::iterator iter = noteIds.begin(); iter != noteIds.end(); ++iter) {
      noteUUIDs.push_back(sharp::xml_node_content(*iter));
    }
    xmlFreeDoc(xml_doc);
  }

  return noteUUIDs;
}


bool FileSystemSyncServer::updates_available_since(int revision)
{
  return latest_revision() > revision; // TODO: Mounting, etc?
}


std::map<std::string, NoteUpdate> FileSystemSyncServer::get_note_updates_since(int revision)
{
  std::map<std::string, NoteUpdate> noteUpdates;

  std::string tempPath = Glib::build_filename(m_cache_path, "sync_temp");
  if(!sharp::directory_exists(tempPath)) {
    sharp::directory_create(tempPath);
  }
  else {
    // Empty the temp dir
    try {
      std::list<std::string> files;
      sharp::directory_get_files(tempPath, files);
      for(std::list<std::string>::iterator iter = files.begin(); iter != files.end(); ++iter) {
        sharp::file_delete(*iter);
      }
    }
    catch(...) {}
  }

  if(is_valid_xml_file(m_manifest_path)) {
    xmlDocPtr xml_doc = xmlReadFile(m_manifest_path.c_str(), "UTF-8", 0);
    xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);

    std::string xpath = str(boost::format("//note[@rev > %1%]") % revision);
    sharp::XmlNodeSet noteNodes = sharp::xml_node_xpath_find(root_node, xpath.c_str());
    DBG_OUT("get_note_updates_since xpath returned %d nodes", int(noteNodes.size()));
    for(sharp::XmlNodeSet::iterator iter = noteNodes.begin(); iter != noteNodes.end(); ++iter) {
      std::string note_id = sharp::xml_node_content(sharp::xml_node_xpath_find_single_node(*iter, "@id"));
      int rev = str_to_int(sharp::xml_node_content(sharp::xml_node_xpath_find_single_node(*iter, "@rev")));
      if(noteUpdates.find(note_id) == noteUpdates.end()) {
        // Copy the file from the server to the temp directory
        std::string revDir = get_revision_dir_path(rev);
        std::string serverNotePath = Glib::build_filename(revDir, note_id + ".note");
        std::string noteTempPath = Glib::build_filename(tempPath, note_id + ".note");
        sharp::file_copy(serverNotePath, noteTempPath);

        // Get the title, contents, etc.
        std::string noteTitle;
        std::string noteXml;
        std::ifstream fin(noteTempPath.c_str());
        if(fin.is_open()) {
          do {
            std::string line;
            std::getline(fin, line);
            if(!fin.eof()) {
              noteXml += line + "\n";
            }
          }
          while(!fin.eof());
          fin.close();
        }
        NoteUpdate update(noteXml, noteTitle, note_id, rev);
        noteUpdates.insert(std::make_pair(note_id, update));
      }
    }
    xmlFreeDoc(xml_doc);
  }

  DBG_OUT("get_note_updates_since (%d) returning: %d", revision, int(noteUpdates.size()));
  return noteUpdates;
}


bool FileSystemSyncServer::begin_sync_transaction()
{
  // Lock expiration: If a lock file exists on the server, a client
  // will never be able to synchronize on its first attempt.  The
  // client should record the time elapsed
  if(sharp::file_exists(m_lock_path)) {
    SyncLockInfo currentSyncLock = current_sync_lock();
    if(m_initial_sync_attempt == sharp::DateTime()) {
      DBG_OUT("Sync: Discovered a sync lock file, wait at least %s before trying again.", currentSyncLock.duration.string().c_str());
      // This is our initial attempt to sync and we've detected
      // a sync file, so we're gonna have to wait.
      m_initial_sync_attempt = sharp::DateTime::now();
      m_last_sync_lock_hash = currentSyncLock.hash_string();
      return false;
    }
    else if(m_last_sync_lock_hash != currentSyncLock.hash_string()) {
      DBG_OUT("Sync: Updated sync lock file discovered, wait at least %s before trying again.", currentSyncLock.duration.string().c_str());
      // The sync lock has been updated and is still a valid lock
      m_initial_sync_attempt = sharp::DateTime::now();
      m_last_sync_lock_hash = currentSyncLock.hash_string();
      return false;
    }
    else {
      if(m_last_sync_lock_hash == currentSyncLock.hash_string()) {
        // The sync lock has is the same so check to see if the
        // duration of the lock has expired.  If it hasn't, wait
        // even longer.
        if(sharp::DateTime::now() - currentSyncLock.duration < m_initial_sync_attempt) {
          DBG_OUT("Sync: You haven't waited long enough for the sync file to expire.");
          return false;
        }
      }

      // Cleanup Old Sync Lock!
      cleanup_old_sync(currentSyncLock);
    }
  }

  // Reset the initialSyncAttempt
  m_initial_sync_attempt = sharp::DateTime();
  m_last_sync_lock_hash = "";

  // Create a new lock file so other clients know another client is
  // actively synchronizing right now.
  m_sync_lock.renew_count = 0;
  m_sync_lock.revision = m_new_revision;
  update_lock_file(m_sync_lock);
  // TODO: Verify that the lockTimeout is actually working or figure
  // out some other way to automatically update the lock file.
  // Reset the timer to 20 seconds sooner than the sync lock duration
  m_lock_timeout.reset(m_sync_lock.duration.total_milliseconds() - 20000);

  m_updated_notes.clear();
  m_deleted_notes.clear();

  return true;
}


bool FileSystemSyncServer::commit_sync_transaction()
{
  bool commitSucceeded = false;

  if(m_updated_notes.size() > 0 || m_deleted_notes.size() > 0) {
    // TODO: error-checking, etc
    std::string manifestFilePath = Glib::build_filename(m_new_revision_path, "manifest.xml");
    if(!sharp::directory_exists(m_new_revision_path)) {
      sharp::directory_create(m_new_revision_path);
    }

    std::map<std::string, std::string> notes;
    if(is_valid_xml_file(m_manifest_path) == true) {
      xmlDocPtr xml_doc = xmlReadFile(m_manifest_path.c_str(), "UTF-8", 0);
      xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);
      sharp::XmlNodeSet noteNodes = sharp::xml_node_xpath_find(root_node, "//note");
      for(sharp::XmlNodeSet::iterator iter = noteNodes.begin(); iter != noteNodes.end(); ++iter) {
        std::string note_id = sharp::xml_node_get_attribute(*iter, "id");
        std::string rev = sharp::xml_node_get_attribute(*iter, "rev");
        notes[note_id] = rev;
      }
      xmlFreeDoc(xml_doc);
    }

    // Write out the new manifest file
    sharp::XmlWriter *xml = new sharp::XmlWriter(manifestFilePath);
    try {
      xml->write_start_document();
      xml->write_start_element("", "sync", "");
      xml->write_attribute_string("", "revision", "", boost::lexical_cast<std::string>(m_new_revision));
      xml->write_attribute_string("", "server-id", "", m_server_id);

      for(std::map<std::string, std::string>::iterator iter = notes.begin(); iter != notes.end(); ++iter) {
        // Don't write out deleted notes
        if(std::find(m_deleted_notes.begin(), m_deleted_notes.end(), iter->first) != m_deleted_notes.end()) {
          continue;
        }

        // Skip updated notes, we'll update them in a sec
        if(std::find(m_updated_notes.begin(), m_updated_notes.end(), iter->first) != m_updated_notes.end()) {
          continue;
        }

        xml->write_start_element("", "note", "");
        xml->write_attribute_string("", "id", "", iter->first);
        xml->write_attribute_string("", "rev", "", iter->second);
        xml->write_end_element();
      }

      // Write out all the updated notes
      for(std::list<std::string>::iterator iter = m_updated_notes.begin(); iter != m_updated_notes.end(); ++iter) {
        xml->write_start_element("", "note", "");
        xml->write_attribute_string("", "id", "", *iter);
        xml->write_attribute_string("", "rev", "", boost::lexical_cast<std::string>(m_new_revision));
        xml->write_end_element();
      }

      xml->write_end_element();
      xml->write_end_document();
      xml->close();
      delete xml;
    }
    catch(...) {
      xml->close();
      delete xml;
      throw;
    }


    // Rename original /manifest.xml to /manifest.xml.old
    std::string oldManifestPath = m_manifest_path + ".old";
    if(sharp::file_exists(m_manifest_path) == true) {
      if(sharp::file_exists(oldManifestPath)) {
        sharp::file_delete(oldManifestPath);
      }
      sharp::file_move(m_manifest_path, oldManifestPath);
    }

    // * * * Begin Cleanup Code * * *
    // TODO: Consider completely discarding cleanup code, in favor
    //       of periodic thorough server consistency checks (say every 30 revs).
    //       Even if we do continue providing some cleanup, consistency
    //       checks should be implemented.

    // Copy the /${parent}/${rev}/manifest.xml -> /manifest.xml
    sharp::file_copy(manifestFilePath, m_manifest_path);

    try {
      // Delete /manifest.xml.old
      if(sharp::file_exists(oldManifestPath)) {
        sharp::file_delete(oldManifestPath);
      }

      std::string oldManifestFilePath = Glib::build_filename(get_revision_dir_path(m_new_revision - 1), "manifest.xml");
      if(sharp::file_exists(oldManifestFilePath)) {
        // TODO: Do step #8 as described in http://bugzilla.gnome.org/show_bug.cgi?id=321037#c17
        // Like this?
        std::list<std::string> files;
        sharp::directory_get_files(oldManifestFilePath, files);
        for(std::list<std::string>::iterator iter = files.begin(); iter != files.end(); ++iter) {
          std::string fileGuid = sharp::file_basename(*iter);
          if(std::find(m_deleted_notes.begin(), m_deleted_notes.end(), fileGuid) != m_deleted_notes.end()
             || std::find(m_updated_notes.begin(), m_updated_notes.end(), fileGuid) != m_updated_notes.end()) {
            sharp::file_delete(Glib::build_filename(oldManifestFilePath, *iter));
          }
          // TODO: Need to check *all* revision dirs, not just previous (duh)
          //       Should be a way to cache this from checking earlier.
        }

        // TODO: Leaving old empty dir for now.  Some stuff is probably easier
        //       when you can guarantee the existence of each intermediate directory?
      }
    }
    catch(std::exception & e) {
      ERR_OUT(_("Exception during server cleanup while committing. Server integrity is OK, but "
                "there may be some excess files floating around.  Here's the error: %s\n"), e.what());
    }
    // * * * End Cleanup Code * * *
  }

  m_lock_timeout.cancel();
  sharp::file_delete(m_lock_path);// TODO: Errors?
  commitSucceeded = true;// TODO: When return false?
  return commitSucceeded;
}


bool FileSystemSyncServer::cancel_sync_transaction()
{
  m_lock_timeout.cancel();
  sharp::file_delete(m_lock_path);
  return true;
}


int FileSystemSyncServer::latest_revision()
{
  int latestRev = -1;
  int latestRevDir = -1;
  xmlDocPtr xml_doc = NULL;
  if(is_valid_xml_file(m_manifest_path) == true) {
    xml_doc = xmlReadFile(m_manifest_path.c_str(), "UTF-8", 0);
    xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);
    xmlNodePtr syncNode = sharp::xml_node_xpath_find_single_node(root_node, "//sync");
    std::string latestRevStr = sharp::xml_node_get_attribute(syncNode, "revision");
    if(latestRevStr != "") {
      latestRev = str_to_int(latestRevStr);
    }
  }

  bool foundValidManifest = false;
  while (!foundValidManifest) {
    if(latestRev < 0) {
      // Look for the highest revision parent path
      std::list<std::string> directories;
      sharp::directory_get_directories(m_server_path, directories);
      for(std::list<std::string>::iterator iter = directories.begin(); iter != directories.end(); ++iter) {
        try {
          int currentRevParentDir = str_to_int(sharp::file_filename(*iter));
          if(currentRevParentDir > latestRevDir) {
            latestRevDir = currentRevParentDir;
          }
        }
        catch(...)
        {}
      }

      if(latestRevDir >= 0) {
        directories.clear();
        sharp::directory_get_directories(
          Glib::build_filename(m_server_path, boost::lexical_cast<std::string>(latestRevDir)),
          directories);
        for(std::list<std::string>::iterator iter = directories.begin(); iter != directories.end(); ++iter) {
          try {
            int currentRev = str_to_int(*iter);
            if(currentRev > latestRev) {
              latestRev = currentRev;
            }
          }
          catch(...)
          {}
        }
      }

      if(latestRev >= 0) {
        // Validate that the manifest file inside the revision is valid
        // TODO: Should we create the /manifest.xml file with a valid one?
        std::string revDirPath = get_revision_dir_path(latestRev);
        std::string revManifestPath = Glib::build_filename(revDirPath, "manifest.xml");
        if(is_valid_xml_file(revManifestPath)) {
          foundValidManifest = true;
        }
        else {
          // TODO: Does this really belong here?
          sharp::directory_delete(revDirPath, true);
          // Continue looping
        }
      }
      else {
        foundValidManifest = true;
      }
    }
    else {
      foundValidManifest = true;
    }
  }

  xmlFreeDoc(xml_doc);
  return latestRev;
}


SyncLockInfo FileSystemSyncServer::current_sync_lock()
{
  SyncLockInfo syncLockInfo;

  if(is_valid_xml_file(m_lock_path)) {
    xmlDocPtr xml_doc = xmlReadFile(m_lock_path.c_str(), "UTF-8", 0);
    xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);

    xmlNodePtr node = sharp::xml_node_xpath_find_single_node(root_node, "//transaction-id/text ()");
    if(node != NULL) {
      std::string transaction_id_txt = sharp::xml_node_content(node);
      syncLockInfo.transaction_id = transaction_id_txt;
    }

    node = sharp::xml_node_xpath_find_single_node(root_node, "//client-id/text ()");
    if(node != NULL) {
      std::string client_id_txt = sharp::xml_node_content(node);
      syncLockInfo.client_id = client_id_txt;
    }

    node = sharp::xml_node_xpath_find_single_node(root_node, "renew-count/text ()");
    if(node != NULL) {
      std::string renew_txt = sharp::xml_node_content(node);
      syncLockInfo.renew_count = str_to_int(renew_txt);
    }

    node = sharp::xml_node_xpath_find_single_node(root_node, "lock-expiration-duration/text ()");
    if(node != NULL) {
      std::string span_txt = sharp::xml_node_content(node);
      syncLockInfo.duration = sharp::TimeSpan::parse(span_txt);
    }

    node = sharp::xml_node_xpath_find_single_node(root_node, "revision/text ()");
    if(node != NULL) {
      std::string revision_txt = sharp::xml_node_content(node);
      syncLockInfo.revision = str_to_int(revision_txt);
    }

    xmlFreeDoc(xml_doc);
  }

  return syncLockInfo;
}


std::string FileSystemSyncServer::id()
{
  m_server_id = "";

  // Attempt to read from manifest file first
  if(is_valid_xml_file(m_manifest_path)) {
    sharp::XmlReader reader(m_manifest_path);
    if(reader.read()) {
      if(reader.get_node_type() == XML_READER_TYPE_ELEMENT && reader.get_name() == "sync") {
	m_server_id = reader.get_attribute("server-id");
      }
    }
  }

  // Generate a new ID if there isn't already one
  if(m_server_id == "") {
    m_server_id = sharp::uuid().string();
  }

  return m_server_id;
}


std::string FileSystemSyncServer::get_revision_dir_path(int rev)
{
  return Glib::build_filename(m_server_path,
                              boost::lexical_cast<std::string>(rev/100),
                              boost::lexical_cast<std::string>(rev));
}


void FileSystemSyncServer::update_lock_file(const SyncLockInfo & syncLockInfo)
{
  sharp::XmlWriter xml(m_lock_path);
  try {
    xml.write_start_document();
    xml.write_start_element("", "lock", "");

    xml.write_start_element("", "transaction-id", "");
    xml.write_string(syncLockInfo.transaction_id);
    xml.write_end_element();

    xml.write_start_element("", "client-id", "");
    xml.write_string(syncLockInfo.client_id);
    xml.write_end_element();

    xml.write_start_element("", "renew-count", "");
    xml.write_string(boost::lexical_cast<std::string>(syncLockInfo.renew_count));
    xml.write_end_element();

    xml.write_start_element("", "lock-expiration-duration", "");
    xml.write_string(syncLockInfo.duration.string());
    xml.write_end_element();

    xml.write_start_element("", "revision", "");
    xml.write_string(boost::lexical_cast<std::string>(syncLockInfo.revision));
    xml.write_end_element();

    xml.write_end_element();
    xml.write_end_document();

    xml.close();
  }
  catch(...) {
    xml.close();
    throw;
  }
}


void FileSystemSyncServer::cleanup_old_sync(const SyncLockInfo &)
{
  DBG_OUT("Sync: Cleaning up a previous failed sync transaction");
  int rev = latest_revision();
  if(rev >= 0 && !is_valid_xml_file(m_manifest_path)) {
    // Time to discover the latest valid revision
    // If no manifest.xml file exists, that means we've got to
    // figure out if there are any previous revisions with valid
    // manifest.xml files around.
    for (; rev >= 0; rev--) {
      std::string revParentPath = get_revision_dir_path(rev);
      std::string manPath = Glib::build_filename(revParentPath, "manifest.xml");

      if(is_valid_xml_file(manPath) == false) {
        continue;
      }

      // Restore a valid manifest path
      sharp::file_copy(manPath, m_manifest_path);
      break;
    }
  }

  // Delete the old lock file
  DBG_OUT("Sync: Deleting expired lockfile");
  try {
    sharp::file_delete(m_lock_path);
  }
  catch(std::exception & e) {
    ERR_OUT(_("Error deleting the old synchronization lock \"%s\": %s"), m_lock_path.c_str(), e.what());
  }
}


bool FileSystemSyncServer::is_valid_xml_file(const std::string & xmlFilePath)
{
  // Check that file exists
  if(!sharp::file_exists(xmlFilePath)) {
    return false;
  }

  // TODO: Permissions errors
  // Attempt to load the file and parse it as XML
  xmlDocPtr xml_doc = xmlReadFile(xmlFilePath.c_str(), "UTF-8", 0);
  if(!xml_doc) {
    return false;
  }
  xmlFreeDoc(xml_doc);
  return true;
}


void FileSystemSyncServer::lock_timeout()
{
  m_sync_lock.renew_count++;
  update_lock_file(m_sync_lock);
  // Reset the timer to 20 seconds sooner than the sync lock duration
  m_lock_timeout.reset(m_sync_lock.duration.total_milliseconds() - 20000);
}


}
}
