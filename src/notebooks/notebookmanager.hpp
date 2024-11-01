/*
 * gnote
 *
 * Copyright (C) 2012-2015,2017,2019,2022-2024 Aurimas Cernius
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




#ifndef _NOTEBOOK_MANAGER_HPP__
#define _NOTEBOOK_MANAGER_HPP__

#include <sigc++/signal.h>

#include "notebooks/createnotebookdialog.hpp"
#include "notebooks/notebook.hpp"
#include "notebooks/specialnotebooks.hpp"
#include "note.hpp"
#include "tag.hpp"

namespace gnote {

class IGnote;


namespace notebooks {


class NotebookManager
{
public:
  typedef sigc::signal<void(const Note &, const Notebook &)> NotebookEventHandler;

  NotebookManager(NoteManagerBase &);
  void init();

  NoteManagerBase & note_manager() const
    {
      return m_note_manager;
    }

  template <typename Adder>
  void get_notebooks(Adder add, bool include_special = false) const
    {
      for(const auto& nb : m_all_notebooks) {
        if(!include_special && std::dynamic_pointer_cast<SpecialNotebook>(nb)) {
          continue;
        }
        add(nb);
      }
    }

  Notebook::ORef get_notebook(const Glib::ustring & notebookName) const;
  bool notebook_exists(const Glib::ustring & notebookName) const;
  Notebook & get_or_create_notebook(const Glib::ustring &);
  bool add_notebook(Notebook::Ptr &&);
  void delete_notebook(Notebook &);
  Notebook::ORef get_notebook_from_note(const NoteBase&);
  Notebook::ORef get_notebook_from_tag(const Tag&);
  static bool is_notebook_tag(const Tag&);
  void prompt_create_new_notebook(IGnote &, Gtk::Window &, std::function<void(Notebook::ORef)> on_complete = {});
  void prompt_create_new_notebook(IGnote &, Gtk::Window &, std::vector<NoteBase::Ref> && notes_to_add,
    std::function<void(Notebook::ORef)> on_complete = {});
  static void prompt_delete_notebook(IGnote &, Gtk::Window *, Notebook &);
  bool move_note_to_notebook(Note &, Notebook::ORef);

  Notebook & active_notes_notebook()
    {
      return *m_active_notes;
    }

  NotebookEventHandler signal_note_added_to_notebook;
  NotebookEventHandler signal_note_removed_from_notebook;
  sigc::signal<void()> signal_notebook_list_changed;
  sigc::signal<void(const Note &, bool)> signal_note_pin_status_changed;
private:
  static void on_create_notebook_response(IGnote & g, CreateNotebookDialog & dialog, int respons, const std::vector<Glib::ustring> & notes_to_add,
    std::function<void(Notebook::ORef)> on_complete);
  void load_notebooks();

  std::vector<Notebook::Ptr> m_all_notebooks;
  Notebook::Ptr                        m_active_notes;
  NoteManagerBase                    & m_note_manager;
};

}

}

#endif
