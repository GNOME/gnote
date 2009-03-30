


#ifndef __NOTEBOOK_NEW_NOTEMNUITEM_HPP__
#define __NOTEBOOK_NEW_NOTEMNUITEM_HPP__


#include <gtkmm/imagemenuitem.h>

#include "notebooks/notebook.hpp"

namespace gnote {
  namespace notebooks {


class NotebookNewNoteMenuItem
  : public Gtk::ImageMenuItem
{
public:
  NotebookNewNoteMenuItem(const Notebook::Ptr &);
  void on_activated();
  Notebook::Ptr get_notebook() const
    {
      return m_notebook;
    }
  // the menu item is comparable.
//  bool operator==(const NotebookNewNoteMenuItem &);
//  bool operator<(const NotebookNewNoteMenuItem &);
//  bool operator>(const NotebookNewNoteMenuItem &);
private:
  Notebook::Ptr m_notebook;
};


  }
}

#endif
