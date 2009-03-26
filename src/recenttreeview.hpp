


#ifndef __RECENTTREEVIEW_HPP_
#define __RECENTTREEVIEW_HPP_

namespace gnote {

class RecentTreeView
  : public Gtk::TreeView
{
public:
  RecentTreeView()
    : Gtk::TreeView()
    {
    }

protected:
  virtual void on_drag_begin(const Glib::RefPtr<Gdk::DragContext> & )
    {
    }
};

}


#endif
