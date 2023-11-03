/*
 * gnote
 *
 * Copyright (C) 2013,2017,2023 Aurimas Cernius
 * Copyright (c) 2009 Romain Tartière <romain@blogreen.org>
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


#include "notetag.hpp"
#include "todonoteaddin.hpp"


namespace todo {

static std::vector<Glib::ustring> s_todo_patterns;

TodoModule::TodoModule()
{
  if(s_todo_patterns.size() == 0) {
    s_todo_patterns.push_back("FIXME");
    s_todo_patterns.push_back("TODO");
    s_todo_patterns.push_back("XXX");
  }

  ADD_INTERFACE_IMPL(Todo);
}



void Todo::initialize()
{
  auto & tag_table = get_note().get_tag_table();
  for(auto s : s_todo_patterns) {
    if(!tag_table->lookup(s)) {
      Glib::RefPtr<Gtk::TextTag> tag = gnote::NoteTag::create(Glib::ustring(s), gnote::NoteTag::NO_FLAG);
      tag->property_foreground() = "#0080f0";
      tag->property_weight() = PANGO_WEIGHT_BOLD;
      tag->property_underline() = Pango::Underline::SINGLE;
      tag_table->add(tag);
    }
  }
}

void Todo::shutdown()
{
}

void Todo::on_note_opened()
{
  get_buffer()->signal_insert().connect(sigc::mem_fun(*this, &Todo::on_insert_text));
  get_buffer()->signal_erase().connect(sigc::mem_fun(*this, &Todo::on_delete_range));

  highlight_note();
}

void Todo::on_insert_text(const Gtk::TextIter & pos, const Glib::ustring & /*text*/, int /*bytes*/)
{
  highlight_region(pos, pos);
}

void Todo::on_delete_range(const Gtk::TextBuffer::iterator & start, const Gtk::TextBuffer::iterator & end)
{
  highlight_region(start, end);
}

void Todo::highlight_note()
{
  Gtk::TextIter start = get_buffer()->get_iter_at_offset(0);
  Gtk::TextIter end = start;
  end.forward_to_end();
  highlight_region(start, end);
}

void Todo::highlight_region(Gtk::TextIter start, Gtk::TextIter end)
{
  if(!start.starts_line()) {
    start.backward_line();
  }
  if(!end.ends_line()) {
    end.forward_line();
  }

  for(auto pattern : s_todo_patterns) {
    highlight_region(pattern, start, end);
  }    
}

void Todo::highlight_region(const Glib::ustring & pattern, Gtk::TextIter start, Gtk::TextIter end)
{
  get_buffer()->remove_tag_by_name(pattern, start, end);
  Gtk::TextIter region_start = start;
  while(start.forward_search(pattern + ":", Gtk::TextSearchFlags::TEXT_ONLY, region_start, start, end)) {
    Gtk::TextIter region_end = start;
    get_buffer()->apply_tag_by_name(pattern, region_start, region_end);
  }
}

}

