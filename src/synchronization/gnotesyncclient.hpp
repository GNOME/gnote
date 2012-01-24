/*
 * gnote
 *
 * Copyright (C) 2012 Aurimas Cernius
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


#include "syncmanager.hpp"



namespace gnote {
namespace sync {

  class GnoteSyncClient
    : public SyncClient
  {
  public:
    GnoteSyncClient();

    virtual sharp::DateTime last_sync_date()
      {
        return m_last_sync_date;
      }
    virtual void last_sync_date(const sharp::DateTime &);
    virtual int last_synchronized_revision()
      {
        return m_last_sync_rev;
      }
    virtual void last_synchronized_revision(int);
    virtual int get_revision(const Note::Ptr & note);
    virtual void set_revision(const Note::Ptr & note, int revision);
    virtual std::map<std::string, std::string> deleted_note_titles()
      {
        return m_deleted_notes;
      }
    virtual void reset();
    virtual std::string associated_server_id()
      {
        return m_server_id;
      }
    virtual void associated_server_id(const std::string &);
  private:
    static const char *LOCAL_MANIFEST_FILE_NAME;

    sharp::DateTime m_last_sync_date;
    int m_last_sync_rev;
    std::string m_server_id;
    std::string m_local_manifest_file_path;
    std::map<std::string, int> m_file_revisions;
    std::map<std::string, std::string> m_deleted_notes;
  };

}
}
