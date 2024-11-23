/*
 * gnote
 *
 * Copyright (C) 2010-2016,2019,2022-2024 Aurimas Cernius
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

  Tag &NotebookNoteAddin::get_template_tag() const
  {
    return manager().tag_manager().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
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
    Glib::ustring name;
    if(auto current_notebook = ignote().notebook_manager().get_notebook_from_note(get_note())) {
      name = current_notebook.value().get().get_name();
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
    if(!get_note().contains_tag(get_template_tag())) {
      auto notebook_button = Gio::MenuItem::create(_("Notebook"), make_menu());
      widgets.push_back(gnote::PopoverWidget(gnote::NOTE_SECTION_CUSTOM_SECTIONS, gnote::NOTEBOOK_ORDER, notebook_button));
    }

    return widgets;
  }


  void NotebookNoteAddin::on_new_notebook_menu_item(const Glib::VariantBase&) const
  {
    std::vector<NoteBase::Ref> note_list;
    note_list.emplace_back(get_note());
    auto & gnote = ignote();
    gnote.notebook_manager().prompt_create_new_notebook(gnote, *dynamic_cast<Gtk::Window*>(get_window()->host()), std::move(note_list));
    get_window()->signal_popover_widgets_changed();
  }


  void NotebookNoteAddin::on_move_to_notebook(const Glib::VariantBase & state)
  {
    get_window()->host()->find_action("move-to-notebook")->set_state(state);
    Glib::ustring name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(state).get();
    Notebook::ORef notebook;
    if(name.size()) {
      notebook = ignote().notebook_manager().get_notebook(name);
    }
    ignote().notebook_manager().move_note_to_notebook(get_note(), notebook);
  }


  Glib::RefPtr<Gio::MenuModel> NotebookNoteAddin::make_menu() const
  {
    auto menu = Gio::Menu::create();

    // Add new notebook item
    auto new_notebook_item = Gio::MenuItem::create(_("_New notebook..."), "win.new-notebook");
    menu->append_item(new_notebook_item);

    // Add the "(no notebook)" item at the top of the list
    auto no_notebook_item = Gio::MenuItem::create(_("No notebook"), "");
    no_notebook_item->set_action_and_target("win.move-to-notebook", Glib::Variant<Glib::ustring>::create(""));
    menu->append_item(no_notebook_item);

    // Add in all the real notebooks
    menu->append_section(get_notebook_menu_items());
    return menu;
  }
  

  Glib::RefPtr<Gio::MenuModel> NotebookNoteAddin::get_notebook_menu_items() const
  {
    auto items = Gio::Menu::create();
    std::vector<Notebook::Ref> notebooks;
    ignote().notebook_manager().get_notebooks([&notebooks](const Notebook::Ptr& nb) { notebooks.emplace_back(*nb); });
    for(const Notebook& notebook : notebooks) {
      const auto name = notebook.get_name();
      auto item = Gio::MenuItem::create(name, "");
      item->set_action_and_target("win.move-to-notebook", Glib::Variant<Glib::ustring>::create(name));
      items->append_item(item);
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
