/*
 * gnote
 *
 * Copyright (C) 2010,2013,2016,2023,2025 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
 * Original C# file
 * (C) 2009 Mark Wakim <markwakim@gmail.com>
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


// Translated from UnderlineNoteAddin.cs:


#include <glibmm/i18n.h>
#include <gtkmm/togglebutton.h>

#include "debug.hpp"
#include "sharp/modulefactory.hpp"
#include "notewindow.hpp"
#include "underlinenoteaddin.hpp"
#include "underlinetag.hpp"


namespace underline {

  UnderlineModule::UnderlineModule()
  {
    ADD_INTERFACE_IMPL(UnderlineNoteAddin);
    enabled(false);
  }



  void UnderlineNoteAddin::initialize()
  {
    // If a tag of this name already exists, don't install.
    auto & tag_table = get_note().get_tag_table();
    if(!tag_table->lookup("underline")) {
      m_tag = Glib::make_refptr_for_instance(new UnderlineTag());
      tag_table->add(m_tag);
    }
  }


  void UnderlineNoteAddin::shutdown()
  {
    // Remove the tag only if we installed it.
    if(m_tag) {
      get_note().get_tag_table()->remove(m_tag);
      m_tag.reset();
    }
  }


  void UnderlineNoteAddin::on_note_opened()
  {
    gnote::NoteWindow *note_window = get_window();
    note_window->signal_build_text_menu
      .connect(sigc::mem_fun(*this, &UnderlineNoteAddin::add_menu_item));

    auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_U, Gdk::ModifierType::CONTROL_MASK);
    auto action = Gtk::NamedAction::create("win.underline-enable");
    auto shortcut = Gtk::Shortcut::create(trigger, action);
    note_window->shortcut_controller().add_shortcut(shortcut); // TODO also remove
  }

  void UnderlineNoteAddin::on_note_foregrounded()
  {
    m_on_underline_clicked_cid.disconnect(); // just in case, prevent subscribing twice
    m_on_underline_clicked_cid = get_window()->host()->find_action("underline-enable")->signal_change_state()
      .connect(sigc::mem_fun(*this, &UnderlineNoteAddin::on_underline_clicked));
  }

  void UnderlineNoteAddin::on_note_backgrounded()
  {
    m_on_underline_clicked_cid.disconnect();
  }

  void UnderlineNoteAddin::add_menu_item(gnote::NoteTextMenu & menu)
  {
    auto box = dynamic_cast<Gtk::Box*>(menu.get_child());
    if(!box) {
      ERR_OUT("Menu child is not Gtk::Box");
      return;
    }

    auto formatting = box->get_first_child();
    while(formatting && formatting->get_name() != "formatting") {
      formatting = formatting->get_next_sibling();
    }
    if(!formatting) {
      ERR_OUT("Item 'formatting' not found");
      return;
    }
    auto fmt_box = dynamic_cast<Gtk::Box*>(formatting);
    if(!fmt_box) {
      ERR_OUT("Item 'formatting' is not Gtk::Box");
      return;
    }
    auto font_box = fmt_box->get_first_child();
    while(font_box && font_box->get_name() != "font-box") {
      font_box = font_box->get_next_sibling();
    }
    if(!font_box) {
      ERR_OUT("Item 'font_box' not found");
      return;
    }
    auto fnt_box = dynamic_cast<Gtk::Box*>(font_box);
    if(!fnt_box) {
     ERR_OUT("Item 'font-box' is not Gtk::Box");
     return;
    }

    get_window()->host()->find_action("underline-enable")->set_state(Glib::Variant<bool>::create(get_buffer()->is_active_tag("underline")));

    auto button = Gtk::make_managed<Gtk::ToggleButton>();
    button->set_action_name("win.underline-enable");
    button->set_icon_name("format-text-underline-symbolic");
    button->set_has_frame(false);
    fnt_box->append(*button);
  }

  void UnderlineNoteAddin::on_underline_clicked(const Glib::VariantBase & state)
  {
    get_window()->host()->find_action("underline-enable")->set_state(state);
    on_underline_pressed();
  }

  void UnderlineNoteAddin::on_underline_pressed()
  {
    get_buffer()->toggle_active_tag("underline");
  }

}

