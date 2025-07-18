/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017-2023,2025 Aurimas Cernius
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
#include <stdexcept>

#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>

#include "debug.hpp"
#include "filesystemsyncserver.hpp"
#include "gvfstransfer.hpp"
#include "preferences.hpp"
#include "sharp/directory.hpp"
#include "sharp/files.hpp"
#include "sharp/uuid.hpp"
#include "sharp/xml.hpp"
#include "sharp/xmlreader.hpp"
#include "sharp/xmlwriter.hpp"


namespace {

int str_to_int(const Glib::ustring & s)
{
  try {
    return STRING_TO_INT(s);
  }
  catch(...) {
    return 0;
  }
}

struct NoteUpload
  : gnote::sync::FileTransfer
{
  NoteUpload(const Glib::RefPtr<Gio::File> &src, const Glib::RefPtr<Gio::File> &dest, Glib::ustring &&result_path)
    : FileTransfer(src, dest)
    , result_path(std::move(result_path))
  {}

  const Glib::ustring result_path;
};

struct NoteDownload
  : gnote::sync::FileTransfer
{
  explicit NoteDownload(const Glib::ustring &note_id)
    : FileTransfer({}, {})
    , revision(-1)
    , note_id(note_id)
  {}

  NoteDownload(const Glib::RefPtr<Gio::File> &src, const Glib::RefPtr<Gio::File> &dest, int revision, Glib::ustring &&note_id, Glib::ustring &&result_path)
    : FileTransfer(src, dest)
    , revision(revision)
    , note_id(std::move(note_id))
    , result_path(std::move(result_path))
  {}

  const int revision;
  const Glib::ustring note_id;
  const Glib::ustring result_path;
};

class NoteDownloadHash
{
public:
  std::size_t operator()(const NoteDownload & d) const noexcept
  {
    return m_hash(d.note_id);
  }
private:
  gnote::Hash<Glib::ustring> m_hash;
};

class NoteDownloadIdEqual
{
public:
  bool operator()(const NoteDownload &x, const NoteDownload &y) const noexcept
  {
    return x.note_id == y.note_id;
  }
};

typedef std::unordered_set<NoteDownload, NoteDownloadHash, NoteDownloadIdEqual> NoteDownloadSet;

}


