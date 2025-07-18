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
  static SyncServer *create(Glib::RefPtr<Gio::File> && path, Preferences & prefs);
  FileSystemSyncServer(Glib::RefPtr<Gio::File> && path, const Glib::ustring & client_id);
  virtual bool begin_sync_transaction() override;
  virtual bool commit_sync_transaction() override;
  virtual bool cancel_sync_transaction() override;
  virtual std::vector<Glib::ustring> get_all_note_uuids() override;
  NoteUpdatesMap get_note_updates_since(int revision) override;
  virtual void delete_notes(const std::vector<Glib::ustring> & deletedNoteUUIDs) override;
  void upload_notes(const std::vector<NoteBase::Ref> & notes) override;
  virtual int latest_revision() override; // NOTE: Only reliable during a transaction
  virtual SyncLockInfo current_sync_lock() override;
  virtual Glib::ustring id() override;
  virtual bool updates_available_since(int revision) override;
protected:
  virtual void mkdir_p(const Glib::RefPtr<Gio::File> & path);
private:
  void common_ctor();

  Glib::RefPtr<Gio::File> get_revision_dir_path(int rev);
  void cleanup_old_sync(const SyncLockInfo & syncLockInfo);
  void update_lock_file(const SyncLockInfo & syncLockInfo);
  bool is_valid_xml_file(const Glib::RefPtr<Gio::File> & xmlFilePath, xmlDocPtr *xml_doc);
  void lock_timeout();

  std::vector<Glib::ustring> m_updated_notes;
  std::vector<Glib::ustring> m_deleted_notes;

  Glib::ustring m_server_id;

  Glib::RefPtr<Gio::File> m_server_path;
  Glib::ustring m_cache_path;
  Glib::RefPtr<Gio::File> m_lock_path;
  Glib::RefPtr<Gio::File> m_manifest_path;

  int m_new_revision;
  Glib::RefPtr<Gio::File> m_new_revision_path;

  utils::InterruptableTimeout m_lock_timeout;
  SyncLockInfo m_sync_lock;
};

}
}

#endif
