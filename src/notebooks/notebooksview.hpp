/*
 * gnote
 *
 * Copyright (C) 2013,2019,2022-2024 Aurimas Cernius
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



#ifndef _NOTEBOOKSVIEW_HPP_
#define _NOTEBOOKSVIEW_HPP_

#include <gtkmm/listview.h>

namespace gnote {

  namespace notebooks {

  class NotebooksView
    : public Gtk::Box
  {
  public:
    NotebooksView(NoteManagerBase & manager, const Glib::RefPtr<Gio::ListModel> & model);

    Notebook::ORef get_selected_notebook() const;
    void select_all_notes_notebook();
    void select_notebook(Notebook&);
    void set_notebooks(const Glib::RefPtr<Gio::ListModel> & model);

    sigc::signal<void(const Notebook &)> signal_selected_notebook_changed;
  private:
    void on_selection_changed(guint, guint);

    NoteManagerBase & m_note_manager;
    Gtk::ListView m_list;
  };

  }
}

#endif

