

#include <boost/format.hpp>
#include <glibmm/i18n.h>
#include <gtkmm/image.h>

#include "sharp/string.hpp"
#include "gnote.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "notebooks/notebookmenuitem.hpp"
#include "notebooks/notebooknewnotemenuitem.hpp"

namespace gnote {
  namespace notebooks {

    NotebookNewNoteMenuItem::NotebookNewNoteMenuItem(const Notebook::Ptr & notebook)
      : Gtk::ImageMenuItem(str(boost::format(_("New \"%1%\" Note")) % notebook->get_name()))
      , m_notebook(notebook)
    {
      set_image(*manage(new Gtk::Image(utils::get_icon("note-new", 16))));
      signal_activate().connect(sigc::mem_fun(*this, &NotebookNewNoteMenuItem::on_activated));
    }


    
    void NotebookNewNoteMenuItem::on_activated()
    {
      if (!m_notebook) {
				return;
      }
			
			// Look for the template note and create a new note
      Note::Ptr templateNote = m_notebook->get_template_note ();
      Note::Ptr note;
			
			NoteManager & noteManager(Gnote::default_note_manager());
			note = noteManager.create ();
			if (templateNote) {
				// Use the body from the template note
        std::string xmlContent = sharp::string_replace_all(templateNote->xml_content(), 
                                                           templateNote->get_title(), 
                                                           note->get_title());
				note->set_xml_content(xmlContent);
			}
			
			note->add_tag (m_notebook->get_tag());
			note->get_window()->show ();
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
