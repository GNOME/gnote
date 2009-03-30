

#ifndef __NOTEBOOK_APPLICATION_ADDIN_HPP__
#define __NOTEBOOK_APPLICATION_ADDIN_HPP__

#include <gdkmm/pixbuf.h>
#include <gtkmm/actiongroup.h>

#include "applicationaddin.hpp"
#include "note.hpp"

namespace gnote {
  namespace notebooks {


    class NotebookApplicationAddin
      : public ApplicationAddin
    {
    public:
      static ApplicationAddin * create();
      virtual void initialize ();
      virtual void shutdown ();
      virtual bool initialized ();

    protected:
      NotebookApplicationAddin();
    private:
      void on_tray_notebook_menu_shown();
      void on_tray_notebook_menu_hidden();
      void on_new_notebook_menu_shown();
      void on_new_notebook_menu_hidden();
      void add_menu_items(Gtk::Menu *);
      void remove_menu_items(Gtk::Menu *);
      void on_new_notebook_menu_item();
      void on_tag_added(const Note::Ptr&, const Tag::Ptr&);
      void on_tag_removed(const Note::Ptr&, const std::string&);
      void on_note_added(const Note::Ptr &);
      void on_note_deleted(const Note::Ptr &);
////


      bool m_initialized;
      guint m_notebookUi;
      Glib::RefPtr<Gtk::ActionGroup> m_actionGroup;
      Glib::RefPtr<Gdk::Pixbuf>      m_notebookIcon;
      Glib::RefPtr<Gdk::Pixbuf>      m_newNotebookIcon;
      Gtk::Menu                     *m_trayNotebookMenu;
      Gtk::Menu                     *m_mainWindowNotebookMenu;

    };


  }
}


#endif

