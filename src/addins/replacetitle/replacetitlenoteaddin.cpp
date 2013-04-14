/*
 * gnote
 *
 * Copyright (C) 2011-2013 Aurimas Cernius
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
#include <gtkmm/stock.h>
#include <gtkmm/clipboard.h>

#include "notewindow.hpp"
#include "replacetitlenoteaddin.hpp"
#include "sharp/string.hpp"

namespace replacetitle {


ReplaceTitleModule::ReplaceTitleModule()
{
  ADD_INTERFACE_IMPL(ReplaceTitleNoteAddin);
}


void ReplaceTitleNoteAddin::initialize()
{
}

void ReplaceTitleNoteAddin::shutdown()
{
}

void ReplaceTitleNoteAddin::on_note_opened()
{
  m_menu_item = manage(new Gtk::ImageMenuItem(_("Replace title")));
  m_menu_item->set_image(*manage(new Gtk::Image(Gtk::Stock::FIND_AND_REPLACE, Gtk::ICON_SIZE_MENU)));
  m_menu_item->signal_activate().connect(
    sigc::mem_fun(*this, &ReplaceTitleNoteAddin::replacetitle_button_clicked));

  gnote::NoteWindow *note_window = get_window();
  note_window->signal_foregrounded.connect(
    sigc::mem_fun(*this, &ReplaceTitleNoteAddin::on_note_foregrounded));
  note_window->signal_backgrounded.connect(
    sigc::mem_fun(*this, &ReplaceTitleNoteAddin::on_note_backgrounded));

  m_menu_item->show() ;
  add_plugin_menu_item(m_menu_item);
}

void ReplaceTitleNoteAddin::on_note_foregrounded()
{
  m_menu_item->add_accelerator("activate",
                               get_window()->get_accel_group(),
                               GDK_KEY_R,
                               Gdk::CONTROL_MASK,
                                Gtk::ACCEL_VISIBLE);
}

void ReplaceTitleNoteAddin::on_note_backgrounded()
{
  m_menu_item->remove_accelerator(get_window()->get_accel_group(),
                                  GDK_KEY_R, Gdk::CONTROL_MASK);
}

void ReplaceTitleNoteAddin::replacetitle_button_clicked()
{
  // unix primary clipboard
  Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get(GDK_SELECTION_PRIMARY);
  const std::string newTitle= refClipboard->wait_for_text();
  Glib::RefPtr<Gtk::TextBuffer> buffer = get_note()->get_buffer();
  Gtk::TextIter iter = buffer->get_insert()->get_iter();
  int line = iter.get_line();
  int line_offset = iter.get_line_offset();

  // replace note content
  if(!newTitle.empty()) {
    std::string new_content(get_note()->xml_content());
    get_note()->set_xml_content(sharp::string_replace_first(new_content, get_note()->get_title(), newTitle));
    if(line) {
      iter = buffer->get_insert()->get_iter();
      iter.set_line(line);
      iter.set_line_offset(line_offset);
      buffer->place_cursor(iter);
    }
  }
}

}
