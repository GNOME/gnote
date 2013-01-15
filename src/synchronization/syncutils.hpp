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


#ifndef _SYNCHRONIZATION_SYNCUTILS_HPP_
#define _SYNCHRONIZATION_SYNCUTILS_HPP_


#include <string>
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
    std::string m_xml_content;//Empty if deleted?
    std::string m_title;
    std::string m_uuid; //needed?
    int m_latest_revision;

    NoteUpdate(const std::string & xml_content, const std::string & title, const std::string & uuid, int latest_revision);
    bool basically_equal_to(const Note::Ptr & existing_note);
  private:
    std::string get_inner_content(const std::string & full_content_element) const;
    bool compare_tags(const std::map<std::string, Tag::Ptr> set1, const std::map<std::string, Tag::Ptr> set2) const;
  };


  class SyncUtils
    : public base::Singleton<SyncUtils>
  {
  public:
    bool is_fuse_enabled();
    bool enable_fuse();
    std::string find_first_executable_in_path(const std::vector<std::string> & executableNames);
    std::string find_first_executable_in_path(const std::string & executableName);
  private:
    static const char *common_paths[];
    static SyncUtils s_obj;

    std::string m_guisu_tool;
    std::string m_modprobe_tool;
  };

}
}


#endif
