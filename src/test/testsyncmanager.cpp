/*
 * gnote
 *
 * Copyright (C) 2017-2020,2023,2025 Aurimas Cernius
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


#include "testsyncaddin.hpp"
#include "testsyncmanager.hpp"

namespace test {

SyncManager::SyncManager(gnote::IGnote & g, gnote::NoteManagerBase & manager, const Glib::ustring & sync_path)
  : gnote::sync::SyncManager(g, manager)
  , m_sync_path(sync_path)
{
  m_client.reset(new test::SyncClient(manager));
}

test::SyncClient & SyncManager::get_client(const Glib::ustring & manifest)
{
  SyncClient & client = dynamic_cast<SyncClient&>(*m_client);
  client.set_manifest_path(manifest);
  client.reparse();
  return client;
}

void SyncManager::reset_client()
{
}

void SyncManager::perform_synchronization(const gnote::sync::SyncUI::Ptr & sync_ui)
{
  m_sync_ui = sync_ui;
  synchronization_thread();
}

bool SyncManager::synchronized_note_xml_matches(const Glib::ustring & /*noteXml1*/, const Glib::ustring & /*noteXml2*/)
{
  return false;
}

gnote::sync::SyncServiceAddin *SyncManager::get_sync_service_addin(const Glib::ustring & /*sync_service_id*/)
{
  return new SyncAddin(m_sync_path);
}

gnote::sync::SyncServiceAddin *SyncManager::get_configured_sync_service()
{
  return get_sync_service_addin("");
}

void SyncManager::delete_notes_in_main_thread(gnote::sync::SyncServer & server)
{
  delete_notes(server);
}

void SyncManager::note_save(const gnote::NoteBase & note)
{
  const_cast<gnote::NoteBase&>(note).save();
}

void SyncManager::create_note_in_main_thread(const gnote::sync::NoteUpdate & noteUpdate)
{
  create_note(noteUpdate);
}

void SyncManager::update_note_in_main_thread(const gnote::NoteBase & existing_note, const gnote::sync::NoteUpdate & note_update)
{
  update_note(const_cast<gnote::NoteBase&>(existing_note), note_update);
}

void SyncManager::delete_note_in_main_thread(const gnote::NoteBase & existing_note)
{
  delete_note(const_cast<gnote::NoteBase&>(existing_note));
}

}

