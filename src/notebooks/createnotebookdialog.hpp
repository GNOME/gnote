


#ifndef __NOTEBOOK_CREATENOTEBOOKDIALOG_HPP_
#define __NOTEBOOK_CREATENOTEBOOKDIALOG_HPP_

#include <gdkmm/pixbuf.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>

#include "utils.hpp"

namespace gnote {
  namespace notebooks {


class CreateNotebookDialog
  : public utils::HIGMessageDialog
{
public:
  CreateNotebookDialog(Gtk::Window *parent, GtkDialogFlags f);

  std::string get_notebook_name();
  void set_notebook_name(const std::string &);

private:
  void on_name_entry_changed();
  Gtk::Entry                m_nameEntry;
  Gtk::Label                m_errorLabel;
  Glib::RefPtr<Gdk::Pixbuf> m_newNotebookIcon;
  Glib::RefPtr<Gdk::Pixbuf> m_newNotebookIconDialog;
};

  }
}


#endif
