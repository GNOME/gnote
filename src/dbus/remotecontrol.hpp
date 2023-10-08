/*
 * gnote
 *
 * Copyright (C) 2011-2014,2017,2019-2020,2023 Aurimas Cernius
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

#include <vector>

#include <giomm/dbusconnection.h>

#include "dbus/iremotecontrol.hpp"
#include "mainwindow.hpp"
#include "note.hpp"


namespace gnote {

class IGnote;
class NoteManagerBase;

class RemoteControl
  : public IRemoteControl
{
public:
  RemoteControl(const Glib::RefPtr<Gio::DBus::Connection> &, IGnote&, NoteManagerBase&, const char *, const char *,
                const Glib::RefPtr<Gio::DBus::InterfaceInfo> &);
  virtual ~RemoteControl();

  virtual bool AddTagToNote(const Glib::ustring& uri, const Glib::ustring& tag_name) override;
  virtual Glib::ustring CreateNamedNote(const Glib::ustring& linked_title) override;
  virtual Glib::ustring CreateNote() override;
  virtual bool DeleteNote(const Glib::ustring& uri) override;
  virtual bool DisplayNote(const Glib::ustring& uri) override;
  virtual bool DisplayNoteWithSearch(const Glib::ustring& uri, const Glib::ustring& search) override;
  virtual void DisplaySearch() override;
  virtual void DisplaySearchWithText(const Glib::ustring& search_text) override;
  virtual Glib::ustring FindNote(const Glib::ustring& linked_title) override;
  virtual Glib::ustring FindStartHereNote() override;
  virtual std::vector<Glib::ustring> GetAllNotesWithTag(const Glib::ustring& tag_name) override;
  virtual int32_t GetNoteChangeDate(const Glib::ustring& uri) override;
  virtual int64_t GetNoteChangeDateUnix(const Glib::ustring& uri) override;
  virtual Glib::ustring GetNoteCompleteXml(const Glib::ustring& uri) override;
  virtual Glib::ustring GetNoteContents(const Glib::ustring& uri) override;
  virtual Glib::ustring GetNoteContentsXml(const Glib::ustring& uri) override;
  virtual int32_t GetNoteCreateDate(const Glib::ustring& uri) override;
  virtual int64_t GetNoteCreateDateUnix(const Glib::ustring& uri) override;
  virtual Glib::ustring GetNoteTitle(const Glib::ustring& uri) override;
  virtual std::vector<Glib::ustring> GetTagsForNote(const Glib::ustring& uri) override;
  virtual bool HideNote(const Glib::ustring& uri) override;
  virtual std::vector<Glib::ustring> ListAllNotes() override;
  virtual bool NoteExists(const Glib::ustring& uri) override;
  virtual bool RemoveTagFromNote(const Glib::ustring& uri, const Glib::ustring& tag_name) override;
  virtual std::vector<Glib::ustring> SearchNotes(const Glib::ustring& query, const bool& case_sensitive) override;
  virtual bool SetNoteCompleteXml(const Glib::ustring& uri, const Glib::ustring& xml_contents) override;
  virtual bool SetNoteContents(const Glib::ustring& uri, const Glib::ustring& text_contents) override;
  virtual bool SetNoteContentsXml(const Glib::ustring& uri, const Glib::ustring& xml_contents) override;
  virtual Glib::ustring Version() override;

private:
  void on_note_added(NoteBase &);
  void on_note_deleted(NoteBase &);
  void on_note_saved(NoteBase &);
  MainWindow & present_note(NoteBase &);

  IGnote & m_gnote;
  NoteManagerBase & m_manager;
};


}

#endif

