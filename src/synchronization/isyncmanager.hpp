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


#ifndef _SYNCHRONIZATION_ISYNCMANAGER_HPP_
#define _SYNCHRONIZATION_ISYNCMANAGER_HPP_

#include <list>
#include <string>
#include <tr1/memory>

#include "note.hpp"
#include "syncui.hpp"
#include "syncutils.hpp"

namespace gnote {
namespace sync {

class SyncLockInfo
{
public:
  std::string client_id;
  std::string transaction_id;
  int renew_count;
  sharp::TimeSpan duration;
  int revision;

  SyncLockInfo();
  std::string hash_string();
};

class SyncClient
{
public:
  typedef std::tr1::shared_ptr<SyncClient> Ptr;

  virtual ~SyncClient();

  virtual int last_synchronized_revision() = 0;
  virtual void last_synchronized_revision(int) = 0;
  virtual sharp::DateTime last_sync_date() = 0;
  virtual void last_sync_date(const sharp::DateTime &) = 0;
  virtual int get_revision(const Note::Ptr & note) = 0;
  virtual void set_revision(const Note::Ptr & note, int revision) = 0;
  virtual std::map<std::string, std::string> deleted_note_titles() = 0;
  virtual void reset() = 0;
  virtual std::string associated_server_id() = 0;
  virtual void associated_server_id(const std::string &) = 0;
};

class ISyncManager
  : public base::Singleton<ISyncManager>
{
public:
  virtual ~ISyncManager();

  virtual void reset_client() = 0;
  virtual void perform_synchronization(const std::tr1::shared_ptr<SyncUI> & sync_ui) = 0;
  virtual void resolve_conflict(SyncTitleConflictResolution resolution) = 0;
  virtual bool synchronized_note_xml_matches(const std::string & noteXml1, const std::string & noteXml2) = 0;
  virtual SyncState state() const = 0;
};

class SyncServer
{
public:
  typedef std::tr1::shared_ptr<SyncServer> Ptr;

  virtual ~SyncServer();

  virtual bool begin_sync_transaction() = 0;
  virtual bool commit_sync_transaction() = 0;
  virtual bool cancel_sync_transaction() = 0;
  virtual std::list<std::string> get_all_note_uuids() = 0;
  virtual std::map<std::string, NoteUpdate> get_note_updates_since(int revision) = 0;
  virtual void delete_notes(const std::list<std::string> & deletedNoteUUIDs) = 0;
  virtual void upload_notes(const std::list<Note::Ptr> & notes) = 0;
  virtual int latest_revision() = 0; // NOTE: Only reliable during a transaction
  virtual SyncLockInfo current_sync_lock() = 0;
  virtual std::string id() = 0;
  virtual bool updates_available_since(int revision) = 0;
};

class GnoteSyncException
  : public std::runtime_error
{
public:
  GnoteSyncException(const char * what_arg) : std::runtime_error(what_arg){}
};

}
}

#endif

