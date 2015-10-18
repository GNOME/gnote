/*
 * gnote
 *
 * Copyright (C) 2011-2015 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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



#include <boost/bind.hpp>

#include <glibmm.h>
#include <glibmm/i18n.h>
#include <gtkmm/stock.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/treeiter.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/uimanager.h>

#include "sharp/string.hpp"
#include "notebooks/notebookapplicationaddin.hpp"
#include "notebooks/notebookmanager.hpp"
#include "notebooks/notebooknewnotemenuitem.hpp"
#include "notebooks/notebook.hpp"
#include "iactionmanager.hpp"
#include "debug.hpp"
#include "iconmanager.hpp"
#include "notemanager.hpp"

namespace gnote {
  namespace notebooks {

    ApplicationAddin * NotebookApplicationAddin::create()
    {
      return new NotebookApplicationAddin();
    }


    NotebookApplicationAddin::NotebookApplicationAddin()
      : m_initialized(false)
    {
    }



    void NotebookApplicationAddin::initialize ()
    {
      IActionManager & am(IActionManager::obj());

      NoteManager & nm(note_manager());

      FOREACH(const NoteBase::Ptr & note, nm.get_notes()) {
        note->signal_tag_added.connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_added));
        note->signal_tag_removed.connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_removed));
      }

      nm.signal_note_added.connect(
        sigc::mem_fun(*this, &NotebookApplicationAddin::on_note_added));
      nm.signal_note_deleted.connect(
        sigc::mem_fun(*this, &NotebookApplicationAddin::on_note_deleted));

      am.add_app_action("new-notebook");
      am.get_app_action("new-notebook")->signal_activate().connect(
        sigc::mem_fun(*this, &NotebookApplicationAddin::on_new_notebook_action));
      am.add_app_menu_item(IActionManager::APP_ACTION_NEW, 300, _("New Note_book..."), "app.new-notebook");
        
      m_initialized = true;
    }


    void NotebookApplicationAddin::shutdown ()
    {
      m_initialized = false;
    }

    bool NotebookApplicationAddin::initialized ()
    {
      return m_initialized;
    }

    void NotebookApplicationAddin::on_new_notebook_action(const Glib::VariantBase&)
    {
      NotebookManager::prompt_create_new_notebook (NULL);
    }


    void NotebookApplicationAddin::on_tag_added(const NoteBase & note, const Tag::Ptr& tag)
    {
      if (NotebookManager::obj().is_adding_notebook()) {
        return;
      }

      std::string megaPrefix(Tag::SYSTEM_TAG_PREFIX);
      megaPrefix += Notebook::NOTEBOOK_TAG_PREFIX;
      if (!tag->is_system() || !Glib::str_has_prefix(tag->name(), megaPrefix)) {
        return;
      }

      std::string notebookName =
        sharp::string_substring(tag->name(), megaPrefix.size());

      Notebook::Ptr notebook =
        NotebookManager::obj().get_or_create_notebook (notebookName);

      NotebookManager::obj().signal_note_added_to_notebook() (static_cast<const Note&>(note), notebook);
    }



    void NotebookApplicationAddin::on_tag_removed(const NoteBase::Ptr& note,
                                                  const std::string& normalizedTagName)
    {
      std::string megaPrefix(Tag::SYSTEM_TAG_PREFIX);
      megaPrefix += Notebook::NOTEBOOK_TAG_PREFIX;

      if (!Glib::str_has_prefix(normalizedTagName, megaPrefix)) {
        return;
      }

      std::string normalizedNotebookName =
        sharp::string_substring(normalizedTagName, megaPrefix.size());

      Notebook::Ptr notebook =
        NotebookManager::obj().get_notebook (normalizedNotebookName);
      if (!notebook) {
        return;
      }

      NotebookManager::obj().signal_note_removed_from_notebook() (*static_pointer_cast<Note>(note), notebook);
    }

    void NotebookApplicationAddin::on_note_added(const NoteBase::Ptr & note)
    {
        note->signal_tag_added.connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_added));
        note->signal_tag_removed.connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_removed));
    }


    void NotebookApplicationAddin::on_note_deleted(const NoteBase::Ptr &)
    {
      // remove the signal to the note...
    }


  }
}
