/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017 Aurimas Cernius
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


#ifndef _SYNCHRONIZATION_SYNCUTILS_HPP_
#define _SYNCHRONIZATION_SYNCUTILS_HPP_


#include <vector>

#include "note.hpp"
#include "base/singleton.hpp"


namespace gnote {
namespace sync {

  enum SyncState {
    IDLE,
    NO_CONFIGURED_SYNC_SERVICE,
    SYNC_SERVER_CREATION_FAILED,
    CONNECTING,
    ACQUIRING_LOCK,
    LOCKED,
    PREPARE_DOWNLOAD,
    DOWNLOADING,
    PREPARE_UPLOAD,
    UPLOADING,
    DELETE_SERVER_NOTES,
    COMMITTING_CHANGES,
    SUCCEEDED,
    FAILED,
    USER_CANCELLED
  };

  enum NoteSyncType {
    UPLOAD_NEW,
    UPLOAD_MODIFIED,
    DOWNLOAD_NEW,
    DOWNLOAD_MODIFIED,
    DELETE_FROM_SERVER,
    DELETE_FROM_CLIENT
  };

  enum SyncTitleConflictResolution {
    CANCEL,
    OVERWRITE_EXISTING,
    RENAME_EXISTING_NO_UPDATE,
    RENAME_EXISTING_AND_UPDATE
  };

  class NoteUpdate
  {
  public:
    Glib::ustring m_xml_content;//Empty if deleted?
    Glib::ustring m_title;
    Glib::ustring m_uuid; //needed?
    int m_latest_revision;

    NoteUpdate(const Glib::ustring & xml_content, const Glib::ustring & title, const Glib::ustring & uuid, int latest_revision);
    bool basically_equal_to(const Note::Ptr & existing_note);
  private:
    Glib::ustring get_inner_content(const Glib::ustring & full_content_element) const;
    bool compare_tags(const std::map<Glib::ustring, Tag::Ptr> set1, const std::map<Glib::ustring, Tag::Ptr> set2) const;
  };


  class SyncUtils
    : public base::Singleton<SyncUtils>
  {
  public:
    bool is_fuse_enabled();
    bool enable_fuse();
    Glib::ustring find_first_executable_in_path(const std::vector<Glib::ustring> & executableNames);
    Glib::ustring find_first_executable_in_path(const Glib::ustring & executableName);
  private:
    static const char *common_paths[];
    static SyncUtils s_obj;

    Glib::ustring m_guisu_tool;
    Glib::ustring m_modprobe_tool;
  };

}
}


#endif
