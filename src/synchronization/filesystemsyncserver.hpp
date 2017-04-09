/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017 Aurimas Cernius
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

#include "base/macros.hpp"
#include "isyncmanager.hpp"
#include "utils.hpp"
#include "sharp/datetime.hpp"


namespace gnote {
namespace sync {


class FileSystemSyncServer
  : public SyncServer
{
public:
  static SyncServer::Ptr create(const Glib::ustring & path);
  static SyncServer::Ptr create(const Glib::ustring & path, const Glib::ustring & client_id);
  virtual bool begin_sync_transaction() override;
  virtual bool commit_sync_transaction() override;
  virtual bool cancel_sync_transaction() override;
  virtual std::list<Glib::ustring> get_all_note_uuids() override;
  virtual std::map<Glib::ustring, NoteUpdate> get_note_updates_since(int revision) override;
  virtual void delete_notes(const std::list<Glib::ustring> & deletedNoteUUIDs) override;
  virtual void upload_notes(const std::list<Note::Ptr> & notes) override;
  virtual int latest_revision() override; // NOTE: Only reliable during a transaction
  virtual SyncLockInfo current_sync_lock() override;
  virtual Glib::ustring id() override;
  virtual bool updates_available_since(int revision) override;
private:
  explicit FileSystemSyncServer(const Glib::ustring & path);
  FileSystemSyncServer(const Glib::ustring & path, const Glib::ustring & client_id);
  void common_ctor();

  Glib::ustring get_revision_dir_path(int rev);
  void cleanup_old_sync(const SyncLockInfo & syncLockInfo);
  void update_lock_file(const SyncLockInfo & syncLockInfo);
  bool is_valid_xml_file(const Glib::ustring & xmlFilePath);
  void lock_timeout();

  std::list<Glib::ustring> m_updated_notes;
  std::list<Glib::ustring> m_deleted_notes;

  Glib::ustring m_server_id;

  Glib::ustring m_server_path;
  Glib::ustring m_cache_path;
  Glib::ustring m_lock_path;
  Glib::ustring m_manifest_path;

  int m_new_revision;
  Glib::ustring m_new_revision_path;

  sharp::DateTime m_initial_sync_attempt;
  Glib::ustring m_last_sync_lock_hash;
  utils::InterruptableTimeout m_lock_timeout;
  SyncLockInfo m_sync_lock;
};

}
}

#endif
