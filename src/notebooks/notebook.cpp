/*
 * gnote
 *
 * Copyright (C) 2010-2014,2017 Aurimas Cernius
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

#include "sharp/string.hpp"
#include "notemanager.hpp"
#include "notebooks/notebook.hpp"
#include "notebooks/notebookmanager.hpp"
#include "itagmanager.hpp"

namespace gnote {
namespace notebooks {


  const char * Notebook::NOTEBOOK_TAG_PREFIX = "notebook:";
  Tag::Ptr Notebook::s_template_tag;

  Tag::Ptr Notebook::template_tag()
  {
    if(s_template_tag == NULL) {
      s_template_tag = ITagManager::obj().get_or_create_system_tag(
        ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
    }

    return s_template_tag;
  }

  bool Notebook::is_template_note(const Note::Ptr & note)
  {
    Tag::Ptr tag = template_tag();
    if(tag == NULL) {
      return false;
    }
    return note->contains_tag(tag);
  }

  /// <summary>
  /// Construct a new Notebook with a given name
  /// </summary>
  /// <param name="name">
  /// A <see cref="System.String"/>.  This is the name that will be used
  /// to identify the notebook.
  /// </param>
  Notebook::Notebook(NoteManager & manager, const Glib::ustring & name, bool is_special)
    : m_note_manager(manager)
  {
    // is special assume the name as is, and we don't want a tag.
    if(is_special) {
      m_name = name;
    }
    else {
      set_name(name);
      m_tag = ITagManager::obj().get_or_create_system_tag(
        Glib::ustring(NOTEBOOK_TAG_PREFIX) + name);
    }
  }

  /// <summary>
  /// Construct a new Notebook with the specified notebook system tag.
  /// </summary>
  /// <param name="notebookTag">
  /// A <see cref="Tag"/>.  This must be a system notebook tag.
  /// </param>
  Notebook::Notebook(NoteManager & manager, const Tag::Ptr & notebookTag)
    : m_note_manager(manager)
  {
  // Parse the notebook name from the tag name
    Glib::ustring systemNotebookPrefix = Glib::ustring(Tag::SYSTEM_TAG_PREFIX)
      + NOTEBOOK_TAG_PREFIX;
    Glib::ustring notebookName = sharp::string_substring(notebookTag->name(),
                                                       systemNotebookPrefix.length());
    set_name(notebookName);
    m_tag = notebookTag;
  }

  void Notebook::set_name(const Glib::ustring & value)
  {
    Glib::ustring trimmedName = sharp::string_trim(value);
    if(!trimmedName.empty()) {
      m_name = trimmedName;
      m_normalized_name = trimmedName.lowercase();

      // The templateNoteTite should show the name of the
      // notebook.  For example, if the name of the notebooks
      // "Meetings", the templateNoteTitle should be "Meetings
      // Notebook Template".  Translators should place the
      // name of the notebook accordingly using "%1".
      Glib::ustring format = _("%1 Notebook Template");
      m_default_template_note_title = Glib::ustring::compose(format, m_name);
    }
  }


  Glib::ustring Notebook::get_normalized_name() const
  {
    return m_normalized_name;
  }


  Tag::Ptr Notebook::get_tag() const
  { 
    return m_tag; 
  }


  Note::Ptr Notebook::find_template_note() const
  {
    Note::Ptr note;
    Tag::Ptr templ_tag = template_tag();
    Tag::Ptr notebook_tag = ITagManager::obj().get_system_tag(NOTEBOOK_TAG_PREFIX + get_name());
    if(!templ_tag || !notebook_tag) {
      return note;
    }
    std::list<NoteBase*> notes;
    templ_tag->get_notes(notes);
    FOREACH(NoteBase *n, notes) {
      if(n->contains_tag(notebook_tag)) {
        note = static_pointer_cast<Note>(n->shared_from_this());
        break;
      }
    }

    return note;
  }

  Note::Ptr Notebook::get_template_note() const
  {
    NoteBase::Ptr note = find_template_note();

    if (!note) {
      Glib::ustring title = m_default_template_note_title;
      if(m_note_manager.find(title)) {
        std::list<NoteBase*> tag_notes;
        m_tag->get_notes(tag_notes);
        title = m_note_manager.get_unique_name(title);
      }
      note = m_note_manager.create(title, NoteManager::get_note_template_content (title));
          
      // Select the initial text
      NoteBuffer::Ptr buffer = static_pointer_cast<Note>(note)->get_buffer();
      buffer->select_note_body();

      // Flag this as a template note
      Tag::Ptr templ_tag = template_tag();
      note->add_tag(templ_tag);

      // Add on the notebook system tag so Tomboy
      // will persist the tag/notebook across sessions
      // if no other notes are added to the notebook.
      Tag::Ptr notebook_tag = ITagManager::obj().get_or_create_system_tag(NOTEBOOK_TAG_PREFIX + get_name());
      note->add_tag (notebook_tag);
        
      note->queue_save (CONTENT_CHANGED);
    }

    return static_pointer_cast<Note>(note);
  }

  Note::Ptr Notebook::create_notebook_note()
  {
    Glib::ustring temp_title;
    Note::Ptr note_template = get_template_note();

    temp_title = m_note_manager.get_unique_name(_("New Note"));
    NoteBase::Ptr note = m_note_manager.create_note_from_template(temp_title, note_template);

    // Add the notebook tag
    note->add_tag(m_tag);

    return static_pointer_cast<Note>(note);
  }

  /// <summary>
  /// Returns true when the specified note exists in the notebook
  /// </summary>
  /// <param name="note">
  /// A <see cref="Note"/>
  /// </param>
  /// <returns>
  /// A <see cref="System.Boolean"/>
  /// </returns>
  bool Notebook::contains_note(const Note::Ptr & note, bool include_system)
  {
    bool contains = note->contains_tag(m_tag);
    if(!contains || include_system) {
      return contains;
    }
    return !is_template_note(note);
  }

  bool Notebook::add_note(const Note::Ptr & note)
  {
    NotebookManager::obj().move_note_to_notebook(note, shared_from_this());
    return true;
  }

  Glib::ustring Notebook::normalize(const Glib::ustring & s)
  {
    return Glib::ustring(sharp::string_trim(s)).lowercase();
  }

}
}
