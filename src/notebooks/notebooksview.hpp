/*
 * gnote
 *
 * Copyright (C) 2013,2019,2022-2023 Aurimas Cernius
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

#include <gtkmm/treeview.h>

namespace gnote {

  namespace notebooks {

  class NotebooksView
    : public Gtk::TreeView
  {
  public:
    NotebooksView(NoteManagerBase & manager, const Glib::RefPtr<Gtk::TreeModel> & model);

    Notebook::Ptr get_selected_notebook() const;
    void select_all_notes_notebook();
  private:
    bool on_drag_data_received(const Glib::ValueBase & value, double x, double y);
    NoteManagerBase & m_note_manager;
  };

  }
}

#endif

