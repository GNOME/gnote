/*
 * gnote
 *
 * Copyright (C) 2010,2012-2013,2017 Aurimas Cernius
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
#include <gtkmm/image.h>

#include "sharp/string.hpp"
#include "iconmanager.hpp"
#include "mainwindow.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "notebooks/notebookmenuitem.hpp"
#include "notebooks/notebooknewnotemenuitem.hpp"

namespace gnote {
  namespace notebooks {

    NotebookNewNoteMenuItem::NotebookNewNoteMenuItem(const Notebook::Ptr & notebook)
      // TRANSLATORS: %1: format placeholder for the notebook name
      : Gtk::ImageMenuItem(Glib::ustring::compose(_("New \"%1\" Note"), notebook->get_name()))
      , m_notebook(notebook)
    {
      set_image(*manage(new Gtk::Image(IconManager::obj().get_icon(IconManager::NOTE_NEW, 16))));
      signal_activate().connect(sigc::mem_fun(*this, &NotebookNewNoteMenuItem::on_activated));
    }


    
    void NotebookNewNoteMenuItem::on_activated()
    {
      if (!m_notebook) {
        return;
      }
      
      // Look for the template note and create a new note
      Note::Ptr note = m_notebook->create_notebook_note ();
      MainWindow::present_in_new_window(note,
        Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE)->get_boolean(
          Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE));
    }


    bool NotebookNewNoteMenuItem::operator==(const NotebookMenuItem & rhs)
    {
      return m_notebook->get_name() == rhs.get_notebook()->get_name();
    }


    bool NotebookNewNoteMenuItem::operator<(const NotebookMenuItem & rhs)
    {
      return m_notebook->get_name() < rhs.get_notebook()->get_name();
    }


    bool NotebookNewNoteMenuItem::operator>(const NotebookMenuItem & rhs)
    {
      return m_notebook->get_name() > rhs.get_notebook()->get_name();
    }

  }
}
