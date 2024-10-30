/*
 * gnote
 *
 * Copyright (C) 2011-2015,2017,2019,2021-2024 Aurimas Cernius
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



#include <glibmm/i18n.h>
#include <glibmm/stringutils.h>
#include <gtkmm/treeiter.h>
#include <gtkmm/treemodel.h>

#include "sharp/string.hpp"
#include "notebooks/notebookapplicationaddin.hpp"
#include "notebooks/notebookmanager.hpp"
#include "notebooks/notebook.hpp"
#include "iactionmanager.hpp"
#include "ignote.hpp"
#include "debug.hpp"
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
      IActionManager & am(ignote().action_manager());

      NoteManager & nm(note_manager());

      nm.for_each([this](NoteBase & note) {
        note.signal_tag_added.connect(sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_added));
        note.signal_tag_removed.connect(sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_removed));
      });

      nm.signal_note_added.connect(
        sigc::mem_fun(*this, &NotebookApplicationAddin::on_note_added));

      am.add_app_action("new-notebook");
      am.get_app_action("new-notebook")->signal_activate().connect(
        sigc::mem_fun(*this, &NotebookApplicationAddin::on_new_notebook_action));
      am.add_app_menu_item(APP_SECTION_NEW, 300, _("New Note_book..."), "app.new-notebook");
        
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
      auto & gnote = ignote();
      gnote.notebook_manager().prompt_create_new_notebook(gnote, gnote.get_main_window());
    }


    void NotebookApplicationAddin::on_tag_added(const NoteBase & note, const Tag &tag)
    {
      NotebookManager & manager = ignote().notebook_manager();

      Glib::ustring megaPrefix(Tag::SYSTEM_TAG_PREFIX);
      megaPrefix += Notebook::NOTEBOOK_TAG_PREFIX;
      if(!tag.is_system() || !Glib::str_has_prefix(tag.name(), megaPrefix)) {
        return;
      }

      Glib::ustring notebookName = sharp::string_substring(tag.name(), megaPrefix.size());

      auto & notebook = manager.get_or_create_notebook(notebookName);

      manager.signal_note_added_to_notebook(static_cast<const Note&>(note), notebook);
    }



    void NotebookApplicationAddin::on_tag_removed(const NoteBase & note,
                                                  const Glib::ustring & normalizedTagName)
    {
      Glib::ustring megaPrefix(Tag::SYSTEM_TAG_PREFIX);
      megaPrefix += Notebook::NOTEBOOK_TAG_PREFIX;

      if (!Glib::str_has_prefix(normalizedTagName, megaPrefix)) {
        return;
      }

      Glib::ustring normalizedNotebookName =
        sharp::string_substring(normalizedTagName, megaPrefix.size());

      NotebookManager & manager = ignote().notebook_manager();
      auto nb = manager.get_notebook(normalizedNotebookName);
      if(!nb) {
        return;
      }

      Notebook & notebook = nb.value();
      manager.signal_note_removed_from_notebook(static_cast<const Note&>(note), notebook);
    }

    void NotebookApplicationAddin::on_note_added(NoteBase & note)
    {
        note.signal_tag_added.connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_added));
        note.signal_tag_removed.connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_removed));
    }

  }
}
