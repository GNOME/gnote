


#ifndef _NOTEBOOK_MANAGER_HPP__
#define _NOTEBOOK_MANAGER_HPP__

#include <gtkmm/treemodel.h>

#include "notebooks/notebook.hpp"

namespace gnote {
  namespace notebooks {


class NotebookManager
{
public:
  class ColumnRecord
    : public Gtk::TreeModelColumnRecord
  {
  public:
    ColumnRecord()
      { add(m_col1); }
    Gtk::TreeModelColumn<Notebook::Ptr> m_col1;
  };
  
  static const ColumnRecord & get_column_record()
    { static ColumnRecord record;
      return record;
    }
  static Glib::RefPtr<Gtk::TreeModel> get_notebooks_with_special_items()
    { return Glib::RefPtr<Gtk::TreeModel>(); }
  static Notebook::Ptr get_notebook_from_note(const Note::Ptr &)
    {
      return Notebook::Ptr();
    }
  static void prompt_create_new_notebook(Gtk::Window *)
    {}
  static void prompt_delete_notebook(Gtk::Window *, const notebooks::Notebook::Ptr &)
    {}

};

  }
}

#endif
