/*
 * gnote
 *
 * Copyright (C) 2012-2014 Aurimas Cernius
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

#include "base/macros.hpp"
#include "isyncmanager.hpp"



namespace gnote {
namespace sync {

  class GnoteSyncClient
    : public SyncClient
  {
  public:
    static SyncClient::Ptr create(NoteManagerBase &);

    virtual sharp::DateTime last_sync_date() override
      {
        return m_last_sync_date;
      }
    virtual void last_sync_date(const sharp::DateTime &) override;
    virtual int last_synchronized_revision() override
      {
        return m_last_sync_rev;
      }
    virtual void last_synchronized_revision(int) override;
    virtual int get_revision(const NoteBase::Ptr & note) override;
    virtual void set_revision(const NoteBase::Ptr & note, int revision) override;
    virtual std::map<std::string, std::string> deleted_note_titles() override
      {
        return m_deleted_notes;
      }
    virtual void reset() override;
    virtual std::string associated_server_id() override
      {
        return m_server_id;
      }
    virtual void associated_server_id(const std::string &) override;
  protected:
    GnoteSyncClient();
    void init(NoteManagerBase &);
    void parse(const std::string & manifest_path);

    std::string m_local_manifest_file_path;
  private:
    static const char *LOCAL_MANIFEST_FILE_NAME;

    void note_deleted_handler(const NoteBase::Ptr &);
    void on_changed(const Glib::RefPtr<Gio::File>&, const Glib::RefPtr<Gio::File>&,
                    Gio::FileMonitorEvent);
    void write(const std::string & manifest_path);
    void read_updated_note_atts(sharp::XmlReader & reader);
    void read_deleted_note_atts(sharp::XmlReader & reader);
    void read_notes(sharp::XmlReader & reader, void (GnoteSyncClient::*read_note_atts)(sharp::XmlReader&));

    Glib::RefPtr<Gio::FileMonitor> m_file_watcher;
    sharp::DateTime m_last_sync_date;
    int m_last_sync_rev;
    std::string m_server_id;
    std::map<std::string, int> m_file_revisions;
    std::map<std::string, std::string> m_deleted_notes;
  };

}
}
