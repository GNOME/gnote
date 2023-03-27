/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017,2019,2021-2022 Aurimas Cernius
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

#include "sharp/string.hpp"
#include "notebooks/createnotebookdialog.hpp"
#include "notebooks/notebookmanager.hpp"
#include "iconmanager.hpp"
#include "ignote.hpp"
#include "utils.hpp"

namespace gnote {
  namespace notebooks {

    CreateNotebookDialog::CreateNotebookDialog(Gtk::Window *parent, GtkDialogFlags f, IGnote & g)
      : utils::HIGMessageDialog(parent, f, Gtk::MessageType::OTHER, Gtk::ButtonsType::NONE)
      , m_gnote(g)
    {
      set_title(_("Create Notebook"));
      Gtk::Grid *table = manage(new Gtk::Grid);
      table->set_orientation(Gtk::Orientation::HORIZONTAL);
      table->set_column_spacing(6);
      
      Gtk::Label *label = manage(new Gtk::Label (_("N_otebook name:"), true));
      label->property_xalign() = 0;
      label->show ();
      
      m_nameEntry.signal_changed().connect(
        sigc::mem_fun(*this, &CreateNotebookDialog::on_name_entry_changed));
      m_nameEntry.set_activates_default(true);
      m_nameEntry.show ();
      label->set_mnemonic_widget(m_nameEntry);
      
      m_errorLabel.property_xalign() = 0;
      m_errorLabel.set_markup(
        Glib::ustring::compose("<span foreground='red' style='italic'>%1</span>",
            _("Name already taken")));
      
      table->attach(*label, 0, 0, 1, 1);
      table->attach(m_nameEntry, 1, 0, 1, 1);
      table->attach(m_errorLabel, 1, 1, 1, 1);
      table->show ();
      
      set_extra_widget(table);
      
      add_button(_("_Cancel"), Gtk::ResponseType::CANCEL, false);
      // Translation note: This is the Create button in the Create
      // New Note Dialog.
      add_button(_("C_reate"), Gtk::ResponseType::OK, true);
      
      // Only let the Ok response be sensitive when
      // there's something in nameEntry
      set_response_sensitive(Gtk::ResponseType::OK, false);
      m_errorLabel.hide();
    }


    Glib::ustring CreateNotebookDialog::get_notebook_name()
    {
      return sharp::string_trim(m_nameEntry.get_text());
    }


    void CreateNotebookDialog::set_notebook_name(const Glib::ustring & value)
    {
      m_nameEntry.set_text(sharp::string_trim(value));
    }


    void CreateNotebookDialog::on_name_entry_changed()
    {
      bool nameTaken = false;
      if(m_gnote.notebook_manager().notebook_exists(get_notebook_name())) {
        m_errorLabel.show ();
        nameTaken = true;
      } 
      else {
        m_errorLabel.hide ();
      }
      
      set_response_sensitive(Gtk::ResponseType::OK, (get_notebook_name().empty() || nameTaken) ? false : true);
    }

  }
}
