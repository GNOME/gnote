/*
 * gnote
 *
 * Copyright (C) 2013,2019,2022-2025 Aurimas Cernius
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

#include <gtkmm/columnview.h>

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
    sigc::signal<void(Note&)> signal_open_template_note;
  private:
    void on_selection_changed(guint, guint);
    bool on_notebooks_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
    void on_selected_notebook_changed(const Notebook&);
    void on_create_new_notebook();
    void on_rename_notebook();
    void rename_notebook(const Notebook& old_notebook, const Glib::ustring& new_name);
    void on_delete_notebook();
    void on_open_template_note();

    NoteManagerBase & m_note_manager;
    Gtk::Button m_new_button;
    Gtk::Button m_rename_button;
    Gtk::Button m_delete_button;
    Gtk::Button m_open_template_note;
    Gtk::ColumnView m_list;
  };

  }
}

#endif

