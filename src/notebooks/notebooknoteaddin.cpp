/*
 * gnote
 *
 * Copyright (C) 2010-2015 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



#include <glibmm/i18n.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/separatormenuitem.h>

#include "notebooks/notebooknoteaddin.hpp"
#include "notebooks/notebookmanager.hpp"
#include "debug.hpp"
#include "iactionmanager.hpp"
#include "iconmanager.hpp"
#include "tag.hpp"
#include "itagmanager.hpp"
#include "notewindow.hpp"


namespace {

class NotebookNoteAction
  : public Gtk::Action
{
public:
  static Glib::RefPtr<Gtk::Action> create(const sigc::slot<void, Gtk::Menu*> & slot)
    {
      return Glib::RefPtr<Gtk::Action>(new NotebookNoteAction(slot));
    }
  virtual Gtk::Widget *create_menu_item_vfunc() override
    {
      m_submenu_built = false;
      Gtk::MenuItem *menu_item = new Gtk::ImageMenuItem;
      m_menu = manage(new Gtk::Menu);
      m_menu->signal_hide().connect(
        sigc::mem_fun(*this, &NotebookNoteAction::on_menu_hidden));
      menu_item->set_submenu(*m_menu);
      return menu_item;
    }
protected:
  virtual void on_activate() override
    {
      Gtk::Action::on_activate();
      if(m_submenu_built) {
        return;
      }
      update_menu();
    }
private:
  explicit NotebookNoteAction(const sigc::slot<void, Gtk::Menu*> & slot)
    : m_submenu_built(false)
    , m_update_menu_slot(slot)
    {
      set_name("NotebookAction");
      set_label(_("Notebook"));
      set_tooltip(_("Place this note into a notebook"));
    }
  void on_menu_hidden()
    {
      m_submenu_built = false;
    }
  void update_menu()
    {
      m_update_menu_slot(m_menu);
      m_submenu_built = true;
    }

  Gtk::Menu *m_menu;
  bool m_submenu_built;
  sigc::slot<void, Gtk::Menu*> m_update_menu_slot;
};

}

namespace gnote {
namespace notebooks {

  Tag::Ptr           NotebookNoteAddin::s_templateTag;

  Tag::Ptr NotebookNoteAddin::get_template_tag()
  {
    if(!s_templateTag) {
      s_templateTag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
    }
    return s_templateTag;
  }
  

  NotebookNoteAddin::NotebookNoteAddin()
  {
  }


  NoteAddin * NotebookNoteAddin::create()
  {
    return new NotebookNoteAddin();
  }


  void NotebookNoteAddin::initialize ()
  {
  }


  void NotebookNoteAddin::shutdown ()
  {
  }


  void NotebookNoteAddin::on_note_opened()
  {
    if(!get_note()->contains_tag(get_template_tag())) {
      add_note_action(NotebookNoteAction::create(sigc::mem_fun(*this, &NotebookNoteAddin::update_menu)),
                      gnote::NOTEBOOK_ORDER);
    }
  }


  void NotebookNoteAddin::on_new_notebook_menu_item()
  {
    Note::List noteList;
    noteList.push_back(get_note());
    NotebookManager::obj().prompt_create_new_notebook(
      dynamic_cast<Gtk::Window*>(get_note()->get_window()->host()), noteList);
  }


  void NotebookNoteAddin::update_menu(Gtk::Menu *menu)
  {
    // Clear the old items
    FOREACH(Gtk::Widget *menu_item, menu->get_children()) {
      menu->remove(*menu_item);
    }

    // Add new notebook item
    Gtk::ImageMenuItem *new_notebook_item = manage(new Gtk::ImageMenuItem(_("_New notebook..."), true));
    new_notebook_item->set_image(*manage(new Gtk::Image(
      IconManager::obj().get_icon(IconManager::NOTEBOOK_NEW, 16))));
    new_notebook_item->signal_activate().connect(sigc::mem_fun(*this, &NotebookNoteAddin::on_new_notebook_menu_item));
    new_notebook_item->show();
    menu->append(*new_notebook_item);

    // Add the "(no notebook)" item at the top of the list
    NotebookMenuItem *no_notebook_item = manage(new NotebookMenuItem(get_note(), Notebook::Ptr()));
    no_notebook_item->show_all();
    menu->append(*no_notebook_item);

    NotebookMenuItem *active_menu_item = no_notebook_item;
    Notebook::Ptr current_notebook = NotebookManager::obj().get_notebook_from_note(get_note());
      
    // Add in all the real notebooks
    std::list<NotebookMenuItem*> notebook_menu_items;
    get_notebook_menu_items(notebook_menu_items);
    if(!notebook_menu_items.empty()) {
      Gtk::SeparatorMenuItem *separator = manage(new Gtk::SeparatorMenuItem());
      separator->show_all();
      menu->append(*separator);

      FOREACH(NotebookMenuItem *item, notebook_menu_items) {
        item->show_all();
        menu->append(*item);
        if(current_notebook == item->get_notebook())
          active_menu_item = item;
      }
    }

    active_menu_item->set_active(true);
  }
  

  void NotebookNoteAddin::get_notebook_menu_items(std::list<NotebookMenuItem*>& items)
  {
    Glib::RefPtr<Gtk::TreeModel> model = NotebookManager::obj().get_notebooks();
    Gtk::TreeIter iter;

    items.clear();

    iter = model->children().begin();
    for(iter = model->children().begin(); iter != model->children().end(); ++iter) {
      Notebook::Ptr notebook;
      iter->get_value(0, notebook);
      NotebookMenuItem *item = manage(new NotebookMenuItem(get_note(), notebook));
      items.push_back(item);
    }
  }


}
}
