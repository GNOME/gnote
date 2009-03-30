

#ifndef _NOTEBOOKS_TREEVIEW_HPP_
#define _NOTEBOOKS_TREEVIEW_HPP_

#include <gtkmm/treeview.h>

namespace gnote {

  class NoteManager;

  namespace notebooks {

  class NotebooksTreeView
    : public Gtk::TreeView
  {
  public:
    NotebooksTreeView(const Glib::RefPtr<Gtk::TreeModel> & model);

  protected:
    virtual void on_drag_data_received( const Glib::RefPtr<Gdk::DragContext> & context,
                                        int x, int y,
                                        const Gtk::SelectionData & selection_data,
                                        guint info, guint time);
    virtual bool on_drag_motion(const Glib::RefPtr<Gdk::DragContext> & context,
                                int x, int y, guint time);
    virtual void on_drag_leave(const Glib::RefPtr<Gdk::DragContext> & context, guint time);
  private:
    NoteManager & m_note_manager;
  };

  }
}

#endif

