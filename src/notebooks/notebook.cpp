/*
 * gnote
 *
 * Copyright (C) 2010-2014,2017,2019,2022-2024 Aurimas Cernius
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
#include "debug.hpp"
#include "notemanager.hpp"
#include "notebooks/notebook.hpp"
#include "notebooks/notebookmanager.hpp"

namespace gnote {
namespace notebooks {


  const char * Notebook::NOTEBOOK_TAG_PREFIX = "notebook:";
  Glib::ustring Notebook::s_template_tag;

  Tag::ORef Notebook::template_tag() const
  {
    auto &tag_manager = m_note_manager.tag_manager();
    if(s_template_tag.empty()) {
      auto &tag = tag_manager.get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
      s_template_tag = tag.normalized_name();
      return tag;
    }

    if(auto tag = tag_manager.get_tag(s_template_tag)) {
      return *tag;
    }

    return Tag::ORef();
  }

  bool Notebook::is_template_note(const Note & note)
  {
    if(auto tag = template_tag()) {
      return note.contains_tag(tag.value());
    }
    return false;
  }

  Notebook::Ptr Notebook::create(NoteManagerBase& manager, const Glib::ustring& name, bool is_special)
  {
    return Glib::make_refptr_for_instance(new Notebook(manager, name, is_special));
  }

  Notebook::Ptr Notebook::create(NoteManagerBase& manager, const Tag &tag)
  {
    return Glib::make_refptr_for_instance(new Notebook(manager, tag));
  }

  Notebook::Notebook(NoteManagerBase & manager, const Glib::ustring & name, bool is_special)
    : m_note_manager(manager)
  {
    // is special assume the name as is, and we don't want a tag.
    if(is_special) {
      m_name = name;
    }
    else {
      set_name(name);
      m_tag = manager.tag_manager().get_or_create_system_tag(
        Glib::ustring(NOTEBOOK_TAG_PREFIX) + name).normalized_name();
    }
  }

  Notebook::Notebook(NoteManagerBase & manager, const Tag &notebook_tag)
    : m_note_manager(manager)
  {
  // Parse the notebook name from the tag name
    Glib::ustring systemNotebookPrefix = Glib::ustring(Tag::SYSTEM_TAG_PREFIX)
      + NOTEBOOK_TAG_PREFIX;
    Glib::ustring notebookName = sharp::string_substring(notebook_tag.name(), systemNotebookPrefix.length());
    set_name(notebookName);
    m_tag = notebook_tag.normalized_name();
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


  Tag::ORef Notebook::get_tag() const
  {
    if(auto tag = m_note_manager.tag_manager().get_tag(m_tag)) {
      return Tag::ORef(std::ref(*tag));
    }

    return Tag::ORef();
  }


  Note::ORef Notebook::find_template_note() const
  {
    auto template_tag = this->template_tag();
    if(!template_tag) {
      return Note::ORef();
    }
    auto notebook_tag = m_note_manager.tag_manager().get_system_tag(NOTEBOOK_TAG_PREFIX + get_name());
    if(!notebook_tag) {
      return Note::ORef();
    }
    Tag &templ_tag = template_tag.value();
    Tag &nb_tag = notebook_tag.value();

    for(NoteBase *n : templ_tag.get_notes()) {
      if(n->contains_tag(nb_tag)) {
        return Note::ORef(std::ref(*static_cast<Note*>(n)));
      }
    }

    return Note::ORef();
  }

  Note & Notebook::get_template_note() const
  {
    {
      auto note = find_template_note();
      if(note) {
        return note.value();
      }
    }

    Glib::ustring title = m_default_template_note_title;
    if(m_note_manager.find(title)) {
      title = m_note_manager.get_unique_name(title);
    }
    auto content = NoteManager::get_note_template_content(title);
    auto & note = m_note_manager.create(std::move(title), std::move(content));

    // Select the initial text
    NoteBuffer::Ptr buffer = static_cast<Note&>(note).get_buffer();
    buffer->select_note_body();

    // Flag this as a template note
    if(auto templ_tag = template_tag()) {
      note.add_tag(templ_tag.value());
    }
    else {
      ERR_OUT("No template tag available. This is a bug.");
    }

    // Add on the notebook system tag so Tomboy
    // will persist the tag/notebook across sessions
    // if no other notes are added to the notebook.
    note.add_tag(m_note_manager.tag_manager().get_or_create_system_tag(NOTEBOOK_TAG_PREFIX + get_name()));

    note.queue_save(CONTENT_CHANGED);

    return static_cast<Note&>(note);
  }

  Note & Notebook::create_notebook_note()
  {
    Glib::ustring temp_title;
    auto & note_template = get_template_note();

    temp_title = m_note_manager.get_unique_name(_("New Note"));
    auto & note = m_note_manager.create_note_from_template(std::move(temp_title), note_template);

    // Add the notebook tag
    note.add_tag(*get_tag());

    return static_cast<Note&>(note);
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
  bool Notebook::contains_note(const Note & note, bool include_system)
  {
    bool contains = false;
    if(auto tag = get_tag()) {
      contains = note.contains_tag(*tag);
    }
    if(!contains || include_system) {
      return contains;
    }
    return !is_template_note(note);
  }

  bool Notebook::add_note(Note & note)
  {
    m_note_manager.notebook_manager().move_note_to_notebook(note, *this);
    return true;
  }

  Glib::ustring Notebook::normalize(const Glib::ustring & s)
  {
    return Glib::ustring(sharp::string_trim(s)).lowercase();
  }

}
}
