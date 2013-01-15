/*
 * gnote
 *
 * Copyright (C) 2011-2013 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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



#include <boost/bind.hpp>

#include <glibmm.h>
#include <glibmm/i18n.h>
#include <gtkmm/stock.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/treeiter.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/uimanager.h>

#include "sharp/string.hpp"
#include "notebooks/notebookapplicationaddin.hpp"
#include "notebooks/notebookmanager.hpp"
#include "notebooks/notebooknewnotemenuitem.hpp"
#include "notebooks/notebook.hpp"
#include "iactionmanager.hpp"
#include "debug.hpp"
#include "iconmanager.hpp"
#include "notemanager.hpp"

namespace gnote {
  namespace notebooks {

    ApplicationAddin * NotebookApplicationAddin::create()
    {
      return new NotebookApplicationAddin();
    }


    NotebookApplicationAddin::NotebookApplicationAddin()
      : m_initialized(false)
      , m_notebookUi(0)
      , m_trayNotebookMenu(NULL)
    {
    }



    static const char * uixml = "          <ui>"
          "  <popup name='TrayIconMenu' action='TrayIconMenuAction'>"
          "    <placeholder name='TrayNewNotePlaceholder'>"
          "      <menuitem name='TrayNewNotebookMenu' action='TrayNewNotebookMenuAction' position='top' />"
          "    </placeholder>"
          "  </popup>"
          "</ui>";

    void NotebookApplicationAddin::initialize ()
    {
      m_actionGroup = Glib::RefPtr<Gtk::ActionGroup>(Gtk::ActionGroup::create("Notebooks"));
      m_actionGroup->add(  
        Gtk::Action::create ("TrayNewNotebookMenuAction", Gtk::Stock::NEW,
                             _("Notebooks"),
                             _("Create a new note in a notebook")));
          
      IActionManager & am(IActionManager::obj());
      m_notebookUi = am.get_ui()->add_ui_from_string (uixml);
      
      am.get_ui()->insert_action_group (m_actionGroup, 0);
      
      Gtk::MenuItem *item = dynamic_cast<Gtk::MenuItem*>(
        am.get_widget ("/TrayIconMenu/TrayNewNotePlaceholder/TrayNewNotebookMenu"));
      if (item) {
        Gtk::ImageMenuItem *image_item = dynamic_cast<Gtk::ImageMenuItem*>(item);
        if (image_item) {
          image_item->set_image(*manage(new Gtk::Image(
              IconManager::obj().get_icon(IconManager::NOTEBOOK, 16))));
        }
        m_trayNotebookMenu = manage(new Gtk::Menu());
        item->set_submenu(*m_trayNotebookMenu);
        
        m_trayNotebookMenu->signal_show()
          .connect(sigc::mem_fun(*this, 
                                 &NotebookApplicationAddin::on_tray_notebook_menu_shown));
        m_trayNotebookMenu->signal_hide()
          .connect(sigc::mem_fun(*this, 
                                 &NotebookApplicationAddin::on_tray_notebook_menu_hidden));
      }
      
      Gtk::ImageMenuItem *imageitem = dynamic_cast<Gtk::ImageMenuItem*>(
        am.get_widget ("/NotebooksTreeContextMenu/NewNotebookNote"));
      if (imageitem) {
        imageitem->set_image(*manage(new Gtk::Image(
            IconManager::obj().get_icon(IconManager::NOTE_NEW, 16))));
      }

      NoteManager & nm(note_manager());
      
      for(Note::List::const_iterator iter = nm.get_notes().begin();
          iter != nm.get_notes().end(); ++iter) {
        const Note::Ptr & note(*iter);
        note->signal_tag_added().connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_added));
        note->signal_tag_removed().connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_removed));
      }
       
      nm.signal_note_added.connect(
        sigc::mem_fun(*this, &NotebookApplicationAddin::on_note_added));
      nm.signal_note_deleted.connect(
        sigc::mem_fun(*this, &NotebookApplicationAddin::on_note_deleted));

      am.add_app_action("new-notebook");
      am.get_app_action("new-notebook")->signal_activate().connect(
        sigc::mem_fun(*this, &NotebookApplicationAddin::on_new_notebook_action));
      am.add_app_menu_item(IActionManager::APP_ACTION_NEW, 300, _("New Note_book"), "app.new-notebook");
        
      m_initialized = true;
    }


    void NotebookApplicationAddin::shutdown ()
    {
      IActionManager & am(IActionManager::obj());
      try {
        am.get_ui()->remove_action_group(m_actionGroup);
      } 
      catch (...)
      {
      }
      try {
        am.get_ui()->remove_ui(m_notebookUi);
      } 
      catch (...)
      {
      }
      m_notebookUi = 0;
      
      if (m_trayNotebookMenu) {
        delete m_trayNotebookMenu;
      }
      
      m_initialized = false;
    }
    
    bool NotebookApplicationAddin::initialized ()
    {
      return m_initialized;
    }

    void NotebookApplicationAddin::on_tray_notebook_menu_shown()
    {
      add_menu_items(m_trayNotebookMenu, m_trayNotebookMenuItems);
    }

    void NotebookApplicationAddin::on_tray_notebook_menu_hidden()
    {
      remove_menu_items(m_trayNotebookMenu, m_trayNotebookMenuItems);
    }


    void NotebookApplicationAddin::add_menu_items(Gtk::Menu * menu,   
                                                  std::list<Gtk::MenuItem*> & menu_items)
    {
      remove_menu_items (menu, menu_items);      

      NotebookNewNoteMenuItem *item;

      Glib::RefPtr<Gtk::TreeModel> model = NotebookManager::obj().get_notebooks();
      Gtk::TreeIter iter;
      
      // Add in the "New Notebook..." menu item
      Gtk::ImageMenuItem *newNotebookMenuItem =
        manage(new Gtk::ImageMenuItem (_("New Note_book..."), true));
      newNotebookMenuItem->set_image(*manage(new Gtk::Image(
          IconManager::obj().get_icon(IconManager::NOTEBOOK_NEW, 16))));
      newNotebookMenuItem->signal_activate()
        .connect(sigc::mem_fun(*this, &NotebookApplicationAddin::on_new_notebook_menu_item));
      newNotebookMenuItem->show_all ();
      menu->append (*newNotebookMenuItem);
      menu_items.push_back(newNotebookMenuItem);

      
      if (model->children().size() > 0) {
        Gtk::SeparatorMenuItem *separator = manage(new Gtk::SeparatorMenuItem ());
        separator->show_all ();
        menu->append (*separator);
        menu_items.push_back(separator);
        
        iter = model->children().begin();
        while (iter) {
          Notebook::Ptr notebook;
          iter->get_value(0, notebook);
          item = manage(new NotebookNewNoteMenuItem (notebook));
          item->show_all ();
          menu->append (*item);
          menu_items.push_back(item);
          ++iter;
        }
      }
    }

    void NotebookApplicationAddin::remove_menu_items(Gtk::Menu * menu, 
                                                     std::list<Gtk::MenuItem*> & menu_items)
    {
      for(std::list<Gtk::MenuItem*>::const_iterator iter = menu_items.begin();
          iter != menu_items.end(); ++iter) {
        menu->remove(**iter);
      }
      menu_items.clear();
    }


    void NotebookApplicationAddin::on_new_notebook_menu_item()
    {
      NotebookManager::prompt_create_new_notebook (NULL);
    }


    void NotebookApplicationAddin::on_new_notebook_action(const Glib::VariantBase&)
    {
      NotebookManager::prompt_create_new_notebook (NULL);
    }


    void NotebookApplicationAddin::on_tag_added(const Note & note, const Tag::Ptr& tag)
    {
      if (NotebookManager::obj().is_adding_notebook()) {
        return;
      }
      
      std::string megaPrefix(Tag::SYSTEM_TAG_PREFIX);
      megaPrefix += Notebook::NOTEBOOK_TAG_PREFIX;
      if (!tag->is_system() || !Glib::str_has_prefix(tag->name(), megaPrefix)) {
        return;
      }
      
      std::string notebookName =
        sharp::string_substring(tag->name(), megaPrefix.size());
      
      Notebook::Ptr notebook =
        NotebookManager::obj().get_or_create_notebook (notebookName);
        
      NotebookManager::obj().signal_note_added_to_notebook() (note, notebook);
    }

    

    void NotebookApplicationAddin::on_tag_removed(const Note::Ptr& note, 
                                                  const std::string& normalizedTagName)
    {
      std::string megaPrefix(Tag::SYSTEM_TAG_PREFIX);
      megaPrefix += Notebook::NOTEBOOK_TAG_PREFIX;

      if (!Glib::str_has_prefix(normalizedTagName, megaPrefix)) {
        return;
      }
      
      std::string normalizedNotebookName =
        sharp::string_substring(normalizedTagName, megaPrefix.size());
      
      Notebook::Ptr notebook =
        NotebookManager::obj().get_notebook (normalizedNotebookName);
      if (!notebook) {
        return;
      }
      
      NotebookManager::obj().signal_note_removed_from_notebook() (*note, notebook);
    }

    void NotebookApplicationAddin::on_note_added(const Note::Ptr & note)
    {
        note->signal_tag_added().connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_added));
        note->signal_tag_removed().connect(
          sigc::mem_fun(*this, &NotebookApplicationAddin::on_tag_removed));
    }


    void NotebookApplicationAddin::on_note_deleted(const Note::Ptr &)
    {
      // remove the signal to the note...
    }


  }
}
