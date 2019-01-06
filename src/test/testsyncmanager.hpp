/*
 * gnote
 *
 * Copyright (C) 2017-2018 Aurimas Cernius
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


#ifndef _TEST_TESTSYNCMANAGER_HPP_
#define _TEST_TESTSYNCMANAGER_HPP_

#include "base/macros.hpp"
#include "synchronization/syncmanager.hpp"
#include "testsyncclient.hpp"


namespace test {

class SyncManager
  : public gnote::sync::SyncManager
{
public:
  SyncManager(gnote::NoteManagerBase & note_manager, const Glib::ustring & sync_path);
  virtual void reset_client() override;
  virtual void perform_synchronization(const gnote::sync::SyncUI::Ptr & sync_ui) override;
  virtual void resolve_conflict(gnote::sync::SyncTitleConflictResolution resolution) override;
  virtual bool synchronized_note_xml_matches(const Glib::ustring & noteXml1, const Glib::ustring & noteXml2) override;
  virtual gnote::sync::SyncServiceAddin *get_sync_service_addin(const Glib::ustring & sync_service_id) override;
  virtual gnote::sync::SyncServiceAddin *get_configured_sync_service() override;
  virtual void delete_notes_in_main_thread(const gnote::sync::SyncServer::Ptr & server) override;
  virtual void note_save(const gnote::Note::Ptr & note) override;
  test::SyncClient::Ptr get_client(const Glib::ustring & manifest);
protected:
  virtual void create_note_in_main_thread(const gnote::sync::NoteUpdate & noteUpdate) override;
  virtual void update_note_in_main_thread(const gnote::Note::Ptr & existingNote, const gnote::sync::NoteUpdate & noteUpdate) override;
  virtual void delete_note_in_main_thread(const gnote::Note::Ptr & existingNote) override;
private:
  Glib::ustring m_sync_path;
};

}

#endif

