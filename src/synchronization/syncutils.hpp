/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017,2019,2021,2023-2025 Aurimas Cernius
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


#include "note.hpp"


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
    [[nodiscard]] bool basically_equal_to(const NoteBase &existing_note) const;
  private:
    Glib::ustring get_inner_content(const Glib::ustring & full_content_element) const;
    bool compare_tags(const NoteData::TagSet &set1, const NoteData::TagSet &set2) const;
  };

}
}


#endif
