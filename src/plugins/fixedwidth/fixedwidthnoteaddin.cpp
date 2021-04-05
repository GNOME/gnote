/*
 * gnote
 *
 * Copyright (C) 2010,2013,2016 Aurimas Cernius
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


#include <glibmm/i18n.h>

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



  void FixedWidthNoteAddin::initialize ()
  {
    // If a tag of this name already exists, don't install.
    if (!get_note()->get_tag_table()->lookup ("monospace")) {
      m_tag = Glib::RefPtr<Gtk::TextTag>(new FixedWidthTag ());
				get_note()->get_tag_table()->add (m_tag);
			}

    auto button = gnote::utils::create_popover_button("win.fixedwidth-enable", "");
    auto label = dynamic_cast<Gtk::Label*>(dynamic_cast<Gtk::Bin*>(button)->get_child());
    Glib::ustring lbl = "<tt>";
    lbl += _("Fixed Wid_th");
    lbl += "</tt>";
    label->set_markup_with_mnemonic(lbl);
    add_text_menu_item(button);
  }


  void FixedWidthNoteAddin::shutdown ()
  {
	// Remove the tag only if we installed it.
    if (m_tag) {
      get_note()->get_tag_table()->remove (m_tag);
    }
  }


  void FixedWidthNoteAddin::on_note_opened ()
  {
    get_window()->text_menu()->signal_show().connect(
      sigc::mem_fun(*this, &FixedWidthNoteAddin::menu_shown));
    dynamic_cast<gnote::NoteTextMenu*>(get_window()->text_menu())->signal_set_accels
      .connect(sigc::mem_fun(*this, &FixedWidthNoteAddin::set_accels));

    gnote::NoteWindow *note_window = get_window();
    note_window->signal_foregrounded.connect(
      sigc::mem_fun(*this, &FixedWidthNoteAddin::on_note_foregrounded));
    note_window->signal_backgrounded.connect(
      sigc::mem_fun(*this, &FixedWidthNoteAddin::on_note_backgrounded));
  }

  void FixedWidthNoteAddin::menu_shown()
  {
    auto host = get_window()->host();
    if(host == NULL) {
      return;
    }

    host->find_action("fixedwidth-enable")->set_state(
      Glib::Variant<bool>::create(get_buffer()->is_active_tag ("monospace")));
  }


  void FixedWidthNoteAddin::on_note_foregrounded()
  {
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
    on_accel();
  }

  void FixedWidthNoteAddin::set_accels(const gnote::utils::GlobalKeybinder & keybinder)
  {
    const_cast<gnote::utils::GlobalKeybinder&>(keybinder).add_accelerator(
      sigc::mem_fun(*this, &FixedWidthNoteAddin::on_accel),
      GDK_KEY_T, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
  }

  void FixedWidthNoteAddin::on_accel()
  {
    get_buffer()->toggle_active_tag("monospace");
  }


}

