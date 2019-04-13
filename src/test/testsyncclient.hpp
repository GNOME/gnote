/*
 * gnote
 *
 * Copyright (C) 2014,2017,2019 Aurimas Cernius
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


#ifndef _TEST_SYNCCLIENT_HPP_
#define _TEST_SYNCCLIENT_HPP_

#include "synchronization/gnotesyncclient.hpp"


namespace test {

class SyncClient
  : public gnote::sync::GnoteSyncClient
{
public:
  typedef std::shared_ptr<SyncClient> Ptr;

  SyncClient(gnote::NoteManagerBase & manager);

  void set_manifest_path(const Glib::ustring & path)
  {
    m_local_manifest_file_path = path;
  }

  void reparse()
  {
    parse(m_local_manifest_file_path);
  }
};

}

#endif

