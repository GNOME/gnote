

#ifndef _NOTEBOOKS_TREEVIEW_HPP_
#define _NOTEBOOKS_TREEVIEW_HPP_

#include <gtkmm/treeview.h>

namespace gnote {
  namespace notebooks {

  class NotebooksTreeView
    : public Gtk::TreeView
  {
  public:
    NotebooksTreeView(const Glib::RefPtr<Gtk::TreeModel> & model)
      : Gtk::TreeView(model)
      {
      }
  };

  }
}

#endif

