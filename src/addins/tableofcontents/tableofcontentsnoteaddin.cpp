/*
 * "Table of Contents" is a Note add-in for Gnote.
 *  It lists note's table of contents in a menu.
 *
 * Copyright (C) 2013 Luc Pionchon <pionchon.luc@gmail.com>
 * Copyright (C) 2013,2015-2016 Aurimas Cernius <aurisc4@gmail.com>
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

#include <gtkmm/modelbutton.h>
#include <gtkmm/stock.h>
#include <gtkmm/separatormenuitem.h>

#include "sharp/string.hpp"

#include "iactionmanager.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "notebuffer.hpp"
#include "utils.hpp"

#include "tableofcontents.hpp"
#include "tableofcontentsnoteaddin.hpp"
#include "tableofcontentsmenuitem.hpp"
#include "tableofcontentsutils.hpp"

namespace tableofcontents {

TableofcontentsModule::TableofcontentsModule()
{
  ADD_INTERFACE_IMPL(TableofcontentsNoteAddin);
}


TableofcontentsNoteAddin::TableofcontentsNoteAddin()
  : m_toc_menu       (NULL)
  , m_toc_menu_built (false)
{
}

void TableofcontentsNoteAddin::initialize () {}
void TableofcontentsNoteAddin::shutdown   () {}


Gtk::ImageMenuItem * new_toc_menu_item ()
//create a menu item like: "[]_Table_of_Contents______Ctrl-Alt-1__>"
{
  Gtk::ImageMenuItem * menu_item = manage(new Gtk::ImageMenuItem ());
  menu_item->set_image(*manage(new Gtk::Image(Gtk::Stock::JUMP_TO, Gtk::ICON_SIZE_MENU)));

  Gtk::AccelLabel *acclabel = manage(new Gtk::AccelLabel(_("Table of Contents")));
  acclabel->set_alignment (Gtk::ALIGN_START);
  /* I don't have gtkmm-3.6, but I have gtk-3.6 */
  /* TO UNCOMMENT *///acclabel->set_accel (GDK_KEY_1, Gdk::CONTROL_MASK | Gdk::MOD1_MASK);
  /* TO DELETE    */gtk_accel_label_set_accel (acclabel->gobj (),GDK_KEY_1, GdkModifierType (GDK_CONTROL_MASK | GDK_MOD1_MASK));
  acclabel->show ();

  menu_item->add (*acclabel);

  return menu_item;
}


void TableofcontentsNoteAddin::on_note_opened ()
{
  m_toc_menu = manage(new Gtk::Menu);
  m_toc_menu->signal_hide().connect(
    sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_menu_hidden));

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

  auto buffer = get_note()->get_buffer();
  if(buffer) {
    buffer->signal_changed().connect(sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_note_changed));
  }

  // Reacts to key press events
  win->signal_key_press_event().connect(
    sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_key_pressed));

  // TOC can show up also in the contextual menu
  win->editor()->signal_populate_popup().connect(
    sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_populate_popup));

  // Heading tags
  m_tag_bold  = get_note()->get_tag_table()->lookup ("bold");
  m_tag_large = get_note()->get_tag_table()->lookup ("size:large");
  m_tag_huge  = get_note()->get_tag_table()->lookup ("size:huge");
}


void TableofcontentsNoteAddin::on_foregrounded()
{
  auto host = get_window()->host();
  auto goto_action = host->find_action("tableofcontents-goto-heading");
  goto_action->set_state(Glib::Variant<gint32>::create(-1));
}


