

#include <glibmm/i18n.h>

#include "notebooks/notebookmenuitem.hpp"
#include "notebooks/notebookmanager.hpp"



namespace gnote {
  namespace notebooks {

    NotebookMenuItem::NotebookMenuItem(Gtk::RadioButtonGroup & group, 
                                       const Note::Ptr & note, const Notebook::Ptr & notebook)
      : Gtk::RadioMenuItem(group, notebook ? notebook->get_name() : _("No notebook"))
      , m_note(note)
      , m_notebook(notebook)
    {
     	if (!notebook) {
				// This is for the "No notebook" menu item
				
				// Check to see if the specified note belongs
				// to a notebook.  If so, don't activate the
				// radio button.
				if (!NotebookManager::instance().get_notebook_from_note (note)) {
					set_active(true);
        }
			} 
      else if (notebook->contains_note (note)) {
				set_active(true);
			}
			
      signal_activate().connect(sigc::mem_fun(*this, &NotebookMenuItem::on_activated));
    }


    void NotebookMenuItem::on_activated()
    {
      if(!m_note) {
        return;
      }

      NotebookManager::instance().move_note_to_notebook(m_note, m_notebook);
    }

    // the menu item is comparable.
    bool NotebookMenuItem::operator==(const NotebookMenuItem & rhs)
    {
      return m_notebook->get_name() == rhs.m_notebook->get_name();
    }


    bool NotebookMenuItem::operator<(const NotebookMenuItem & rhs)
    {
      return m_notebook->get_name() < rhs.m_notebook->get_name();
    }


    bool NotebookMenuItem::operator>(const NotebookMenuItem & rhs)
    {
      return m_notebook->get_name() > rhs.m_notebook->get_name();
    }

  }
}
