/*
 * gnote
 *
 * Copyright (C) 2014 Aurimas Cernius
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
  SyncManager(gnote::NoteManagerBase&);
  virtual void reset_client() override;
  virtual void perform_synchronization(const gnote::sync::SyncUI::Ptr & sync_ui) override;
  virtual void resolve_conflict(gnote::sync::SyncTitleConflictResolution resolution) override;
  virtual bool synchronized_note_xml_matches(const std::string & noteXml1, const std::string & noteXml2) override;
  virtual gnote::sync::SyncServiceAddin *get_sync_service_addin(const std::string & sync_service_id) override;
  virtual gnote::sync::SyncServiceAddin *get_configured_sync_service() override;
  test::SyncClient::Ptr get_client(const std::string & manifest);
};

}

#endif

