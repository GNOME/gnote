/*
 * gnote
 *
 * Copyright (C) 2011-2014,2016 Aurimas Cernius
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

#include <glibmm/i18n.h>

#include "config.h"

#include "debug.hpp"
#include "ignote.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "remotecontrolproxy.hpp"
#include "search.hpp"
#include "tag.hpp"
#include "itagmanager.hpp"
#include "dbus/remotecontrol.hpp"
#include "sharp/map.hpp"

namespace gnote {


  RemoteControl::RemoteControl(const Glib::RefPtr<Gio::DBus::Connection> & cnx, NoteManager& manager,
                               const char * path, const char * interface_name,
                               const Glib::RefPtr<Gio::DBus::InterfaceInfo> & gnote_interface)
    : IRemoteControl(cnx, path, interface_name, gnote_interface)
    , m_manager(manager)
  {
    DBG_OUT("initialized remote control");
    m_manager.signal_note_added.connect(
      sigc::mem_fun(*this, &RemoteControl::on_note_added));
    m_manager.signal_note_deleted.connect(
      sigc::mem_fun(*this, &RemoteControl::on_note_deleted));
    m_manager.signal_note_saved.connect(
      sigc::mem_fun(*this, &RemoteControl::on_note_saved));
  }


  RemoteControl::~RemoteControl()
  {
  }

  bool RemoteControl::AddTagToNote(const std::string& uri, const std::string& tag_name)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note) {
      return false;
    }
    Tag::Ptr tag = ITagManager::obj().get_or_create_tag(tag_name);
    note->add_tag (tag);
    return true;
  }


  std::string RemoteControl::CreateNamedNote(const std::string& linked_title)
  {
    NoteBase::Ptr note = m_manager.find(linked_title);
    if (note)
      return "";

    try {
      note = m_manager.create (linked_title);
      return note->uri();
    } 
    catch (const std::exception & e) {
      ERR_OUT(_("Exception thrown when creating note: %s"), e.what());
    }
    return "";
  }

  std::string RemoteControl::CreateNote()
  {
    try {
      NoteBase::Ptr note = m_manager.create ();
      return note->uri();
    } 
    catch(...)
    {
    }
    return "";
  }

  bool RemoteControl::DeleteNote(const std::string& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note) {
      return false;
    }

    m_manager.delete_note (note);
    return true;

  }

  bool RemoteControl::DisplayNote(const std::string& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note) {
      return false;
    }

    present_note(note);
    return true;
  }


  bool RemoteControl::DisplayNoteWithSearch(const std::string& uri, const std::string& search)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note) {
      return false;
    }

    MainWindow & window(present_note(note));
    window.set_search_text(search);
    window.show_search_bar();

    return true;
  }


  void RemoteControl::DisplaySearch()
  {
    IGnote::obj().open_search_all().present();
  }


  void RemoteControl::DisplaySearchWithText(const std::string& search_text)
  {
    MainWindow & recent_changes = IGnote::obj().get_main_window();
    recent_changes.set_search_text(search_text);
    recent_changes.present();
    recent_changes.show_search_bar();
  }


  std::string RemoteControl::FindNote(const std::string& linked_title)
  {
    NoteBase::Ptr note = m_manager.find(linked_title);
    return (!note) ? "" : note->uri();
  }


  std::string RemoteControl::FindStartHereNote()
  {
    NoteBase::Ptr note = m_manager.find_by_uri(m_manager.start_note_uri());
    return (!note) ? "" : note->uri();
  }


  std::vector< std::string > RemoteControl::GetAllNotesWithTag(const std::string& tag_name)
  {
    Tag::Ptr tag = ITagManager::obj().get_tag(tag_name);
    if (!tag)
      return std::vector< std::string >();

    std::vector< std::string > tagged_note_uris;
    std::list<NoteBase*> notes;
    tag->get_notes(notes);
    FOREACH(NoteBase *iter, notes) {
      tagged_note_uris.push_back(iter->uri());
    }
    return tagged_note_uris;
  }


  int32_t RemoteControl::GetNoteChangeDate(const std::string& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return -1;
    return note->metadata_change_date().sec();
  }


  std::string RemoteControl::GetNoteCompleteXml(const std::string& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return "";
    return note->get_complete_note_xml();

  }


  std::string RemoteControl::GetNoteContents(const std::string& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return "";
    return static_pointer_cast<Note>(note)->text_content();
  }


  std::string RemoteControl::GetNoteContentsXml(const std::string& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return "";
    return note->xml_content();
  }


  int32_t RemoteControl::GetNoteCreateDate(const std::string& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return -1;
    return note->create_date().sec();
  }


  std::string RemoteControl::GetNoteTitle(const std::string& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return "";
    return note->get_title();
  }


  std::vector< std::string > RemoteControl::GetTagsForNote(const std::string& uri)
  {
    NoteBase::Ptr note = m_manager.find_by_uri(uri);
    if (!note)
      return std::vector< std::string >();

    std::vector< std::string > tags;
    std::list<Tag::Ptr> l;
    note->get_tags(l);
    for (std::list<Tag::Ptr>::const_iterator iter = l.begin();
         iter != l.end(); ++iter) {
      tags.push_back((*iter)->normalized_name());
    }
    return tags;
  }


bool RemoteControl::HideNote(const std::string& uri)
{
  NoteBase::Ptr note = m_manager.find_by_uri(uri);
  if (!note)
    return false;

  NoteWindow *window = static_pointer_cast<Note>(note)->get_window();
  if(window == NULL) {
    return true;
  }
  MainWindow *win = MainWindow::get_owning(*window);
  if(win) {
    win->unembed_widget(*window);
  }
  return true;
}


std::vector< std::string > RemoteControl::ListAllNotes()
{
  std::vector< std::string > uris;
  FOREACH(const NoteBase::Ptr & iter, m_manager.get_notes()) {
    uris.push_back(iter->uri());
  }
  return uris;
}


bool RemoteControl::NoteExists(const std::string& uri)
{
  NoteBase::Ptr note = m_manager.find_by_uri(uri);
  return note != NULL;
}


bool RemoteControl::RemoveTagFromNote(const std::string& uri, 
                                      const std::string& tag_name)
{
  NoteBase::Ptr note = m_manager.find_by_uri(uri);
  if (!note)
    return false;
  Tag::Ptr tag = ITagManager::obj().get_tag(tag_name);
  if (tag) {
    note->remove_tag (tag);
  }
  return true;
}


std::vector< std::string > RemoteControl::SearchNotes(const std::string& query,
                                                      const bool& case_sensitive)
{
  if (query.empty())
    return std::vector< std::string >();

  Search search(m_manager);
  std::vector< std::string > list;
  Search::ResultsPtr results =
    search.search_notes(query, case_sensitive, notebooks::Notebook::Ptr());

  for(Search::Results::const_reverse_iterator iter = results->rbegin();
      iter != results->rend(); iter++) {

    list.push_back(iter->second->uri());
  }

  return list;
}


bool RemoteControl::SetNoteCompleteXml(const std::string& uri, 
                                       const std::string& xml_contents)
{
  NoteBase::Ptr note = m_manager.find_by_uri(uri);
  if(!note) {
    return false;
  }
    
  note->load_foreign_note_xml(xml_contents, CONTENT_CHANGED);
  return true;
}


bool RemoteControl::SetNoteContents(const std::string& uri, 
                                    const std::string& text_contents)
{
  NoteBase::Ptr note = m_manager.find_by_uri(uri);
  if(!note) {
    return false;
  }

  static_pointer_cast<Note>(note)->set_text_content(text_contents);
  return true;
}


bool RemoteControl::SetNoteContentsXml(const std::string& uri, 
                                       const std::string& xml_contents)
{
  NoteBase::Ptr note = m_manager.find_by_uri(uri);
  if(!note) {
    return false;
  }
  note->set_xml_content(xml_contents);
  return true;
}


std::string RemoteControl::Version()
{
  return VERSION;
}



void RemoteControl::on_note_added(const NoteBase::Ptr & note)
{
  if(note) {
    NoteAdded(note->uri());
  }
}


void RemoteControl::on_note_deleted(const NoteBase::Ptr & note)
{
  if(note) {
    NoteDeleted(note->uri(), note->get_title());
  }
}


void RemoteControl::on_note_saved(const NoteBase::Ptr & note)
{
  if(note) {
    NoteSaved(note->uri());
  }
}


MainWindow & RemoteControl::present_note(const NoteBase::Ptr & note)
{
  MainWindow & window = IGnote::obj().get_window_for_note();
  MainWindow::present_in(window, static_pointer_cast<Note>(note));
  return window;
}


}
