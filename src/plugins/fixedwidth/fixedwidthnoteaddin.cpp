/*
 * gnote
 *
 * Copyright (C) 2010,2013,2016,2023,2025 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
 * Original C# file
 * (C) 2006 Ryan Lortie <desrt@desrt.ca>
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


// Translated from FixedWidthNoteAddin.cs:
// Add a 'fixed width' item to the font styles menu.


#include "debug.hpp"

#include <glibmm/i18n.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>

#include "sharp/modulefactory.hpp"
#include "notewindow.hpp"
#include "utils.hpp"
#include "fixedwidthnoteaddin.hpp"
#include "fixedwidthtag.hpp"


namespace fixedwidth {

  FixedWidthModule::FixedWidthModule()
  {
    ADD_INTERFACE_IMPL(FixedWidthNoteAddin);
  }



  void FixedWidthNoteAddin::initialize()
  {
    // If a tag of this name already exists, don't install.
    auto tag_table = get_note().get_tag_table();
    if(!tag_table->lookup("monospace")) {
      auto tag = Glib::make_refptr_for_instance(new FixedWidthTag);
      m_tag = tag;
      tag_table->add(std::move(tag));
  }
  }


  void FixedWidthNoteAddin::shutdown()
  {
    // Remove the tag only if we installed it.
    if(m_tag) {
      get_note().get_tag_table()->remove(m_tag);
      m_tag.reset();
    }
  }


  void FixedWidthNoteAddin::on_note_opened()
  {
    gnote::NoteWindow *note_window = get_window();
    note_window->signal_build_text_menu
      .connect(sigc::mem_fun(*this, &FixedWidthNoteAddin::add_menu_item));

    auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_T, Gdk::ModifierType::CONTROL_MASK);
    auto action = Gtk::NamedAction::create("win.fixedwidth-enable");
    auto shortcut = Gtk::Shortcut::create(trigger, action);
    note_window->shortcut_controller().add_shortcut(shortcut);
  }


  void FixedWidthNoteAddin::on_note_foregrounded()
  {
    m_menu_item_cid.disconnect();  // just in caes, don't subscribe twice
    m_menu_item_cid = get_window()->host()->find_action("fixedwidth-enable")->signal_change_state()
      .connect(sigc::mem_fun(*this, &FixedWidthNoteAddin::on_menu_item_state_changed));
  }


  void FixedWidthNoteAddin::on_note_backgrounded()
  {
    m_menu_item_cid.disconnect();
  }

  void FixedWidthNoteAddin::on_menu_item_state_changed(const Glib::VariantBase & state)
  {
    get_window()->host()->find_action("fixedwidth-enable")->set_state(state);
    get_buffer()->toggle_active_tag("monospace");
  }

  void FixedWidthNoteAddin::add_menu_item(gnote::NoteTextMenu & menu)
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

    get_window()->host()->find_action("fixedwidth-enable")->set_state(Glib::Variant<bool>::create(get_buffer()->is_active_tag("monospace")));

    auto button = Gtk::make_managed<Gtk::ToggleButton>();
    button->set_action_name("win.fixedwidth-enable");
    button->set_has_frame(false);
    auto label = Gtk::make_managed<Gtk::Label>();
    Glib::ustring lbl = "<tt>";
    lbl += _("Fixed Wid_th");
    lbl += "</tt>";
    label->set_markup_with_mnemonic(lbl);
    button->set_child(*label);
    fmt_box->append(*button);
  }
}

