/*
 * gnote
 *
 * Copyright (C) 2012-2015,2017,2019,2022-2023 Aurimas Cernius
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
#include <gtkmm/liststore.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treemodelsort.h>
#include <gtkmm/treemodelfilter.h>

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

  Glib::RefPtr<Gtk::TreeModel> get_notebooks()
    { return m_filteredNotebooks; }
  /// <summary>
  /// A Gtk.TreeModel that contains all of the items in the
  /// NotebookManager TreeStore including SpecialNotebooks
  /// which are used in the "Search All Notes" window.
  /// </summary>
  /// <param name="notebookName">
  /// A <see cref="System.String"/>
  /// </param>
  /// <returns>
  /// A <see cref="Notebook"/>
  /// </returns>
  Glib::RefPtr<Gtk::TreeModel> get_notebooks_with_special_items()
    { return m_notebooks_to_display; }

  Notebook::ORef get_notebook(const Glib::ustring & notebookName) const;
  bool notebook_exists(const Glib::ustring & notebookName) const;
  Notebook & get_or_create_notebook(const Glib::ustring &);
  bool add_notebook(Notebook::Ptr &&);
  void delete_notebook(Notebook &);
  bool get_notebook_iter(const Notebook::Ptr &, Gtk::TreeIter<Gtk::TreeRow> & );
  Notebook::ORef get_notebook_from_note(const NoteBase&);
  Notebook::ORef get_notebook_from_tag(const Tag::Ptr &);
  static bool is_notebook_tag(const Tag::Ptr &);
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
  static int compare_notebooks_sort_func(const Gtk::TreeIter<Gtk::TreeConstRow> &, const Gtk::TreeIter<Gtk::TreeConstRow> &);
  static void on_create_notebook_response(IGnote & g, CreateNotebookDialog & dialog, int respons, const std::vector<Glib::ustring> & notes_to_add,
    std::function<void(Notebook::ORef)> on_complete);
  void load_notebooks();
  bool filter_notebooks_to_display(const Gtk::TreeIter<Gtk::TreeConstRow> &);
  void on_active_notes_size_changed();
  static bool filter_notebooks(const Gtk::TreeIter<Gtk::TreeConstRow> &);

  class ColumnRecord
    : public Gtk::TreeModelColumnRecord
  {
  public:
    ColumnRecord()
      { add(m_col1); }
    Gtk::TreeModelColumn<Notebook::Ptr> m_col1;
  };

  ColumnRecord                         m_column_types;
  Glib::RefPtr<Gtk::ListStore>         m_notebooks;
  Glib::RefPtr<Gtk::TreeModelSort>     m_sortedNotebooks;
  Glib::RefPtr<Gtk::TreeModelFilter>   m_notebooks_to_display;
  Glib::RefPtr<Gtk::TreeModelFilter>   m_filteredNotebooks;
  // <summary>
  // The key for this dictionary is Notebook.Name.ToLower ().
  // </summary>
  std::map<Glib::ustring, Gtk::TreeIter<Gtk::TreeRow>> m_notebookMap;
  std::vector<Notebook::Ptr> m_all_notebooks;
  Notebook::Ptr                        m_active_notes;
  NoteManagerBase                    & m_note_manager;
};

}

}

#endif
