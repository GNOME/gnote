/*
 * gnote
 *
 * Copyright (C) 2012-2014,2017,2019-2021,2023,2025 Aurimas Cernius
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


#ifndef _SYNCHRONIZATION_ISYNCMANAGER_HPP_
#define _SYNCHRONIZATION_ISYNCMANAGER_HPP_

#include "note.hpp"
#include "syncui.hpp"
#include "syncutils.hpp"

namespace gnote {
namespace sync {

class SyncLockInfo
{
public:
  Glib::ustring client_id;
  Glib::ustring transaction_id;
  int renew_count;
  Glib::TimeSpan duration;
  int revision;

  explicit SyncLockInfo(const Glib::ustring & client);
  Glib::ustring hash_string();
};

class SyncClient
{
public:
  typedef std::unordered_map<Glib::ustring, Glib::ustring, Hash<Glib::ustring>> DeletedTitlesMap;
  virtual ~SyncClient();

  virtual int last_synchronized_revision() const = 0;
  virtual void last_synchronized_revision(int) = 0;
  virtual const Glib::DateTime &last_sync_date() const = 0;
  virtual void last_sync_date(const Glib::DateTime &) = 0;
  virtual int get_revision(const NoteBase &note) const = 0;
  virtual void set_revision(const NoteBase & note, int revision) = 0;
  virtual const DeletedTitlesMap &deleted_note_titles() const = 0;
  virtual void reset() = 0;
  virtual const Glib::ustring &associated_server_id() const = 0;
  virtual void associated_server_id(const Glib::ustring &) = 0;
  virtual void begin_synchronization() = 0;
  virtual void end_synchronization() = 0;
  virtual void cancel_synchronization() = 0;
};

class ISyncManager
{
public:
  virtual ~ISyncManager();

  virtual void reset_client() = 0;
  virtual void perform_synchronization(const SyncUI::Ptr & sync_ui) = 0;
  virtual bool synchronized_note_xml_matches(const Glib::ustring & noteXml1, const Glib::ustring & noteXml2) = 0;
  virtual SyncState state() const = 0;
};

class SyncServer
{
public:
  typedef std::unordered_map<Glib::ustring, NoteUpdate, Hash<Glib::ustring>> NoteUpdatesMap;

  virtual ~SyncServer();

  virtual bool begin_sync_transaction() = 0;
  virtual bool commit_sync_transaction() = 0;
  virtual bool cancel_sync_transaction() = 0;
  virtual std::vector<Glib::ustring> get_all_note_uuids() = 0;
  virtual NoteUpdatesMap get_note_updates_since(int revision) = 0;
  virtual void delete_notes(const std::vector<Glib::ustring> & deletedNoteUUIDs) = 0;
  virtual void upload_notes(const std::vector<NoteBase::Ref> & notes) = 0;
  virtual int latest_revision() = 0; // NOTE: Only reliable during a transaction
  virtual SyncLockInfo current_sync_lock() = 0;
  virtual Glib::ustring id() = 0;
  virtual bool updates_available_since(int revision) = 0;
};

class GnoteSyncException
  : public std::runtime_error
{
public:
  GnoteSyncException(const char *what_arg) : std::runtime_error(what_arg) {}
  GnoteSyncException(const Glib::ustring & what_arg) : std::runtime_error(what_arg.c_str()) {}
  GnoteSyncException(Glib::ustring && what_arg) : std::runtime_error(what_arg.c_str()) {}
};

}
}

#endif

