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


#include <boost/format.hpp>

#include "isyncmanager.hpp"
#include "preferences.hpp"
#include "sharp/uuid.hpp"


namespace gnote {
namespace sync {


SyncLockInfo::SyncLockInfo()
  : client_id(Preferences::obj().get_schema_settings(Preferences::SCHEMA_SYNC)->get_string(Preferences::SYNC_CLIENT_ID))
  , transaction_id(sharp::uuid().string())
  , renew_count(0)
  , duration(0, 2, 0) // default of 2 minutes
  , revision(0)
{
}

std::string SyncLockInfo::hash_string()
{
  return str(boost::format("%1%-%2%-%3%-%4%-%5%") % transaction_id % client_id % renew_count % duration.string() % revision);
}



SyncClient::~SyncClient()
{
}


ISyncManager::~ISyncManager()
{
}



SyncServer::~SyncServer()
{}


}
}

