/*
 * gnote
 *
 * Copyright (C) 2011-2014 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
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

#ifndef __GNOTE_REMOTECONTROL_HPP_
#define __GNOTE_REMOTECONTROL_HPP_

#include <string>
#include <vector>

#include <giomm/dbusconnection.h>

#include "base/macros.hpp"
#include "dbus/iremotecontrol.hpp"
#include "mainwindow.hpp"
#include "note.hpp"


namespace gnote {

class NoteManager;

class RemoteControl
  : public IRemoteControl
{
public:
  RemoteControl(const Glib::RefPtr<Gio::DBus::Connection> &, gnote::NoteManager&, const char *, const char *,
                const Glib::RefPtr<Gio::DBus::InterfaceInfo> &);
  virtual ~RemoteControl();

  virtual bool AddTagToNote(const std::string& uri, const std::string& tag_name) override;
  virtual std::string CreateNamedNote(const std::string& linked_title) override;
  virtual std::string CreateNote() override;
  virtual bool DeleteNote(const std::string& uri) override;
  virtual bool DisplayNote(const std::string& uri) override;
  virtual bool DisplayNoteWithSearch(const std::string& uri, const std::string& search) override;
  virtual void DisplaySearch() override;
  virtual void DisplaySearchWithText(const std::string& search_text) override;
  virtual std::string FindNote(const std::string& linked_title) override;
  virtual std::string FindStartHereNote() override;
  virtual std::vector< std::string > GetAllNotesWithTag(const std::string& tag_name) override;
  virtual int32_t GetNoteChangeDate(const std::string& uri) override;
  virtual std::string GetNoteCompleteXml(const std::string& uri) override;
  virtual std::string GetNoteContents(const std::string& uri) override;
  virtual std::string GetNoteContentsXml(const std::string& uri) override;
  virtual int32_t GetNoteCreateDate(const std::string& uri) override;
  virtual std::string GetNoteTitle(const std::string& uri) override;
  virtual std::vector< std::string > GetTagsForNote(const std::string& uri) override;
  virtual bool HideNote(const std::string& uri) override;
  virtual std::vector< std::string > ListAllNotes() override;
  virtual bool NoteExists(const std::string& uri) override;
  virtual bool RemoveTagFromNote(const std::string& uri, const std::string& tag_name) override;
  virtual std::vector< std::string > SearchNotes(const std::string& query, const bool& case_sensitive) override;
  virtual bool SetNoteCompleteXml(const std::string& uri, const std::string& xml_contents) override;
  virtual bool SetNoteContents(const std::string& uri, const std::string& text_contents) override;
  virtual bool SetNoteContentsXml(const std::string& uri, const std::string& xml_contents) override;
  virtual std::string Version() override;

private:
  void on_note_added(const NoteBase::Ptr &);
  void on_note_deleted(const NoteBase::Ptr &);
  void on_note_saved(const NoteBase::Ptr &);
  MainWindow & present_note(const NoteBase::Ptr &);

  NoteManager & m_manager;
};


}

#endif

