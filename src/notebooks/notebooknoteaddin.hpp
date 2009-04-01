


#ifndef __NOTEBOOKS_NOTEBOOK_NOTE_ADDIN_HPP__
#define __NOTEBOOKS_NOTEBOOK_NOTE_ADDIN_HPP__

#include <gtkmm/menu.h>
#include <gtkmm/menutoolbutton.h>

#include "noteaddin.hpp"
#include "notebooks/notebook.hpp"
#include "notebooks/notebookmenuitem.hpp"
#include "note.hpp"

namespace gnote {
namespace notebooks {

  class NotebookNoteAddin
    : public NoteAddin
  {
  public:
    static NoteAddin * create();    
    virtual void initialize ();
    virtual void shutdown ();
    virtual void on_note_opened ();

  protected:
    NotebookNoteAddin();

  private:
    void initialize_tool_button();
    void on_menu_shown();
    void on_note_added_to_notebook(const Note &, const Notebook::Ptr &);
    void on_note_removed_from_notebook(const Note &, const Notebook::Ptr &);
    void on_new_notebook_menu_item();
    void update_notebook_button_label();
    void update_notebook_button_label(const Notebook::Ptr &);
    void update_menu();
    std::list<NotebookMenuItem*> get_notebook_menu_items();
    Gtk::MenuToolButton      *m_toolButton;
    Gtk::Menu                *m_menu;
    Gtk::RadioButtonGroup     m_radio_group;
    Glib::RefPtr<Gdk::Pixbuf> m_notebookIcon;
    Glib::RefPtr<Gdk::Pixbuf> m_newNotebookIcon;
    sigc::connection          m_show_menu_cid;
    sigc::connection          m_note_added_cid;
    sigc::connection          m_note_removed_cid;
  };

}
}


#endif
