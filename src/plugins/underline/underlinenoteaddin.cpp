/*
 * gnote
 *
 * Copyright (C) 2010,2013,2016 Aurimas Cernius
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



  void UnderlineNoteAddin::initialize ()
  {
    // If a tag of this name already exists, don't install.
    if (!get_note()->get_tag_table()->lookup ("underline")) {
      m_tag = Glib::RefPtr<Gtk::TextTag>(new UnderlineTag ());
				get_note()->get_tag_table()->add (m_tag);
			}

    Gtk::Widget *button = gnote::utils::create_popover_button("win.underline-enable", "");
    auto label = dynamic_cast<Gtk::Label*>(dynamic_cast<Gtk::Bin*>(button)->get_child());
    Glib::ustring lbl("<u>");
    lbl += _("_Underline");
    lbl += "</u>";
    label->set_markup_with_mnemonic(lbl);
    add_text_menu_item(button);
  }


  void UnderlineNoteAddin::shutdown ()
  {
	// Remove the tag only if we installed it.
    if (m_tag) {
      get_note()->get_tag_table()->remove (m_tag);
    }
  }


  void UnderlineNoteAddin::on_note_opened ()
  {
    get_window()->text_menu()->signal_show().connect(
      sigc::mem_fun(*this, &UnderlineNoteAddin::menu_shown));
    dynamic_cast<gnote::NoteTextMenu*>(get_window()->text_menu())->signal_set_accels
      .connect(sigc::mem_fun(*this, &UnderlineNoteAddin::set_accels));

    gnote::NoteWindow *note_window = get_window();
    note_window->signal_foregrounded.connect(
      sigc::mem_fun(*this, &UnderlineNoteAddin::on_note_foregrounded));
    note_window->signal_backgrounded.connect(
      sigc::mem_fun(*this, &UnderlineNoteAddin::on_note_backgrounded));
  }

  void UnderlineNoteAddin::on_note_foregrounded()
  {
    m_on_underline_clicked_cid = get_window()->host()->find_action("underline-enable")->signal_change_state()
      .connect(sigc::mem_fun(*this, &UnderlineNoteAddin::on_underline_clicked));
  }

  void UnderlineNoteAddin::on_note_backgrounded()
  {
    m_on_underline_clicked_cid.disconnect();
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

  void UnderlineNoteAddin::menu_shown()
  {
    get_window()->host()->find_action("underline-enable")->set_state(
      Glib::Variant<bool>::create(get_buffer()->is_active_tag("underline")));
  }

  void UnderlineNoteAddin::set_accels(const gnote::utils::GlobalKeybinder & keybinder)
  {
    const_cast<gnote::utils::GlobalKeybinder&>(keybinder).add_accelerator(
      sigc::mem_fun(*this, &UnderlineNoteAddin::on_underline_pressed),
      GDK_KEY_U, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
  }


}

