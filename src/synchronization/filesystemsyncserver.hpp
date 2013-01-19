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


#ifndef _SYNCHRONIZATION_FILESYSTEMSYNCSERVER_HPP_
#define _SYNCHRONIZATION_FILESYSTEMSYNCSERVER_HPP_

#include "isyncmanager.hpp"
#include "utils.hpp"
#include "sharp/datetime.hpp"


namespace gnote {
namespace sync {


class FileSystemSyncServer
  : public SyncServer
{
public:
  static SyncServer::Ptr create(const std::string & path);
  virtual bool begin_sync_transaction();
  virtual bool commit_sync_transaction();
  virtual bool cancel_sync_transaction();
  virtual std::list<std::string> get_all_note_uuids();
  virtual std::map<std::string, NoteUpdate> get_note_updates_since(int revision);
  virtual void delete_notes(const std::list<std::string> & deletedNoteUUIDs);
  virtual void upload_notes(const std::list<Note::Ptr> & notes);
  virtual int latest_revision(); // NOTE: Only reliable during a transaction
  virtual SyncLockInfo current_sync_lock();
  virtual std::string id();
  virtual bool updates_available_since(int revision);
private:
  explicit FileSystemSyncServer(const std::string & path);

  std::string get_revision_dir_path(int rev);
  void cleanup_old_sync(const SyncLockInfo & syncLockInfo);
  void update_lock_file(const SyncLockInfo & syncLockInfo);
  bool is_valid_xml_file(const std::string & xmlFilePath);
  void lock_timeout();

  std::list<std::string> m_updated_notes;
  std::list<std::string> m_deleted_notes;

  std::string m_server_id;

  std::string m_server_path;
  std::string m_cache_path;
  std::string m_lock_path;
  std::string m_manifest_path;

  int m_new_revision;
  std::string m_new_revision_path;

  sharp::DateTime m_initial_sync_attempt;
  std::string m_last_sync_lock_hash;
  utils::InterruptableTimeout m_lock_timeout;
  SyncLockInfo m_sync_lock;
};

}
}

#endif
