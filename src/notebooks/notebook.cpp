/*
 * gnote
 *
 * Copyright (C) 2010-2013 Aurimas Cernius
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



#include <boost/format.hpp>
#include <glibmm/i18n.h>

#include "sharp/string.hpp"
#include "iconmanager.hpp"
#include "notemanager.hpp"
#include "notebooks/notebook.hpp"
#include "notebooks/notebookmanager.hpp"
#include "itagmanager.hpp"

namespace gnote {
namespace notebooks {


  const char * Notebook::NOTEBOOK_TAG_PREFIX = "notebook:";

  /// <summary>
  /// Construct a new Notebook with a given name
  /// </summary>
  /// <param name="name">
  /// A <see cref="System.String"/>.  This is the name that will be used
  /// to identify the notebook.
  /// </param>
  Notebook::Notebook(NoteManager & manager, const std::string & name, bool is_special)
    : m_note_manager(manager)
  {
    // is special assume the name as is, and we don't want a tag.
    if(is_special) {
      m_name = name;
    }
    else {
      set_name(name);
      m_tag = ITagManager::obj().get_or_create_system_tag(
        std::string(NOTEBOOK_TAG_PREFIX) + name);
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
    std::string systemNotebookPrefix = std::string(Tag::SYSTEM_TAG_PREFIX)
      + NOTEBOOK_TAG_PREFIX;
    std::string notebookName = sharp::string_substring(notebookTag->name(), 
                                                       systemNotebookPrefix.length());
    set_name(notebookName);
    m_tag = notebookTag;
  }

  void Notebook::set_name(const std::string & value)
  {
    std::string trimmedName = value;
    if(!trimmedName.empty()) {
      m_name = trimmedName;
      m_normalized_name = sharp::string_to_lower(trimmedName);

      // The templateNoteTite should show the name of the
      // notebook.  For example, if the name of the notebooks
      // "Meetings", the templateNoteTitle should be "Meetings
      // Notebook Template".  Translators should place the
      // name of the notebook accordingly using "%1%".
      std::string format = _("%1% Notebook Template");
      m_default_template_note_title = str(boost::format(format) % m_name);
    }
  }


  std::string Notebook::get_normalized_name() const
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
    Tag::Ptr template_tag = ITagManager::obj().get_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
    Tag::Ptr notebook_tag = ITagManager::obj().get_system_tag(NOTEBOOK_TAG_PREFIX + get_name());
    if(!template_tag || !notebook_tag) {
      return note;
    }
    std::list<Note*> notes;
    template_tag->get_notes(notes);
    for (std::list<Note*>::iterator iter = notes.begin(); iter != notes.end(); ++iter) {
      if ((*iter)->contains_tag (notebook_tag)) {
        note = (*iter)->shared_from_this();
        break;
      }
    }

    return note;
  }

  Note::Ptr Notebook::get_template_note() const
  {
    Note::Ptr note = find_template_note();

    if (!note) {
      std::string title = m_default_template_note_title;
      if(m_note_manager.find(title)) {
        std::list<Note*> tag_notes;
        m_tag->get_notes(tag_notes);
        title = m_note_manager.get_unique_name (title, tag_notes.size());
      }
      note = m_note_manager.create(title, NoteManager::get_note_template_content (title));
          
      // Select the initial text
      NoteBuffer::Ptr buffer = note->get_buffer();
      buffer->select_note_body();

      // Flag this as a template note
      Tag::Ptr template_tag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
      note->add_tag (template_tag);

      // Add on the notebook system tag so Tomboy
      // will persist the tag/notebook across sessions
      // if no other notes are added to the notebook.
      Tag::Ptr notebook_tag = ITagManager::obj().get_or_create_system_tag(NOTEBOOK_TAG_PREFIX + get_name());
      note->add_tag (notebook_tag);
        
      note->queue_save (CONTENT_CHANGED);
    }

    return note;
  }

  Note::Ptr Notebook::create_notebook_note()
  {
    std::string temp_title;
    Note::Ptr note_template = get_template_note();

    temp_title = m_note_manager.get_unique_name(_("New Note"), m_note_manager.get_notes().size());
    Note::Ptr note = m_note_manager.create_note_from_template(temp_title, note_template);

    // Add the notebook tag
    note->add_tag(m_tag);

    return note;
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
  bool Notebook::contains_note(const Note::Ptr & note)
  {
    return note->contains_tag (m_tag);
  }

  bool Notebook::add_note(const Note::Ptr & note)
  {
    NotebookManager::obj().move_note_to_notebook(note, shared_from_this());
    return true;
  }

  Glib::RefPtr<Gdk::Pixbuf> Notebook::get_icon()
  {
    return IconManager::obj().get_icon(IconManager::NOTEBOOK, 22);
  }

  std::string Notebook::normalize(const std::string & s)
  {
    return sharp::string_to_lower(sharp::string_trim(s));
  }

  Tag::Ptr SpecialNotebook::get_tag() const
  {
    return Tag::Ptr();
  }

  Note::Ptr SpecialNotebook::get_template_note() const
  {
    return m_note_manager.get_or_create_template_note();
  }


  AllNotesNotebook::AllNotesNotebook(NoteManager & manager)
    : SpecialNotebook(manager, _("All Notes"))
  {
  }


  std::string AllNotesNotebook::get_normalized_name() const
  {
    return "___NotebookManager___AllNotes__Notebook___";
  }


  bool AllNotesNotebook::contains_note(const Note::Ptr &)
  {
    return true;
  }


  bool AllNotesNotebook::add_note(const Note::Ptr &)
  {
    return false;
  }

  Glib::RefPtr<Gdk::Pixbuf> AllNotesNotebook::get_icon()
  {
    return IconManager::obj().get_icon(IconManager::FILTER_NOTE_ALL, 22);
  }


  UnfiledNotesNotebook::UnfiledNotesNotebook(NoteManager & manager)
    : SpecialNotebook(manager, _("Unfiled Notes"))
  {
  }

  std::string UnfiledNotesNotebook::get_normalized_name() const
  {
    return "___NotebookManager___UnfiledNotes__Notebook___";
  }


  bool UnfiledNotesNotebook::contains_note(const Note::Ptr & note)
  {
    return !notebooks::NotebookManager::obj().get_notebook_from_note(note);
  }


  bool UnfiledNotesNotebook::add_note(const Note::Ptr & note)
  {
    NotebookManager::obj().move_note_to_notebook(note, Notebook::Ptr());
    return true;
  }

  Glib::RefPtr<Gdk::Pixbuf> UnfiledNotesNotebook::get_icon()
  {
    return IconManager::obj().get_icon(IconManager::FILTER_NOTE_UNFILED, 22);
  }


  PinnedNotesNotebook::PinnedNotesNotebook(NoteManager & manager)
    : SpecialNotebook(manager, _("Pinned Notes"))
  {
  }

  std::string PinnedNotesNotebook::get_normalized_name() const
  {
    return "___NotebookManager___PinnedNotes__Notebook___";
  }


  bool PinnedNotesNotebook::contains_note(const Note::Ptr & note)
  {
    return note->is_pinned();
  }

  bool PinnedNotesNotebook::add_note(const Note::Ptr & note)
  {
    note->set_pinned(true);
    return true;
  }

  Glib::RefPtr<Gdk::Pixbuf> PinnedNotesNotebook::get_icon()
  {
    return IconManager::obj().get_icon(IconManager::PIN_DOWN, 22);
  }


  ActiveNotesNotebook::ActiveNotesNotebook(NoteManager & manager)
    : SpecialNotebook(manager, _("Active Notes"))
  {
    manager.signal_note_deleted
      .connect(sigc::mem_fun(*this, &ActiveNotesNotebook::on_note_deleted));
  }

  std::string ActiveNotesNotebook::get_normalized_name() const
  {
    return "___NotebookManager___ActiveNotes__Notebook___";
  }

  bool ActiveNotesNotebook::contains_note(const Note::Ptr & note)
  {
    return m_notes.find(note) != m_notes.end();
  }

  bool ActiveNotesNotebook::add_note(const Note::Ptr & note)
  {
    if(m_notes.insert(note).second) {
      signal_size_changed();
    }

    return true;
  }

  Glib::RefPtr<Gdk::Pixbuf> ActiveNotesNotebook::get_icon()
  {
    return IconManager::obj().get_icon(IconManager::NOTE, 22);
  }

  void ActiveNotesNotebook::on_note_deleted(const Note::Ptr & note)
  {
    std::set<Note::Ptr>::iterator iter = m_notes.find(note);
    if(iter != m_notes.end()) {
      m_notes.erase(iter);
      signal_size_changed();
    }
  }

}
}
