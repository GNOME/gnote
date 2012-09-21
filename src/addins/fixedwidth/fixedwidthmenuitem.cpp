/*
 * gnote
 *
 * Copyright (C) 2010-2012 Aurimas Cernius
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

#include "noteaddin.hpp"
#include "notewindow.hpp"

#include "fixedwidthmenuitem.hpp"


namespace fixedwidth {

  FixedWidthMenuItem::FixedWidthMenuItem(gnote::NoteAddin *addin)
    : Gtk::CheckMenuItem(Glib::ustring("<tt>")
                         + _("Fixed Wid_th") + "</tt>", true)
    , m_note_addin(addin)
    , m_event_freeze(false)
  {
    gnote::NoteTextMenu::markup_label(*this);
    m_note_addin->get_window()->text_menu()->signal_show().connect(
      sigc::mem_fun(*this, &FixedWidthMenuItem::menu_shown));

    gnote::NoteWindow *note_window = addin->get_window();
    note_window->signal_foregrounded.connect(
      sigc::mem_fun(*this, &FixedWidthMenuItem::on_note_foregrounded));
    note_window->signal_backgrounded.connect(
      sigc::mem_fun(*this, &FixedWidthMenuItem::on_note_backgrounded));

    show_all();
  }


  void FixedWidthMenuItem::on_activate()
  {
    if (!m_event_freeze)
      m_note_addin->get_buffer()->toggle_active_tag ("monospace");
    Gtk::CheckMenuItem::on_activate();
  }


  void FixedWidthMenuItem::menu_shown()
  {
    m_event_freeze = true;
    set_active(m_note_addin->get_buffer()->is_active_tag ("monospace"));
    m_event_freeze = false;
  }


  void FixedWidthMenuItem::on_note_foregrounded()
  {
    add_accelerator("activate",
                    m_note_addin->get_window()->get_accel_group(),
                    GDK_KEY_T,
                    Gdk::CONTROL_MASK,
                    Gtk::ACCEL_VISIBLE);
  }


  void FixedWidthMenuItem::on_note_backgrounded()
  {
    remove_accelerator(m_note_addin->get_window()->get_accel_group(),
                       GDK_KEY_T,
                       Gdk::CONTROL_MASK);
  }


}
