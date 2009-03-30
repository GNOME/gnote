
#include <boost/format.hpp>

#include <glibmm/i18n.h>

#include <gtkmm/table.h>
#include <gtkmm/stock.h>

#include "sharp/string.hpp"
#include "notebooks/createnotebookdialog.hpp"
#include "notebooks/notebookmanager.hpp"
#include "utils.hpp"

namespace gnote {
  namespace notebooks {

    CreateNotebookDialog::CreateNotebookDialog(Gtk::Window *parent, GtkDialogFlags f)
      : utils::HIGMessageDialog(parent, f, Gtk::MESSAGE_INFO, Gtk::BUTTONS_NONE,
                                _("Create a new notebook"),
                                _("Type the name of the notebook you'd like to create."))
    {
      m_newNotebookIcon = utils::get_icon ("notebook-new", 16);
			m_newNotebookIconDialog = utils::get_icon ("notebook-new", 48);
//      set_icon_list(m_newNotebookIconDialog);

      Gtk::Table *table = manage(new Gtk::Table (2, 2, false));
			
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
        str(boost::format("<span foreground='red' style='italic'>%1%</span>")
            % _("Name already taken")));
			
			table->attach (*label, 0, 1, 0, 1);
			table->attach (m_nameEntry, 1, 2, 0, 1);
			table->attach (m_errorLabel, 1, 2, 1, 2);
			table->show ();
			
			set_extra_widget(table);
			
			add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL, false);
			add_button (m_newNotebookIcon,
                  // Translation note: This is the Create button in the Create
                  // New Note Dialog.
                  _("C_reate"), Gtk::RESPONSE_OK, true);
			
			// Only let the Ok response be sensitive when
			// there's something in nameEntry
			set_response_sensitive (Gtk::RESPONSE_OK, false);
			m_errorLabel.hide ();

    }


    std::string CreateNotebookDialog::get_notebook_name()
    {
      return sharp::string_trim(m_nameEntry.get_text());
    }


    void CreateNotebookDialog::set_notebook_name(const std::string & value)
    {
      m_nameEntry.set_text(sharp::string_trim(value));
    }


    void CreateNotebookDialog::on_name_entry_changed()
    {
      bool nameTaken = false;
			if (NotebookManager::instance().notebook_exists (get_notebook_name())) {
				m_errorLabel.show ();
				nameTaken = true;
			} 
      else {
				m_errorLabel.hide ();
			}
			
			set_response_sensitive (Gtk::RESPONSE_OK,
        (get_notebook_name().empty() || nameTaken) ? false : true);

    }

  }
}
