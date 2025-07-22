/*
 * gnote
 *
 * Copyright (C) 2012-2014,2017,2019-2020,2023,2025 Aurimas Cernius
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


#include <giomm/file.h>

#include "isyncmanager.hpp"



namespace gnote {
namespace sync {

  class GnoteSyncClient
    : public SyncClient
  {
  public:
    static SyncClient *create(NoteManagerBase &);
    GnoteSyncClient();

    const Glib::DateTime &last_sync_date() const override
      {
        return m_last_sync_date;
      }
    void last_sync_date(const Glib::DateTime &) override;
    int last_synchronized_revision() const override
      {
        return m_last_sync_rev;
      }
    void last_synchronized_revision(int) override;
    int get_revision(const NoteBase &note) const override;
    void set_revision(const NoteBase &note, int revision) override;
    const DeletedTitlesMap &deleted_note_titles() const override
      {
        return m_deleted_notes;
      }
    void reset() override;
    const Glib::ustring &associated_server_id() const override
      {
        return m_server_id;
      }
    void associated_server_id(const Glib::ustring &) override;
    void begin_synchronization() override;
    void end_synchronization() override;
    void cancel_synchronization() override;
  protected:
    void init(NoteManagerBase &);
    void parse(const Glib::ustring & manifest_path);

    Glib::ustring m_local_manifest_file_path;
  private:
    static const char *LOCAL_MANIFEST_FILE_NAME;

    void note_deleted_handler(NoteBase &);
    void write(const Glib::ustring & manifest_path);
    void read_updated_note_atts(sharp::XmlReader & reader);
    void read_deleted_note_atts(sharp::XmlReader & reader);
    void read_notes(sharp::XmlReader & reader, void (GnoteSyncClient::*read_note_atts)(sharp::XmlReader&));

    Glib::DateTime m_last_sync_date;
    int m_last_sync_rev;
    Glib::ustring m_server_id;
    std::map<Glib::ustring, int> m_file_revisions;
    DeletedTitlesMap  m_deleted_notes;
    bool m_synchronizing;
  };

}
}
