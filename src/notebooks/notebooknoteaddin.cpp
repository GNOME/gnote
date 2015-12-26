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
#include <gtkmm/modelbutton.h>

#include "notebooks/notebooknoteaddin.hpp"
#include "notebooks/notebookmanager.hpp"
#include "debug.hpp"
#include "iactionmanager.hpp"
#include "iconmanager.hpp"
#include "tag.hpp"
#include "itagmanager.hpp"
#include "notewindow.hpp"


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
    auto note_win = get_window();
    note_win->signal_foregrounded.connect(sigc::mem_fun(*this, &NotebookNoteAddin::on_note_window_foregrounded));
    note_win->signal_backgrounded.connect(sigc::mem_fun(*this, &NotebookNoteAddin::on_note_window_backgrounded));
    NotebookManager::obj().signal_notebook_list_changed
      .connect(sigc::mem_fun(*this, &NotebookNoteAddin::on_notebooks_changed));
  }


  void NotebookNoteAddin::on_note_window_foregrounded()
  {
    EmbeddableWidgetHost *host = get_window()->host();
    m_new_notebook_cid = host->find_action("new-notebook")->signal_activate()
      .connect(sigc::mem_fun(*this, &NotebookNoteAddin::on_new_notebook_menu_item));
    Notebook::Ptr current_notebook = NotebookManager::obj().get_notebook_from_note(get_note());
    Glib::ustring name;
    if(current_notebook) {
      name = current_notebook->get_name();
    }
    MainWindowAction::Ptr action = host->find_action("move-to-notebook");
    action->set_state(Glib::Variant<Glib::ustring>::create(name));
    m_move_to_notebook_cid = action->signal_change_state()
      .connect(sigc::mem_fun(*this, &NotebookNoteAddin::on_move_to_notebook));
  }


  void NotebookNoteAddin::on_note_window_backgrounded()
  {
    m_new_notebook_cid.disconnect();
    m_move_to_notebook_cid.disconnect();
  }


  std::map<int, Gtk::Widget*> NotebookNoteAddin::get_actions_popover_widgets() const
  {
    auto widgets = NoteAddin::get_actions_popover_widgets();
    if(!get_note()->contains_tag(get_template_tag())) {
      Gtk::Widget *notebook_button = utils::create_popover_submenu_button("notebooks-submenu", _("Notebook"));
      utils::add_item_to_ordered_map(widgets, gnote::NOTEBOOK_ORDER, notebook_button);

      auto submenu = utils::create_popover_submenu("notebooks-submenu");
      update_menu(submenu);
      utils::add_item_to_ordered_map(widgets, 1000000, submenu);
    }

    return widgets;
  }


  void NotebookNoteAddin::on_new_notebook_menu_item(const Glib::VariantBase&) const
  {
    Note::List noteList;
    noteList.push_back(get_note());
    NotebookManager::obj().prompt_create_new_notebook(
      dynamic_cast<Gtk::Window*>(get_window()->host()), noteList);
    get_window()->signal_popover_widgets_changed();
  }


  void NotebookNoteAddin::on_move_to_notebook(const Glib::VariantBase & state)
  {
    get_window()->host()->find_action("move-to-notebook")->set_state(state);
    Glib::ustring name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(state).get();
    Notebook::Ptr notebook;
    if(name.size()) {
      notebook = NotebookManager::obj().get_notebook(name);
    }
    NotebookManager::obj().move_note_to_notebook(get_note(), notebook);
  }


  void NotebookNoteAddin::update_menu(Gtk::Grid *menu) const
  {
    int top = 0;
    int sub_top = 0;
    Gtk::Grid *subgrid = manage(new Gtk::Grid);
    utils::set_common_popover_widget_props(*subgrid);

    // Add new notebook item
    Gtk::Widget *new_notebook_item = manage(utils::create_popover_button("win.new-notebook", _("_New notebook...")));
    subgrid->attach(*new_notebook_item, 0, sub_top++, 1, 1);
    menu->attach(*subgrid, 0, top++, 1, 1);

    // Add the "(no notebook)" item at the top of the list
    subgrid = manage(new Gtk::Grid);
    utils::set_common_popover_widget_props(*subgrid);
    sub_top = 0;
    Gtk::ModelButton *no_notebook_item = dynamic_cast<Gtk::ModelButton*>(manage(
      utils::create_popover_button("win.move-to-notebook", _("No notebook"))));
    gtk_actionable_set_action_target_value(GTK_ACTIONABLE(no_notebook_item->gobj()), g_variant_new_string(""));
    subgrid->attach(*no_notebook_item, 0, sub_top++, 1, 1);

    // Add in all the real notebooks
    std::list<Gtk::ModelButton*> notebook_menu_items;
    get_notebook_menu_items(notebook_menu_items);
    if(!notebook_menu_items.empty()) {
      FOREACH(Gtk::ModelButton *item, notebook_menu_items) {
        subgrid->attach(*item, 0, sub_top++, 1, 1);
      }

    }

    menu->attach(*subgrid, 0, top++, 1, 1);

    subgrid = manage(new Gtk::Grid);
    sub_top = 0;
    utils::set_common_popover_widget_props(*subgrid);
    Gtk::Widget *back_button = utils::create_popover_submenu_button("main", _("_Back"));
    dynamic_cast<Gtk::ModelButton*>(back_button)->property_inverted() = true;
    subgrid->attach(*back_button, 0, sub_top++, 1, 1);
    menu->attach(*subgrid, 0, top++, 1, 1);
  }
  

  void NotebookNoteAddin::get_notebook_menu_items(std::list<Gtk::ModelButton*>& items) const
  {
    Glib::RefPtr<Gtk::TreeModel> model = NotebookManager::obj().get_notebooks();
    Gtk::TreeIter iter;

    items.clear();

    iter = model->children().begin();
    for(iter = model->children().begin(); iter != model->children().end(); ++iter) {
      Notebook::Ptr notebook;
      iter->get_value(0, notebook);
      Gtk::ModelButton *item = dynamic_cast<Gtk::ModelButton*>(manage(
        utils::create_popover_button("win.move-to-notebook", notebook->get_name())));
      gtk_actionable_set_action_target_value(GTK_ACTIONABLE(item->gobj()), g_variant_new_string(notebook->get_name().c_str()));
      items.push_back(item);
    }
  }


  void NotebookNoteAddin::on_notebooks_changed()
  {
    auto note_win = get_window();
    if(!note_win) {
      return;
    }
    auto host = dynamic_cast<HasActions*>(note_win->host());
    if(host) {
      host->signal_popover_widgets_changed();
    }
  }


}
}
