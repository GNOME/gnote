


#ifndef __NOTEBOOK_MENUITEM_HPP__
#define __NOTEBOOK_MENUITEM_HPP__

#include <gtkmm/radiomenuitem.h>

#include "note.hpp"
#include "notebooks/notebook.hpp"


namespace gnote {
  namespace notebooks {

    class NotebookMenuItem
      : public Gtk::RadioMenuItem
    {
    public:
      NotebookMenuItem(Gtk::RadioButtonGroup & group, const Note::Ptr &, const Notebook::Ptr &);

      const Note::Ptr & get_note() const
        {
          return m_note;
        }
      const Notebook::Ptr & get_notebook() const
        {
          return m_notebook;
        }
      // the menu item is comparable.
      bool operator==(const NotebookMenuItem &);
      bool operator<(const NotebookMenuItem &);
      bool operator>(const NotebookMenuItem &);
    private:
      void on_activated();

      Note::Ptr m_note;
      Notebook::Ptr m_notebook;
    };

  }
}

#endif