std::map<int, Gtk::Widget*> TableofcontentsNoteAddin::get_actions_popover_widgets() const
{
  auto widgets = NoteAddin::get_actions_popover_widgets();
  auto toc_item = gnote::utils::create_popover_submenu_button("tableofcontents-menu", _("Table of Contents"));
  gnote::utils::add_item_to_ordered_map(widgets, gnote::TABLE_OF_CONTENTS_ORDER, toc_item);
  auto toc_menu = gnote::utils::create_popover_submenu("tableofcontents-menu");
  gnote::utils::add_item_to_ordered_map(widgets, 100000, toc_menu);

  int top = 0;
  int sub_top = 0;
  Gtk::Grid *sub = manage(gnote::utils::create_popover_inner_grid());
  std::vector<Gtk::Widget*> toc_items;
  get_toc_popover_items(toc_items);
  if(toc_items.size()) {
    for(auto & toc_button : toc_items) {
      sub->attach(*toc_button, 1, sub_top++, 1, 1);
    }

    toc_menu->attach(*sub, 0, top++, 1, 1);
    sub = manage(gnote::utils::create_popover_inner_grid(&sub_top));
  }

  auto item = manage(gnote::utils::create_popover_button("win.tableofcontents-heading1", _("Heading 1")));
  item->add_accelerator("activate", get_window()->get_accel_group(), GDK_KEY_1, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
  sub->attach(*item, 0, sub_top++, 1, 1);

  item = manage(gnote::utils::create_popover_button("win.tableofcontents-heading2", _("Heading 2")));
  item->add_accelerator("activate", get_window()->get_accel_group(), GDK_KEY_2, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
  sub->attach(*item, 0, sub_top++, 1, 1);

  item = manage(gnote::utils::create_popover_button("win.tableofcontents-help", _("Table of Contents Help")));
  sub->attach(*item, 0, sub_top++, 1, 1);
  toc_menu->attach(*sub, 0, top++, 1, 1);

  sub = manage(gnote::utils::create_popover_inner_grid(&sub_top));
  auto back_item = gnote::utils::create_popover_submenu_button("main", _("_Back"));
  dynamic_cast<Gtk::ModelButton*>(back_item)->property_inverted() = true;
  sub->attach(*back_item, 0, sub_top++, 1, 1);
  toc_menu->attach(*sub, 0, top++, 1, 1);

  return widgets;
}


void TableofcontentsNoteAddin::on_menu_hidden()
{
  m_toc_menu_built = false; //force the submenu to rebuild next time it's supposed to show
}


void TableofcontentsNoteAddin::populate_toc_menu (Gtk::Menu *toc_menu, bool has_action_entries)
//populate a menu with Note's table of contents
{
  // Clear out the old list
  std::vector<Gtk::Widget*> menu_items = toc_menu->get_children();
  for(std::vector<Gtk::Widget*>::reverse_iterator iter = menu_items.rbegin();
      iter != menu_items.rend(); ++iter) {
    toc_menu->remove(**iter);
  }

  // Build a new list
  std::list<TableofcontentsMenuItem*> items;
  get_tableofcontents_menu_items(items);

  for(std::list<TableofcontentsMenuItem*>::iterator iter = items.begin();
      iter != items.end(); ++iter) {
    TableofcontentsMenuItem *item(*iter);
    item->show_all();
    toc_menu->append(*item);
  }

  // Action menu items, or nothing
  if (has_action_entries == false) {
    if (toc_menu->get_children().size() == 0) { // no toc items, and no action entries = empty menu
      Gtk::MenuItem *item = manage(new Gtk::MenuItem(_("(empty table of contents)")));
      item->set_sensitive(false);
      item->show();
      toc_menu->append(*item);
    }
  }
  else {
    Gtk::MenuItem *item;

    if (toc_menu->get_children().size() != 0) { //there are toc items, we add a separator
      item = manage(new Gtk::SeparatorMenuItem ());
      item->show ();
      toc_menu->append(*item);
    }

    item = manage(new Gtk::MenuItem (_("Heading 1")));
    item->add_accelerator("activate", get_window()->get_accel_group(), GDK_KEY_1, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    item->signal_activate().connect(sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_level_1_activated));
    item->show ();
    toc_menu->append(*item);

    item = manage(new Gtk::MenuItem (_("Heading 2")));
    item->add_accelerator("activate", get_window()->get_accel_group(), GDK_KEY_2, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    item->signal_activate().connect(sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_level_2_activated));
    item->show ();
    toc_menu->append(*item);

    item = manage(new Gtk::MenuItem (_("Table of Contents Help")));
    item->signal_activate().connect(sigc::mem_fun(*this, &TableofcontentsNoteAddin::on_toc_help_activated));
    item->show ();
    toc_menu->append(*item);
  }

}


void TableofcontentsNoteAddin::on_populate_popup (Gtk::Menu* popup_menu)
//prepened a toc submenu in the contextual menu
{
  Gtk::Menu *toc_menu = manage(new Gtk::Menu());
  populate_toc_menu (toc_menu);

  Gtk::SeparatorMenuItem *separator = manage(new Gtk::SeparatorMenuItem ());
  separator->show ();
  popup_menu->prepend (*separator);

  Gtk::ImageMenuItem *menu_item = new_toc_menu_item ();
  menu_item->set_submenu (*toc_menu);
  menu_item->show ();

  popup_menu->prepend (*menu_item);
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
  iter     = get_note()->get_buffer()->begin();
  iter_end = get_note()->get_buffer()->end();

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


void TableofcontentsNoteAddin::get_tableofcontents_menu_items(std::list<TableofcontentsMenuItem*> & items)
//go through the note text, and list all lines tagged as heading,
//and for each heading, create a new TableofcontentsMenuItem.
{
  TableofcontentsMenuItem *item = NULL;
  std::vector<TocItem> toc_items;

  get_toc_items(toc_items);
  if(toc_items.size()) {
    //If we have at least one heading
    //we also insert an entry linked to the Note's title:
    item = manage(new TableofcontentsMenuItem(get_note(), get_note()->get_title(), Heading::Title, 0));
    items.push_back(item);
  }

  for(auto & toc_item : toc_items) {
    item = manage(new TableofcontentsMenuItem(get_note(), toc_item.heading, toc_item.heading_level, toc_item.heading_position));
    items.push_back(item);
  }
}


void TableofcontentsNoteAddin::get_toc_popover_items(std::vector<Gtk::Widget*> & items) const
{
  std::vector<TocItem> toc_items;

  get_toc_items(toc_items);
  if(toc_items.size()) {
    auto item = dynamic_cast<Gtk::ModelButton*>(gnote::utils::create_popover_button("win.tableofcontents-goto-heading", ""));
    Gtk::Label *label = (Gtk::Label*)item->get_child();
    label->set_markup("<b>" + get_note()->get_title() + "</b>");
    gtk_actionable_set_action_target_value(GTK_ACTIONABLE(item->gobj()), g_variant_new_int32(0));
    item->property_role() = Gtk::BUTTON_ROLE_NORMAL;
    item->property_inverted() = true;
    item->property_centered() = false;
    items.push_back(item);
  }

  for(auto & toc_item : toc_items) {
    if(toc_item.heading_level == Heading::Level_2) {
      toc_item.heading = "→  " + toc_item.heading;
    }
    auto item = dynamic_cast<Gtk::ModelButton*>(gnote::utils::create_popover_button("win.tableofcontents-goto-heading", toc_item.heading));
    if(toc_item.heading_level == Heading::Level_1) {
      item->set_image(*manage(new Gtk::Image(Gtk::Stock::GO_FORWARD, Gtk::ICON_SIZE_MENU)));
    }
    gtk_actionable_set_action_target_value(GTK_ACTIONABLE(item->gobj()), g_variant_new_int32(toc_item.heading_position));
    item->property_role() = Gtk::BUTTON_ROLE_NORMAL;
    item->property_inverted() = true;
    item->property_centered() = false;
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
void TableofcontentsNoteAddin::on_toc_popup_activated()
{
  if(m_toc_menu_built == false) {
    populate_toc_menu(m_toc_menu, false);
    m_toc_menu_built = true;
  }
  m_toc_menu->popup(0, 0);
}
void TableofcontentsNoteAddin::on_toc_help_activated()
{
  gnote::NoteWindow* window = get_window();
  gnote::utils::show_help("gnote", "addin-tableofcontents",
    window->get_screen()->gobj(), dynamic_cast<Gtk::Window*>(window->host()));
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


bool TableofcontentsNoteAddin::on_key_pressed(GdkEventKey *ev)
//return true if signal handled, false otherwise
//NOTE: if a menu item has an accelerator,
//      its entry is needed until the toc menu is built a first time,
//      then the menu item accelerator takes the signals.
{
  switch(ev->keyval) {

  case GDK_KEY_1:
      if (ev->state & Gdk::CONTROL_MASK
       && ev->state & Gdk::MOD1_MASK    ) {// Ctrl-Alt-1
        on_toc_popup_activated();
        return true;
      }
      else if (ev->state & Gdk::CONTROL_MASK) { // Ctrl-1
        on_level_1_activated ();
        return true;
      }
      else {
        return false;
      }
  break;

  case GDK_KEY_2:
      if (ev->state & Gdk::CONTROL_MASK) { // Ctrl-2
        on_level_2_activated ();
        return true;
      }
      else {
        return false;
      }
  break;

  default:
    return false;
  }

  return false;
}


void TableofcontentsNoteAddin::headification_switch (Heading::Type heading_request)
//apply the correct heading style to the current line(s) including the selection
//switch:  Level_1 <--> Level_2 <--> text
{
  Glib::RefPtr<gnote::NoteBuffer> buffer = get_note()->get_buffer();
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
  auto win = get_note()->get_window();
  if(!win) {
    return;
  }
  win->signal_popover_widgets_changed();
}


} //namespace
