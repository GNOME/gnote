/*
 * gnote
 *
 * Copyright (C) 2011-2014,2016-2017,2019-2020,2022-2024 Aurimas Cernius
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


  RemoteControl::RemoteControl(const Glib::RefPtr<Gio::DBus::Connection> & cnx, IGnote & g, NoteManagerBase & manager,
                               const char * path, const char * interface_name,
                               const Glib::RefPtr<Gio::DBus::InterfaceInfo> & gnote_interface)
    : IRemoteControl(cnx, path, interface_name, gnote_interface)
    , m_gnote(g)
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

  bool RemoteControl::AddTagToNote(const Glib::ustring& uri, const Glib::ustring& tag_name)
  {
    return m_manager.find_by_uri(uri, [this, &tag_name](NoteBase & note) {
      Tag &tag = m_manager.tag_manager().get_or_create_tag(tag_name);
      note.add_tag(tag);
    });
  }


  Glib::ustring RemoteControl::CreateNamedNote(const Glib::ustring& linked_title)
  {
    if(m_manager.find(linked_title)) {
      return "";
    }

    try {
      auto & note = m_manager.create(Glib::ustring(linked_title));
      return note.uri();
    } 
    catch (const std::exception & e) {
      ERR_OUT(_("Exception thrown when creating note: %s"), e.what());
    }
    return "";
  }

  Glib::ustring RemoteControl::CreateNote()
  {
    try {
      return m_manager.create().uri();
    } 
    catch(...)
    {
    }
    return "";
  }

  bool RemoteControl::DeleteNote(const Glib::ustring& uri)
  {
    return m_manager.find_by_uri(uri, [this](NoteBase & note) {
      m_manager.delete_note(note);
    });
  }

  bool RemoteControl::DisplayNote(const Glib::ustring& uri)
  {
    return m_manager.find_by_uri(uri, [this](NoteBase & note) {
      present_note(note);
    });
  }


  bool RemoteControl::DisplayNoteWithSearch(const Glib::ustring& uri, const Glib::ustring& search)
  {
    return m_manager.find_by_uri(uri, [this, &search](NoteBase & note) {
      MainWindow & window(present_note(note));
      window.set_search_text(Glib::ustring(search));
      window.show_search_bar();
    });
  }


  void RemoteControl::DisplaySearch()
  {
    m_gnote.open_search_all().present();
  }


  void RemoteControl::DisplaySearchWithText(const Glib::ustring& search_text)
  {
    MainWindow & recent_changes = m_gnote.open_search_all();
    recent_changes.set_search_text(Glib::ustring(search_text));
    recent_changes.present();
    recent_changes.show_search_bar();
  }


  Glib::ustring RemoteControl::FindNote(const Glib::ustring& linked_title)
  {
    auto note = m_manager.find(linked_title);
    return (!note) ? "" : note.value().get().uri();
  }


  Glib::ustring RemoteControl::FindStartHereNote()
  {
    Glib::ustring ret;
    m_manager.find_by_uri(m_gnote.preferences().start_note_uri(), [&ret](NoteBase & note) {
      ret = note.uri();
    });
    return ret;
  }


  std::vector<Glib::ustring> RemoteControl::GetAllNotesWithTag(const Glib::ustring& tag_name)
  {
    auto tag = m_manager.tag_manager().get_tag(tag_name);
    if(!tag)
      return std::vector<Glib::ustring>();

    std::vector<Glib::ustring> tagged_note_uris;
    auto notes = tag.value().get().get_notes();
    for(NoteBase *iter : notes) {
      tagged_note_uris.push_back(iter->uri());
    }
    return tagged_note_uris;
  }


  int32_t RemoteControl::GetNoteChangeDate(const Glib::ustring& uri)
  {
    return GetNoteChangeDateUnix(uri);
  }


  int64_t RemoteControl::GetNoteChangeDateUnix(const Glib::ustring& uri)
  {
    int64_t ret = -1;
    m_manager.find_by_uri(uri, [&ret](NoteBase & note) {
      ret = note.metadata_change_date().to_unix();
    });
    return ret;
  }


  Glib::ustring RemoteControl::GetNoteCompleteXml(const Glib::ustring& uri)
  {
    Glib::ustring xml;
    m_manager.find_by_uri(uri, [&xml](NoteBase & note) {
      xml = note.get_complete_note_xml();
    });
    return xml;
  }


  Glib::ustring RemoteControl::GetNoteContents(const Glib::ustring& uri)
  {
    Glib::ustring text;
    m_manager.find_by_uri(uri, [&text](NoteBase & note) {
      text = note.text_content();
    });
    return text;
  }


  Glib::ustring RemoteControl::GetNoteContentsXml(const Glib::ustring& uri)
  {
    Glib::ustring xml;
    m_manager.find_by_uri(uri, [&xml](NoteBase & note) {
      xml = note.xml_content();
    });
    return xml;
  }


  int32_t RemoteControl::GetNoteCreateDate(const Glib::ustring& uri)
  {
    return GetNoteCreateDateUnix(uri);
  }


  int64_t RemoteControl::GetNoteCreateDateUnix(const Glib::ustring& uri)
  {
    int64_t ret = -1;
    m_manager.find_by_uri(uri, [&ret](NoteBase & note) {
      ret = note.create_date().to_unix();
    });
    return ret;
  }


  Glib::ustring RemoteControl::GetNoteTitle(const Glib::ustring& uri)
  {
    Glib::ustring title;
    m_manager.find_by_uri(uri, [&title](NoteBase & note) {
      title = note.get_title();
    });
    return title;
  }


  std::vector<Glib::ustring> RemoteControl::GetTagsForNote(const Glib::ustring& uri)
  {
    std::vector<Glib::ustring> tags;
    m_manager.find_by_uri(uri, [&tags](NoteBase & note) {
      for(Tag &tag : note.get_tags()) {
        tags.push_back(tag.normalized_name());
      }
    });
    return tags;
  }


bool RemoteControl::HideNote(const Glib::ustring& uri)
{
  return m_manager.find_by_uri(uri, [](NoteBase & note) {
    if(auto window = static_cast<Note&>(note).get_window()) {
      if(auto win = dynamic_cast<MainWindow*>(window->host())) {
        win->unembed_widget(*window);
      }
    }
  });
}


std::vector<Glib::ustring> RemoteControl::ListAllNotes()
{
  std::vector<Glib::ustring> uris;
  m_manager.for_each([&uris](const NoteBase & note) {
    uris.push_back(note.uri());
  });
  return uris;
}


bool RemoteControl::NoteExists(const Glib::ustring& uri)
{
  return m_manager.find_by_uri(uri).has_value();
}


bool RemoteControl::RemoveTagFromNote(const Glib::ustring& uri, 
                                      const Glib::ustring& tag_name)
{
  return m_manager.find_by_uri(uri, [this, &tag_name](NoteBase & note) {
    if(auto tag = m_manager.tag_manager().get_tag(tag_name)) {
      note.remove_tag(*tag);
    }
  });
}


std::vector<Glib::ustring> RemoteControl::SearchNotes(const Glib::ustring& query,
                                                      const bool& case_sensitive)
{
  if (query.empty())
    return std::vector<Glib::ustring>();

  Search search(m_manager);
  std::vector<Glib::ustring> list;
  auto results = search.search_notes(query, case_sensitive, notebooks::Notebook::ORef());

  for(auto iter = results.rbegin(); iter != results.rend(); ++iter) {
    list.push_back(iter->second.get().uri());
  }

  return list;
}


bool RemoteControl::SetNoteCompleteXml(const Glib::ustring& uri, 
                                       const Glib::ustring& xml_contents)
{
  return m_manager.find_by_uri(uri, [&xml_contents](NoteBase & note) {
    note.load_foreign_note_xml(xml_contents, CONTENT_CHANGED);
  });
}


bool RemoteControl::SetNoteContents(const Glib::ustring& uri, 
                                    const Glib::ustring& text_contents)
{
  return m_manager.find_by_uri(uri, [&text_contents](NoteBase & note) {
    static_cast<Note&>(note).set_text_content(Glib::ustring(text_contents));
  });
}


bool RemoteControl::SetNoteContentsXml(const Glib::ustring& uri, 
                                       const Glib::ustring& xml_contents)
{
  return m_manager.find_by_uri(uri, [&xml_contents](NoteBase & note) {
    note.set_xml_content(Glib::ustring(xml_contents));
  });
}


Glib::ustring RemoteControl::Version()
{
  return VERSION;
}



void RemoteControl::on_note_added(NoteBase & note)
{
  NoteAdded(note.uri());
}


void RemoteControl::on_note_deleted(NoteBase & note)
{
  NoteDeleted(note.uri(), note.get_title());
}


void RemoteControl::on_note_saved(NoteBase & note)
{
  NoteSaved(note.uri());
}


MainWindow & RemoteControl::present_note(NoteBase & note)
{
  return MainWindow::present_default(m_gnote, static_cast<Note&>(note));
}


}
