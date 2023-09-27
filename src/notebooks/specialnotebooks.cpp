/*
 * gnote
 *
 * Copyright (C) 2010-2014,2017,2019-2020,2022-2023 Aurimas Cernius
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

#include "iconmanager.hpp"
#include "ignote.hpp"
#include "notemanager.hpp"
#include "notebookmanager.hpp"
#include "specialnotebooks.hpp"


namespace gnote {
namespace notebooks {


Tag::Ptr SpecialNotebook::get_tag() const
{
  return Tag::Ptr();
}

Note::Ptr SpecialNotebook::get_template_note() const
{
  return std::static_pointer_cast<Note>(m_note_manager.get_or_create_template_note().shared_from_this());
}


AllNotesNotebook::AllNotesNotebook(NoteManagerBase & manager)
  : SpecialNotebook(manager, _("All"))
{
}

Glib::ustring AllNotesNotebook::get_normalized_name() const
{
  return "___NotebookManager___AllNotes__Notebook___";
}

bool AllNotesNotebook::contains_note(const Note::Ptr & note, bool include_system)
{
  if(include_system) {
    return true;
  }
  return !is_template_note(note);
}

bool AllNotesNotebook::add_note(const Note::Ptr &)
{
  return false;
}

Glib::ustring AllNotesNotebook::get_icon_name() const
{
  return IconManager::FILTER_NOTE_ALL;
}


UnfiledNotesNotebook::UnfiledNotesNotebook(NoteManagerBase & manager)
  : SpecialNotebook(manager, _("Unfiled"))
{
}

Glib::ustring UnfiledNotesNotebook::get_normalized_name() const
{
  return "___NotebookManager___UnfiledNotes__Notebook___";
}

bool UnfiledNotesNotebook::contains_note(const Note::Ptr & note, bool include_system)
{
  bool contains = m_note_manager.notebook_manager().get_notebook_from_note(note) == nullptr;
  if(!contains || include_system) {
    return contains;
  }
  return !is_template_note(note);
}

bool UnfiledNotesNotebook::add_note(const Note::Ptr & note)
{
  m_note_manager.notebook_manager().move_note_to_notebook(note, Notebook::Ptr());
  return true;
}

Glib::ustring UnfiledNotesNotebook::get_icon_name() const
{
  return IconManager::FILTER_NOTE_UNFILED;
}


PinnedNotesNotebook::PinnedNotesNotebook(NoteManagerBase & manager)
  : SpecialNotebook(manager, C_("notebook", "Important"))
{
}

Glib::ustring PinnedNotesNotebook::get_normalized_name() const
{
  return "___NotebookManager___PinnedNotes__Notebook___";
}

bool PinnedNotesNotebook::contains_note(const Note::Ptr & note, bool)
{
  return note->is_pinned();
}

bool PinnedNotesNotebook::add_note(const Note::Ptr & note)
{
  note->set_pinned(true);
  return true;
}

Glib::ustring PinnedNotesNotebook::get_icon_name() const
{
  return IconManager::PIN_DOWN;
}


ActiveNotesNotebook::ActiveNotesNotebook(NoteManagerBase & manager)
  : SpecialNotebook(manager, _("Active"))
{
  manager.signal_note_deleted
    .connect(sigc::mem_fun(*this, &ActiveNotesNotebook::on_note_deleted));
}

Glib::ustring ActiveNotesNotebook::get_normalized_name() const
{
  return "___NotebookManager___ActiveNotes__Notebook___";
}

bool ActiveNotesNotebook::contains_note(const Note::Ptr & note, bool include_system)
{
  bool contains = m_notes.find(note) != m_notes.end();
  if(!contains || include_system) {
    return contains;
  }
  return !is_template_note(note);
}

bool ActiveNotesNotebook::add_note(const Note::Ptr & note)
{
  if(m_notes.insert(note).second) {
    signal_size_changed();
  }

  return true;
}

Glib::ustring ActiveNotesNotebook::get_icon_name() const
{
  return IconManager::ACTIVE_NOTES;
}

void ActiveNotesNotebook::on_note_deleted(const NoteBase::Ptr & note)
{
  std::set<Note::Ptr>::iterator iter = m_notes.find(std::static_pointer_cast<Note>(note));
  if(iter != m_notes.end()) {
    m_notes.erase(iter);
    signal_size_changed();
  }
}

bool ActiveNotesNotebook::empty()
{
  if(m_notes.size() == 0) {
    return true;
  }

  // ignore template notes
  Tag::Ptr templ_tag = template_tag();
  for(const Note::Ptr & note : m_notes) {
    if(!note->contains_tag(templ_tag)) {
      return false;
    }
  }

  return true;
}


}
}

