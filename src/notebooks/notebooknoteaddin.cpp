/*
 * gnote
 *
 * Copyright (C) 2010-2016,2019,2022 Aurimas Cernius
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
#include <gtkmm/separator.h>

#include "notebooks/notebooknoteaddin.hpp"
#include "notebooks/notebookmanager.hpp"
#include "debug.hpp"
#include "iactionmanager.hpp"
#include "iconmanager.hpp"
#include "ignote.hpp"
#include "tag.hpp"
#include "notemanagerbase.hpp"
#include "notewindow.hpp"


namespace gnote {
namespace notebooks {

  Tag::Ptr           NotebookNoteAddin::s_templateTag;

  Tag::Ptr NotebookNoteAddin::get_template_tag() const
  {
    if(!s_templateTag) {
      s_templateTag = manager().tag_manager().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
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
    ignote().notebook_manager().signal_notebook_list_changed
      .connect(sigc::mem_fun(*this, &NotebookNoteAddin::on_notebooks_changed));
  }


  void NotebookNoteAddin::on_note_window_foregrounded()
  {
    EmbeddableWidgetHost *host = get_window()->host();
    m_new_notebook_cid = host->find_action("new-notebook")->signal_activate()
      .connect(sigc::mem_fun(*this, &NotebookNoteAddin::on_new_notebook_menu_item));
    Notebook::Ptr current_notebook = ignote().notebook_manager().get_notebook_from_note(get_note());
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


  std::vector<gnote::PopoverWidget> NotebookNoteAddin::get_actions_popover_widgets() const
  {
    auto widgets = NoteAddin::get_actions_popover_widgets();
    if(!get_note()->contains_tag(get_template_tag())) {
      auto notebook_button = new PopoverSubmenuButton(_("Notebook"), true,
        [this] {
          auto submenu = new Gtk::Box(Gtk::Orientation::VERTICAL);
          update_menu(submenu);
          return submenu;
        });

      widgets.push_back(gnote::PopoverWidget(gnote::NOTE_SECTION_CUSTOM_SECTIONS, gnote::NOTEBOOK_ORDER, notebook_button));
    }

    return widgets;
  }


  void NotebookNoteAddin::on_new_notebook_menu_item(const Glib::VariantBase&) const
  {
    Note::List note_list;
    note_list.emplace_back(get_note());
    NotebookManager::prompt_create_new_notebook(ignote(), *dynamic_cast<Gtk::Window*>(get_window()->host()), std::move(note_list));
    get_window()->signal_popover_widgets_changed();
  }


  void NotebookNoteAddin::on_move_to_notebook(const Glib::VariantBase & state)
  {
    get_window()->host()->find_action("move-to-notebook")->set_state(state);
    Glib::ustring name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(state).get();
    Notebook::Ptr notebook;
    if(name.size()) {
      notebook = ignote().notebook_manager().get_notebook(name);
    }
    ignote().notebook_manager().move_note_to_notebook(get_note(), notebook);
  }


  void NotebookNoteAddin::update_menu(Gtk::Box *menu) const
  {
    // Add new notebook item
    Gtk::Widget *new_notebook_item = manage(utils::create_popover_button("win.new-notebook", _("_New notebook...")));
    menu->append(*new_notebook_item);
    menu->append(*manage(new Gtk::Separator));

    // Add the "(no notebook)" item at the top of the list
    Gtk::Button *no_notebook_item = manage(utils::create_popover_button("win.move-to-notebook", _("No notebook")));
    no_notebook_item->set_action_target_value(Glib::Variant<Glib::ustring>::create(""));
    menu->append(*no_notebook_item);

    // Add in all the real notebooks
    auto notebook_menu_items = get_notebook_menu_items();
    if(!notebook_menu_items.empty()) {
      for(auto item : notebook_menu_items) {
        menu->append(*item);
      }

    }
  }
  

  std::vector<Gtk::Widget*> NotebookNoteAddin::get_notebook_menu_items() const
  {
    std::vector<Gtk::Widget*> items;
    Glib::RefPtr<Gtk::TreeModel> model = ignote().notebook_manager().get_notebooks();
    for(const auto& row : model->children()) {
      Notebook::Ptr notebook;
      row.get_value(0, notebook);
      Gtk::Button *item = manage(utils::create_popover_button("win.move-to-notebook", notebook->get_name()));
      item->set_action_target_value(Glib::Variant<Glib::ustring>::create(notebook->get_name()));
      items.push_back(item);
    }

    return items;
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