namespace gnote {
namespace sync {

SyncServer *FileSystemSyncServer::create(Glib::RefPtr<Gio::File> && path, Preferences & prefs)
{
  return new FileSystemSyncServer(std::move(path), prefs.sync_client_id());
}


FileSystemSyncServer::FileSystemSyncServer(Glib::RefPtr<Gio::File> && localSyncPath, const Glib::ustring & client_id)
  : m_server_path(std::move(localSyncPath))
  , m_cache_path(Glib::build_filename(Glib::get_tmp_dir(), Glib::get_user_name(), "gnote"))
  , m_sync_lock(client_id)
{
  common_ctor();
}


void FileSystemSyncServer::common_ctor()
{
  if(!sharp::directory_exists(m_server_path)) {
    throw std::invalid_argument(("Directory not found: " + m_server_path->get_uri()).c_str());
  }

  m_lock_path = m_server_path->get_child("lock");
  m_manifest_path = m_server_path->get_child("manifest.xml");

  m_new_revision = latest_revision() + 1;
  m_new_revision_path = get_revision_dir_path(m_new_revision);

  m_lock_timeout.signal_timeout
    .connect(sigc::mem_fun(*this, &FileSystemSyncServer::lock_timeout));
}


void FileSystemSyncServer::mkdir_p(const Glib::RefPtr<Gio::File> & path)
{
  if(sharp::directory_exists(path) == false) {
    sharp::directory_create(path);
  }
}


void FileSystemSyncServer::upload_notes(const std::vector<NoteBase::Ref> & notes)
{
  mkdir_p(m_new_revision_path);
  DBG_OUT("UploadNotes: notes.Count = %d", int(notes.size()));
  m_updated_notes.reserve(notes.size());
  std::vector<NoteUpload> uploads;
  for(NoteBase &iter : notes) {
    auto file_path = iter.file_path();
    auto local_note = Gio::File::create_for_path(file_path);
    auto server_note = m_new_revision_path->get_child(sharp::file_filename(file_path));
    uploads.emplace_back(local_note, server_note, sharp::file_basename(file_path));
  }

  GvfsTransfer file_transfer(uploads);
  const auto failures = file_transfer.transfer();
  if(failures > 0) {
    throw GnoteSyncException(Glib::ustring::compose(ngettext("Failed to upload %1 note", "Failed to upload %1 notes", failures), failures));
  }
  else {
    for(auto &upload : uploads) {
      m_updated_notes.emplace_back(std::move(upload.result_path));
    }
  }
}


void FileSystemSyncServer::delete_notes(const std::vector<Glib::ustring> & deletedNoteUUIDs)
{
  m_deleted_notes.insert(m_deleted_notes.end(), deletedNoteUUIDs.begin(), deletedNoteUUIDs.end());
}


std::vector<Glib::ustring> FileSystemSyncServer::get_all_note_uuids()
{
  std::vector<Glib::ustring> noteUUIDs;

  xmlDocPtr xml_doc = NULL;
  if(is_valid_xml_file(m_manifest_path, &xml_doc)) {
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


SyncServer::NoteUpdatesMap FileSystemSyncServer::get_note_updates_since(int revision)
{
  SyncServer::NoteUpdatesMap noteUpdates;

  Glib::ustring temp_path = Glib::build_filename(m_cache_path, "sync_temp");
  if(!sharp::directory_exists(temp_path)) {
    sharp::directory_create(temp_path);
  }
  else {
    // Empty the temp dir
    try {
      std::vector<Glib::ustring> files = sharp::directory_get_files(temp_path);
      for(auto & iter : files) {
        sharp::file_delete(iter);
      }
    }
    catch(...) {}
  }

  NoteDownloadSet downloads;
  xmlDocPtr xml_doc = NULL;
  if(is_valid_xml_file(m_manifest_path, &xml_doc)) {
    xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);

    Glib::ustring xpath = Glib::ustring::compose("//note[@rev > %1]", revision);
    sharp::XmlNodeSet noteNodes = sharp::xml_node_xpath_find(root_node, xpath.c_str());
    DBG_OUT("get_note_updates_since xpath returned %d nodes", int(noteNodes.size()));
    if(noteNodes.size() > 0) {
      for(auto & node : noteNodes) {
        Glib::ustring note_id = sharp::xml_node_content(sharp::xml_node_xpath_find_single_node(node, "@id"));
        int rev = str_to_int(sharp::xml_node_content(sharp::xml_node_xpath_find_single_node(node, "@rev")));
        if(downloads.find(NoteDownload(note_id)) == downloads.end()) {
          auto rev_dir = get_revision_dir_path(rev);
          auto server_note = rev_dir->get_child(note_id + ".note");
          // Copy the file from the server to the temp directory
          Glib::ustring note_temp_path = Glib::build_filename(temp_path, note_id + ".note");
          auto dest = Gio::File::create_for_path(note_temp_path);
          downloads.emplace(server_note, dest, rev, std::move(note_id), std::move(note_temp_path));
        }
      }
    }

    xmlFreeDoc(xml_doc);
  }

  GvfsTransfer file_transfers(downloads);
  const auto failures = file_transfers.transfer();

  if(failures > 0) {
    throw GnoteSyncException(Glib::ustring::compose(ngettext("Failed to download %1 note update", "Failed to download %1 note updates", failures), failures));
  }

  for(const auto &downloaded : downloads) {
    Glib::ustring note_xml = sharp::file_read_all_text(downloaded.result_path);
    NoteUpdate update(note_xml, Glib::ustring(), downloaded.note_id, downloaded.revision);
    noteUpdates.insert(std::make_pair(downloaded.note_id, update));
  }

  DBG_OUT("get_note_updates_since (%d) returning: %d", revision, int(noteUpdates.size()));
  return noteUpdates;
}


bool FileSystemSyncServer::begin_sync_transaction()
{
  // Lock expiration: If a lock file exists on the server, a client
  // will never be able to synchronize on its first attempt.  The
  // client should record the time elapsed
  if(m_lock_path->query_exists()) {
    SyncLockInfo currentSyncLock = current_sync_lock();
    auto file_info = m_lock_path->query_info();
    auto lock_expires = file_info->get_modification_date_time().to_utc().add(currentSyncLock.duration);
    auto now = Glib::DateTime::create_now_utc();
    if(now < lock_expires) {
      DBG_OUT("Sync: Discovered a sync lock file, wait at least %s before trying again.", sharp::time_span_string(currentSyncLock.duration).c_str());
      return false;
    }
  }

  // Create a new lock file so other clients know another client is
  // actively synchronizing right now.
  m_sync_lock.renew_count = 0;
  m_sync_lock.revision = m_new_revision;
  update_lock_file(m_sync_lock);
  // TODO: Verify that the lockTimeout is actually working or figure
  // out some other way to automatically update the lock file.
  // Reset the timer to 20 seconds sooner than the sync lock duration
  m_lock_timeout.reset(sharp::time_span_total_milliseconds(m_sync_lock.duration) - 20000);

  m_updated_notes.clear();
  m_deleted_notes.clear();

  return true;
}


bool FileSystemSyncServer::commit_sync_transaction()
{
  bool commitSucceeded = false;

  if(m_updated_notes.size() > 0 || m_deleted_notes.size() > 0) {
    // TODO: error-checking, etc
    auto manifest_file = m_new_revision_path->get_child("manifest.xml");
    if(!sharp::directory_exists(m_new_revision_path)) {
      sharp::directory_create(m_new_revision_path);
    }

    std::map<Glib::ustring, Glib::ustring> notes;
    xmlDocPtr xml_doc = NULL;
    if(is_valid_xml_file(m_manifest_path, &xml_doc) == true) {
      xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);
      sharp::XmlNodeSet noteNodes = sharp::xml_node_xpath_find(root_node, "//note");
      for(sharp::XmlNodeSet::iterator iter = noteNodes.begin(); iter != noteNodes.end(); ++iter) {
        Glib::ustring note_id = sharp::xml_node_get_attribute(*iter, "id");
        Glib::ustring rev = sharp::xml_node_get_attribute(*iter, "rev");
        notes[note_id] = rev;
      }
      xmlFreeDoc(xml_doc);
    }

    // Write out the new manifest file
    sharp::XmlWriter *xml = new sharp::XmlWriter();
    try {
      xml->write_start_document();
      xml->write_start_element("", "sync", "");
      xml->write_attribute_string("", "revision", "", TO_STRING(m_new_revision));
      xml->write_attribute_string("", "server-id", "", m_server_id);

      for(std::map<Glib::ustring, Glib::ustring>::iterator iter = notes.begin(); iter != notes.end(); ++iter) {
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
      for(auto & note : m_updated_notes) {
        xml->write_start_element("", "note", "");
        xml->write_attribute_string("", "id", "", note);
        xml->write_attribute_string("", "rev", "", TO_STRING(m_new_revision));
        xml->write_end_element();
      }

      xml->write_end_element();
      xml->write_end_document();
      xml->close();
      Glib::ustring xml_content = xml->to_string();
      delete xml;
      xml = nullptr;
      if(manifest_file->query_exists()) {
        manifest_file->remove();
      }
      auto stream = manifest_file->create_file();
      gsize written;
      stream->write_all(xml_content, written);
      stream->close();
    }
    catch(...) {
      if(xml) {
        xml->close();
        delete xml;
      }
      throw;
    }


    // Rename original /manifest.xml to /manifest.xml.old
    Glib::RefPtr<Gio::File> oldManifestPath = Gio::File::create_for_uri(m_manifest_path->get_uri() + ".old");
    if(oldManifestPath->query_exists()) {
      oldManifestPath->remove();
    }
    if(m_manifest_path->query_exists() == true) {
      m_manifest_path->move(oldManifestPath);
    }

    // * * * Begin Cleanup Code * * *
    // TODO: Consider completely discarding cleanup code, in favor
    //       of periodic thorough server consistency checks (say every 30 revs).
    //       Even if we do continue providing some cleanup, consistency
    //       checks should be implemented.

    // Copy the /${parent}/${rev}/manifest.xml -> /manifest.xml
    manifest_file->copy(m_manifest_path);

    try {
      // Delete /manifest.xml.old
      if(oldManifestPath->query_exists()) {
        oldManifestPath->remove();
      }

      auto old_manifest_file = get_revision_dir_path(m_new_revision - 1)->get_child("manifest.xml");
      if(old_manifest_file->query_exists()) {
        // TODO: Do step #8 as described in http://bugzilla.gnome.org/show_bug.cgi?id=321037#c17
        // Like this?
        std::vector<Glib::RefPtr<Gio::File>> files = sharp::directory_get_files(old_manifest_file->get_parent());
        for(auto file : files) {
          Glib::ustring fileGuid = file->get_basename();
          if(std::find(m_deleted_notes.begin(), m_deleted_notes.end(), fileGuid) != m_deleted_notes.end()
             || std::find(m_updated_notes.begin(), m_updated_notes.end(), fileGuid) != m_updated_notes.end()) {
            file->remove();
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
  m_lock_path->remove();// TODO: Errors?
  commitSucceeded = true;// TODO: When return false?
  return commitSucceeded;
}


bool FileSystemSyncServer::cancel_sync_transaction()
{
  m_lock_timeout.cancel();
  m_lock_path->remove();
  return true;
}


int FileSystemSyncServer::latest_revision()
{
  int latestRev = -1;
  int latestRevDir = -1;
  xmlDocPtr xml_doc = NULL;
  if(is_valid_xml_file(m_manifest_path, &xml_doc) == true) {
    xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);
    xmlNodePtr syncNode = sharp::xml_node_xpath_find_single_node(root_node, "//sync");
    Glib::ustring latestRevStr = sharp::xml_node_get_attribute(syncNode, "revision");
    if(latestRevStr != "") {
      latestRev = str_to_int(latestRevStr);
    }
  }

  bool foundValidManifest = false;
  while (!foundValidManifest) {
    if(latestRev < 0) {
      // Look for the highest revision parent path
      std::vector<Glib::RefPtr<Gio::File>> directories = sharp::directory_get_directories(m_server_path);
      for(auto & iter : directories) {
        try {
          int currentRevParentDir = str_to_int(sharp::file_filename(iter));
          if(currentRevParentDir > latestRevDir) {
            latestRevDir = currentRevParentDir;
          }
        }
        catch(...)
        {}
      }

      if(latestRevDir >= 0) {
        directories = sharp::directory_get_directories(m_server_path->get_child(TO_STRING(latestRevDir)));
        for(auto & iter : directories) {
          try {
            int currentRev = str_to_int(iter->get_basename());
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
        auto revDirPath = get_revision_dir_path(latestRev);
        auto revManifestPath = revDirPath->get_child("manifest.xml");
        if(is_valid_xml_file(revManifestPath, NULL)) {
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
  SyncLockInfo syncLockInfo(m_sync_lock.client_id);

  xmlDocPtr xml_doc = NULL;
  if(is_valid_xml_file(m_lock_path, &xml_doc)) {
    xmlNodePtr root_node = xmlDocGetRootElement(xml_doc);

    xmlNodePtr node = sharp::xml_node_xpath_find_single_node(root_node, "//transaction-id/text ()");
    if(node != NULL) {
      Glib::ustring transaction_id_txt = sharp::xml_node_content(node);
      syncLockInfo.transaction_id = transaction_id_txt;
    }

    node = sharp::xml_node_xpath_find_single_node(root_node, "//client-id/text ()");
    if(node != NULL) {
      Glib::ustring client_id_txt = sharp::xml_node_content(node);
      syncLockInfo.client_id = client_id_txt;
    }

    node = sharp::xml_node_xpath_find_single_node(root_node, "renew-count/text ()");
    if(node != NULL) {
      Glib::ustring renew_txt = sharp::xml_node_content(node);
      syncLockInfo.renew_count = str_to_int(renew_txt);
    }

    node = sharp::xml_node_xpath_find_single_node(root_node, "lock-expiration-duration/text ()");
    if(node != NULL) {
      Glib::ustring span_txt = sharp::xml_node_content(node);
      syncLockInfo.duration = sharp::time_span_parse(span_txt);
    }

    node = sharp::xml_node_xpath_find_single_node(root_node, "revision/text ()");
    if(node != NULL) {
      Glib::ustring revision_txt = sharp::xml_node_content(node);
      syncLockInfo.revision = str_to_int(revision_txt);
    }

    xmlFreeDoc(xml_doc);
  }

  return syncLockInfo;
}


Glib::ustring FileSystemSyncServer::id()
{
  m_server_id = "";

  // Attempt to read from manifest file first
  xmlDocPtr xml_doc = NULL;
  if(is_valid_xml_file(m_manifest_path, &xml_doc)) {
    sharp::XmlReader reader(xml_doc);
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


Glib::RefPtr<Gio::File> FileSystemSyncServer::get_revision_dir_path(int rev)
{
  return Gio::File::create_for_uri(Glib::build_filename(m_server_path->get_uri(), TO_STRING(rev/100), TO_STRING(rev)));
}


void FileSystemSyncServer::update_lock_file(const SyncLockInfo & syncLockInfo)
{
  sharp::XmlWriter xml;
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
    xml.write_string(TO_STRING(syncLockInfo.renew_count));
    xml.write_end_element();

    xml.write_start_element("", "lock-expiration-duration", "");
    xml.write_string(sharp::time_span_string(syncLockInfo.duration));
    xml.write_end_element();

    xml.write_start_element("", "revision", "");
    xml.write_string(TO_STRING(syncLockInfo.revision));
    xml.write_end_element();

    xml.write_end_element();
    xml.write_end_document();

    xml.close();
    auto stream = m_lock_path->replace();
    gsize written;
    stream->write_all(xml.to_string(), written);
    stream->close();
  }
  catch(Glib::Error & e) {
    xml.close();
    ERR_OUT(_("Error updating lock: %s"), e.what());
    throw;
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
  if(rev >= 0 && !is_valid_xml_file(m_manifest_path, NULL)) {
    // Time to discover the latest valid revision
    // If no manifest.xml file exists, that means we've got to
    // figure out if there are any previous revisions with valid
    // manifest.xml files around.
    for (; rev >= 0; rev--) {
      auto revParentPath = get_revision_dir_path(rev);
      auto manifest = revParentPath->get_child("manifest.xml");

      if(is_valid_xml_file(manifest, NULL) == false) {
        continue;
      }

      // Restore a valid manifest path
      manifest->copy(m_manifest_path);
      break;
    }
  }

  // Delete the old lock file
  DBG_OUT("Sync: Deleting expired lockfile");
  try {
    m_lock_path->remove();
  }
  catch(std::exception & e) {
    ERR_OUT(_("Error deleting the old synchronization lock \"%s\": %s"), m_lock_path->get_uri().c_str(), e.what());
  }
}


bool FileSystemSyncServer::is_valid_xml_file(const Glib::RefPtr<Gio::File> & xmlFile, xmlDocPtr *xml_doc)
{
  // Check that file exists
  if(!xmlFile->query_exists()) {
    return false;
  }

  // TODO: Permissions errors
  // Attempt to load the file and parse it as XML
  auto stream = xmlFile->read();
  std::ostringstream os;
  int buf_size = 4 * 1024;
  char buffer[buf_size];
  gssize read = 0;
  do {
    read = stream->read(buffer, buf_size);
    os.write(buffer, read);
  }
  while(read == buf_size);
  stream->close();
  auto xml_string = os.str();
  xmlDocPtr xml = xmlReadMemory(xml_string.c_str(), xml_string.size(), xmlFile->get_uri().c_str(), "UTF-8", 0);
  if(!xml) {
    return false;
  }
  if(xml_doc == NULL) {
    xmlFreeDoc(xml);
  }
  else {
    *xml_doc = xml;
  }
  return true;
}


void FileSystemSyncServer::lock_timeout()
{
  m_sync_lock.renew_count++;
  update_lock_file(m_sync_lock);
  // Reset the timer to 20 seconds sooner than the sync lock duration
  m_lock_timeout.reset(sharp::time_span_total_milliseconds(m_sync_lock.duration) - 20000);
}


}
}
