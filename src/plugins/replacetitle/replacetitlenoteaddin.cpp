/*
 * gnote
 *
 * Copyright (C) 2011-2013,2016-2017,2019,2023 Aurimas Cernius
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

#include "iactionmanager.hpp"
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
  register_main_window_action_callback("replacetitle-replace",
    sigc::mem_fun(*this, &ReplaceTitleNoteAddin::replacetitle_button_clicked));
}

std::vector<gnote::PopoverWidget> ReplaceTitleNoteAddin::get_actions_popover_widgets() const
{
  auto widgets = NoteAddin::get_actions_popover_widgets();
  auto item = Gio::MenuItem::create(_("Replace title"), "win.replacetitle-replace");
  widgets.push_back(gnote::PopoverWidget::create_for_note(gnote::REPLACE_TITLE_ORDER, item));
  return widgets;
}

void ReplaceTitleNoteAddin::replacetitle_button_clicked(const Glib::VariantBase&)
{
  // unix primary clipboard
  auto refClipboard = Gdk::Display::get_default()->get_primary_clipboard();
  refClipboard->read_text_async([this, refClipboard](const Glib::RefPtr<Gio::AsyncResult> & result) {
    const Glib::ustring newTitle = refClipboard->read_text_finish(result);
    auto & note = get_note();
    auto & buffer = note.get_buffer();

    // replace note content
    if(!newTitle.empty()) {
      Gtk::TextIter title_start = buffer->get_iter_at_offset(0);
      Gtk::TextIter title_end = title_start;
      title_end.forward_to_line_end();
      buffer->erase(title_start, title_end);
      buffer->insert(buffer->get_iter_at_offset(0), newTitle);
      title_end = title_start = buffer->get_iter_at_offset(0);
      title_end.forward_to_line_end();
      Glib::RefPtr<Gtk::TextTag> title_tag = buffer->get_tag_table()->lookup("note-title");
      buffer->apply_tag(title_tag, title_start, title_end);
      // in case the text was multile, new title is only the first line
      note.set_title(title_start.get_text(title_end));
    }
  });
}

}
