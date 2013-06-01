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

  namespace {
    class ReplaceTitleAction
      : public Gtk::Action
    {
    public:
      static Glib::RefPtr<Gtk::Action> create(gnote::NoteWindow *note_window)
        {
          return Glib::RefPtr<Gtk::Action>(new ReplaceTitleAction(note_window));
        }
    protected:
      virtual Gtk::Widget *create_menu_item_vfunc()
        {
          Gtk::ImageMenuItem *menu_item = new Gtk::ImageMenuItem;
          menu_item->add_accelerator("activate", m_note_window->get_accel_group(),
                                     GDK_KEY_R, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
          return menu_item;
        }
    private:
      ReplaceTitleAction(gnote::NoteWindow *note_window)
        : Gtk::Action("ReplaceTitleAction", Gtk::Stock::FIND_AND_REPLACE,
                      _("Replace title"), _("Replace title"))
        , m_note_window(note_window)
        {}

      gnote::NoteWindow *m_note_window;
    };
  }


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
  Glib::RefPtr<Gtk::Action> action = ReplaceTitleAction::create(get_window());
  action->signal_activate().connect(
    sigc::mem_fun(*this, &ReplaceTitleNoteAddin::replacetitle_button_clicked));
  add_note_action(action, 500);
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
