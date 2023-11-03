/*
 * "Table of Contents" is a Note add-in for Gnote.
 *  It lists note's table of contents in a menu.
 *
 * Copyright (C) 2013 Luc Pionchon <pionchon.luc@gmail.com>
 * Copyright (C) 2013,2015-2017,2019-2023 Aurimas Cernius <aurisc4@gmail.com>
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

/* A subclass of NoteAddin, implementing the Table of Contents add-in */

#include <glibmm/i18n.h>

#include <gtkmm/popovermenu.h>

#include "sharp/string.hpp"

#include "iactionmanager.hpp"
#include "ignote.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "notebuffer.hpp"
#include "popoverwidgets.hpp"
#include "utils.hpp"

#include "tableofcontents.hpp"
#include "tableofcontentsnoteaddin.hpp"
#include "tableofcontentsutils.hpp"

namespace tableofcontents {

TableofcontentsModule::TableofcontentsModule()
{
  ADD_INTERFACE_IMPL(TableofcontentsNoteAddin);
}


TableofcontentsNoteAddin::TableofcontentsNoteAddin()
{
}

void TableofcontentsNoteAddin::initialize () {}
void TableofcontentsNoteAddin::shutdown   () {}


void TableofcontentsNoteAddin::on_note_opened()
{
  register_main_window_action_callback("tableofcontents-heading1",
    sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_level_1_action));
  register_main_window_action_callback("tableofcontents-heading2",
    sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_level_2_action));
  register_main_window_action_callback("tableofcontents-help",
    sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_toc_help_action));
  register_main_window_action_callback("tableofcontents-goto-heading",
    sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_goto_heading));

  auto win = get_window();
  win->signal_foregrounded.connect(sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_foregrounded));

  auto & note = get_note();
  auto buffer = note.get_buffer();
  if(buffer) {
    buffer->signal_changed().connect(sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_note_changed));
  }

  {
    auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_1, Gdk::ModifierType::CONTROL_MASK);
    auto action = Gtk::NamedAction::create("win.tableofcontents-heading1");
    auto shortcut = Gtk::Shortcut::create(trigger, action);
    win->shortcut_controller().add_shortcut(shortcut);

    trigger = Gtk::KeyvalTrigger::create(GDK_KEY_2, Gdk::ModifierType::CONTROL_MASK);
    action = Gtk::NamedAction::create("win.tableofcontents-heading2");
    shortcut = Gtk::Shortcut::create(trigger, action);
    win->shortcut_controller().add_shortcut(shortcut);

    trigger = Gtk::KeyvalTrigger::create(GDK_KEY_1, Gdk::ModifierType::CONTROL_MASK | Gdk::ModifierType::ALT_MASK);
    auto cback_action = Gtk::CallbackAction::create(sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_toc_popup_activated));
    shortcut = Gtk::Shortcut::create(trigger, cback_action);
    win->shortcut_controller().add_shortcut(shortcut);
  }

  // Heading tags
  auto & tag_table = note.get_tag_table();
  m_tag_bold = tag_table->lookup("bold");
  m_tag_large = tag_table->lookup("size:large");
  m_tag_huge = tag_table->lookup("size:huge");
}


void TableofcontentsNoteAddin::on_foregrounded()
{
  auto host = get_window()->host();
  auto goto_action = host->find_action("tableofcontents-goto-heading");
  goto_action->set_state(Glib::Variant<gint32>::create(-1));
}


std::vector<gnote::PopoverWidget> TableofcontentsNoteAddin::get_actions_popover_widgets() const
{
  auto toc_menu = get_toc_menu();
  auto widgets = NoteAddin::get_actions_popover_widgets();
  auto toc_item = Gio::MenuItem::create(_("Table of Contents"), toc_menu);
  widgets.push_back(gnote::PopoverWidget(gnote::NOTE_SECTION_CUSTOM_SECTIONS, gnote::TABLE_OF_CONTENTS_ORDER, toc_item));
  return widgets;
}


Glib::RefPtr<Gio::Menu> TableofcontentsNoteAddin::get_toc_menu() const
{
  auto toc_menu = Gio::Menu::create();
  auto fixed_section = toc_menu;

  std::vector<Glib::RefPtr<Gio::MenuItem>> toc_items;
  get_toc_popover_items(toc_items);
  if(toc_items.size()) {
    auto items = Gio::Menu::create();
    for(auto & toc_button : toc_items) {
      items->append_item(toc_button);
    }

    toc_menu->append_section(items);
    fixed_section = Gio::Menu::create();
    toc_menu->append_section(fixed_section);
  }

  auto item = Gio::MenuItem::create(_("Heading 1"), "win.tableofcontents-heading1");
  fixed_section->append_item(item);

  item = Gio::MenuItem::create(_("Heading 2"), "win.tableofcontents-heading2");
  fixed_section->append_item(item);

  item = Gio::MenuItem::create(_("Table of Contents Help"), "win.tableofcontents-help");
  fixed_section->append_item(item);

  return toc_menu;
}


bool TableofcontentsNoteAddin::has_tag_over_range (Glib::RefPtr<Gtk::TextTag> tag, Gtk::TextIter start, Gtk::TextIter end) const
//return true if tag is set from start to end
{
  bool has = false;
  Gtk::TextIter iter = start;
  while (iter.compare(end) != 0 && (has = iter.has_tag(tag))){
    iter.forward_char();
  }
  return has;
}


Heading::Type TableofcontentsNoteAddin::get_heading_level_for_range (Gtk::TextIter start, Gtk::TextIter end) const
//return the heading level from start to end
{
  if (has_tag_over_range (m_tag_bold, start, end)) {

    if (has_tag_over_range (m_tag_huge , start, end)) {
        return Heading::Level_1;
    }
    else if (has_tag_over_range (m_tag_large, start, end)) {
        return Heading::Level_2;
    }
    else {
        return Heading::None;
    }
  }
  else {
      return Heading::None;
  }
}


