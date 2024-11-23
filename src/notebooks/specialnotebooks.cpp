/*
 * gnote
 *
 * Copyright (C) 2010-2014,2017,2019-2020,2022-2024 Aurimas Cernius
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


Tag::ORef SpecialNotebook::get_tag() const
{
  return Tag::ORef();
}

Note & SpecialNotebook::get_template_note() const
{
  return static_cast<Note&>(m_note_manager.get_or_create_template_note());
}


AllNotesNotebook::Ptr AllNotesNotebook::create(NoteManagerBase& manager)
{
  return Glib::make_refptr_for_instance(new AllNotesNotebook(manager));
}

AllNotesNotebook::AllNotesNotebook(NoteManagerBase & manager)
  : SpecialNotebook(manager, _("All"))
{
}

Glib::ustring AllNotesNotebook::get_normalized_name() const
{
  return "___NotebookManager___AllNotes__Notebook___";
}

bool AllNotesNotebook::contains_note(const Note & note, bool include_system)
{
  if(include_system) {
    return true;
  }
  return !is_template_note(note);
}

bool AllNotesNotebook::add_note(Note&)
{
  return false;
}

Glib::ustring AllNotesNotebook::get_icon_name() const
{
  return IconManager::FILTER_NOTE_ALL;
}


UnfiledNotesNotebook::Ptr UnfiledNotesNotebook::create(NoteManagerBase& manager)
{
  return Glib::make_refptr_for_instance(new UnfiledNotesNotebook(manager));
}

UnfiledNotesNotebook::UnfiledNotesNotebook(NoteManagerBase & manager)
  : SpecialNotebook(manager, _("Unfiled"))
{
}

Glib::ustring UnfiledNotesNotebook::get_normalized_name() const
{
  return "___NotebookManager___UnfiledNotes__Notebook___";
}

bool UnfiledNotesNotebook::contains_note(const Note & note, bool include_system)
{
  bool contains = !bool(m_note_manager.notebook_manager().get_notebook_from_note(note));
  if(!contains || include_system) {
    return contains;
  }
  return !is_template_note(note);
}

bool UnfiledNotesNotebook::add_note(Note& note)
{
  m_note_manager.notebook_manager().move_note_to_notebook(note, Notebook::ORef());
  return true;
}

Glib::ustring UnfiledNotesNotebook::get_icon_name() const
{
  return IconManager::FILTER_NOTE_UNFILED;
}


PinnedNotesNotebook::Ptr PinnedNotesNotebook::create(NoteManagerBase& manager)
{
  return Glib::make_refptr_for_instance(new PinnedNotesNotebook(manager));
}

PinnedNotesNotebook::PinnedNotesNotebook(NoteManagerBase & manager)
  : SpecialNotebook(manager, C_("notebook", "Important"))
{
}

Glib::ustring PinnedNotesNotebook::get_normalized_name() const
{
  return "___NotebookManager___PinnedNotes__Notebook___";
}

bool PinnedNotesNotebook::contains_note(const Note & note, bool)
{
  return note.is_pinned();
}

bool PinnedNotesNotebook::add_note(Note & note)
{
  note.set_pinned(true);
  return true;
}

Glib::ustring PinnedNotesNotebook::get_icon_name() const
{
  return IconManager::PIN_DOWN;
}


ActiveNotesNotebook::Ptr ActiveNotesNotebook::create(NoteManagerBase& manager)
{
  return Glib::make_refptr_for_instance(new ActiveNotesNotebook(manager));
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

bool ActiveNotesNotebook::contains_note(const Note & note, bool include_system)
{
  bool contains = m_notes.find(note.uri()) != m_notes.end();
  if(!contains || include_system) {
    return contains;
  }
  return !is_template_note(note);
}

bool ActiveNotesNotebook::add_note(Note & note)
{
  if(m_notes.insert(note.uri()).second) {
    m_note_manager.notebook_manager().signal_note_added_to_notebook(note, *this);
  }

  return true;
}

Glib::ustring ActiveNotesNotebook::get_icon_name() const
{
  return IconManager::ACTIVE_NOTES;
}

void ActiveNotesNotebook::on_note_deleted(NoteBase & note)
{
  auto iter = m_notes.find(note.uri());
  if(iter != m_notes.end()) {
    m_notes.erase(iter);
    m_note_manager.notebook_manager().signal_note_removed_from_notebook(static_cast<Note&>(note), *this);
  }
}

bool ActiveNotesNotebook::empty()
{
  if(m_notes.size() == 0) {
    return true;
  }

  // ignore template notes
  auto templ_tag = template_tag();
  if(!templ_tag) {
    return false;
  }
  Tag &tag = templ_tag.value();

  for(const auto & note_uri : m_notes) {
    if(auto note_ref = m_note_manager.find_by_uri(note_uri)) {
      const NoteBase & note = note_ref.value();
      if(!note.contains_tag(tag)) {
        return false;
      }
    }
  }

  return true;
}


}
}

