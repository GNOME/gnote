/*
 * gnote
 *
 * Copyright (C) 2013-2014,2017 Aurimas Cernius
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

#include "notebooks/specialnotebooks.hpp"
#include "notebooks/notebookmanager.hpp"
#include "iconmanager.hpp"
#include "specialnotesapplicationaddin.hpp"

using gnote::Note;
using gnote::notebooks::Notebook;


namespace {
  class SpecialNotesNotebook
    : public gnote::notebooks::SpecialNotebook
  {
  public:
    SpecialNotesNotebook(gnote::NoteManager & nm)
      : SpecialNotebook(nm, _("Special Notes"))
      {}

    virtual Glib::ustring get_normalized_name() const override
      {
        return "___SpecialNotesAddin___SpecialNotes__Notebook___";
      }

    virtual bool contains_note(const Note::Ptr & note, bool = false) override
      {
        return is_template_note(note);
      }

    virtual bool add_note(const Note::Ptr &) override
      {
        return false;
      }

    virtual Glib::RefPtr<Gdk::Pixbuf> get_icon() override
      {
        return gnote::IconManager::obj().get_icon(gnote::IconManager::SPECIAL_NOTES, 22);
      }
  };
}


namespace specialnotes {

DECLARE_MODULE(SpecialNotesModule);


SpecialNotesModule::SpecialNotesModule()
{
  ADD_INTERFACE_IMPL(SpecialNotesApplicationAddin);
}


const char *SpecialNotesApplicationAddin::IFACE_NAME = "gnote::ApplicationAddin";

SpecialNotesApplicationAddin::SpecialNotesApplicationAddin()
  : m_initialized(false)
{
}

SpecialNotesApplicationAddin::~SpecialNotesApplicationAddin()
{
}

void SpecialNotesApplicationAddin::initialize()
{
  if(!m_initialized) {
    m_initialized = true;

    m_notebook = Notebook::Ptr(new SpecialNotesNotebook(note_manager()));
    gnote::notebooks::NotebookManager::obj().add_notebook(m_notebook);
  }
}

void SpecialNotesApplicationAddin::shutdown()
{
  if(m_notebook != NULL) {
    gnote::notebooks::NotebookManager::obj().delete_notebook(m_notebook);
    m_notebook.reset();
    m_initialized = false;
  }
}

bool SpecialNotesApplicationAddin::initialized()
{
  return m_initialized;
}

}