void TableofcontentsNoteAddin::get_toc_items(std::vector<TocItem> & items) const
{
  Gtk::TextIter iter, iter_end, eol;

  //for each line of the buffer,
  //check if the full line has bold and (large or huge) tags
  auto & buffer = get_note().get_buffer();
  iter = buffer->begin();
  iter_end = buffer->end();

  while (iter != iter_end) {
    eol = iter;
    eol.forward_to_line_end();

    TocItem item;
    item.heading_level = get_heading_level_for_range (iter, eol);

    if (item.heading_level == Heading::Level_1 || item.heading_level == Heading::Level_2) {
      item.heading_position = iter.get_offset();
      item.heading          = iter.get_text(eol);

      items.push_back(item);
    }
    iter.forward_visible_line(); //next line
  }
}


void TableofcontentsNoteAddin::get_toc_popover_items(std::vector<Glib::RefPtr<Gio::MenuItem>> & items) const
{
  std::vector<TocItem> toc_items;

  get_toc_items(toc_items);
  if(toc_items.size()) {
    auto item = Gio::MenuItem::create(get_note().get_title(), "");
    item->set_action_and_target("win.tableofcontents-goto-heading", Glib::Variant<int>::create(0));
    items.push_back(item);
  }

  for(auto & toc_item : toc_items) {
    if(toc_item.heading_level == Heading::Level_2) {
      toc_item.heading = "→  " + toc_item.heading;
    }
    auto item = Gio::MenuItem::create(Glib::ustring(toc_item.heading), "");
    item->set_action_and_target("win.tableofcontents-goto-heading", Glib::Variant<int>::create(toc_item.heading_position));
    items.push_back(item);
  }
}


void TableofcontentsNoteAddin::on_level_1_activated()
{
  headification_switch (Heading::Level_1);
}

void TableofcontentsNoteAddin::on_level_2_activated()
{
  headification_switch (Heading::Level_2);
}

bool TableofcontentsNoteAddin::on_toc_popup_activated(Gtk::Widget & parent, const Glib::VariantBase&)
{
  auto editor = static_cast<gnote::NoteWindow&>(parent).editor();
  Gdk::Rectangle strong, weak;
  editor->get_cursor_locations(strong, weak);
  int x, y;
  editor->buffer_to_window_coords(Gtk::TextWindowType::TEXT, strong.get_x(), strong.get_y(), x, y);
  strong.set_x(x);
  strong.set_y(y);

  auto toc_menu = get_toc_menu();
  auto popover = gnote::utils::make_popover<Gtk::PopoverMenu>(*editor, toc_menu);
  popover->set_pointing_to(strong);
  popover->popup();
  return true;
}

void TableofcontentsNoteAddin::on_toc_help_activated()
{
  gnote::NoteWindow* window = get_window();
  gnote::utils::show_help("gnote", "addin-tableofcontents",
    *dynamic_cast<Gtk::Window*>(window->host()));
}


void TableofcontentsNoteAddin::on_level_1_action(const Glib::VariantBase&)
{
  on_level_1_activated();
  on_note_changed();
}

void TableofcontentsNoteAddin::on_level_2_action(const Glib::VariantBase&)
{
  on_level_2_activated();
  on_note_changed();
}

void TableofcontentsNoteAddin::on_toc_help_action(const Glib::VariantBase&)
{
  on_toc_help_activated();
}


void TableofcontentsNoteAddin::headification_switch (Heading::Type heading_request)
//apply the correct heading style to the current line(s) including the selection
//switch:  Level_1 <--> Level_2 <--> text
{
  auto & buffer = get_note().get_buffer();
  Gtk::TextIter start, end;
  Gtk::TextIter selection_start, selection_end;
  bool has_selection;

  //get selection
  has_selection = buffer->get_selection_bounds (start, end);
  selection_start = start;
  selection_end   = end;

  //grab the complete lines
  while (start.starts_line() == FALSE) {
    start.backward_char();
  }
  if (end.starts_line() && end != start){ // Home + Shift-down: don't take last line.
    end.backward_char();
  }
  while (end.ends_line() == FALSE) {
    end.forward_char();
  }

  //expand the selection to complete lines
  buffer->select_range (start, end);

  //set the heading tags
  Heading::Type current_heading = get_heading_level_for_range (start, end);

  buffer->remove_tag (m_tag_bold,  start, end);
  buffer->remove_tag (m_tag_large, start, end);
  buffer->remove_tag (m_tag_huge,  start, end);

  if( current_heading == Heading::Level_1 && heading_request == Heading::Level_2) { //existing vs requested
    buffer->set_active_tag ("bold");
    buffer->set_active_tag ("size:large");
  }
  else if( current_heading == Heading::Level_2 && heading_request == Heading::Level_1) {
    buffer->set_active_tag ("bold");
    buffer->set_active_tag ("size:huge");
  }
  else if( current_heading == Heading::None) {
    buffer->set_active_tag ("bold");
    buffer->set_active_tag ( (heading_request == Heading::Level_1)?"size:huge":"size:large");
  }

  //restore selection
  if (has_selection == TRUE) {
    buffer->select_range (selection_start, selection_end);
  }
}


void TableofcontentsNoteAddin::on_goto_heading(const Glib::VariantBase & param)
{
  int value = Glib::VariantBase::cast_dynamic<Glib::Variant<gint32>>(param).get();
  goto_heading(get_note(), value);
}


void TableofcontentsNoteAddin::on_note_changed()
{
  auto win = get_note().get_window();
  if(!win) {
    return;
  }
  win->signal_popover_widgets_changed();
}


} //namespace
