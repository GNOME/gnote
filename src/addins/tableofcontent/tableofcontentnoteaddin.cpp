/*
 * "Table of Content" is a Note add-in for Gnote.
 *  It lists Note's table of content in a menu.
 *
 * Copyright (C) 2013 Luc Pionchon <pionchon.luc@gmail.com>
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

/* A subclass of NoteAddin, implementing the Table of Content add-in */

#include <glibmm/i18n.h>

#include <gtkmm/stock.h>
#include <gtkmm/separatormenuitem.h>

#include "sharp/string.hpp"

#include "notemanager.hpp"
#include "notewindow.hpp"
#include "notebuffer.hpp"
#include "utils.hpp"

#include "tableofcontent.hpp"
#include "tableofcontentnoteaddin.hpp"
#include "tableofcontentmenuitem.hpp"

namespace tableofcontent {

TableofcontentModule::TableofcontentModule()
{
  ADD_INTERFACE_IMPL(TableofcontentNoteAddin);
}


TableofcontentNoteAddin::TableofcontentNoteAddin()
  : m_menu_item      (NULL)
  , m_toc_menu       (NULL)
  , m_toc_menu_built (false)
{
}

void TableofcontentNoteAddin::initialize () {}
void TableofcontentNoteAddin::shutdown   () {}


Gtk::ImageMenuItem * new_toc_menu_item ()
//create a menu item like: "[]_Table_of_Content______Ctrl-Alt-1__>"
{
  Gtk::ImageMenuItem * menu_item = manage(new Gtk::ImageMenuItem ());
  menu_item->set_image(*manage(new Gtk::Image(Gtk::Stock::JUMP_TO, Gtk::ICON_SIZE_MENU)));

  Gtk::AccelLabel *acclabel = manage(new Gtk::AccelLabel(_("Table of Content")));
  acclabel->set_alignment (Gtk::ALIGN_START);
  /* I don't have gtkmm-3.6, but I have gtk-3.6 */
  /* TO UNCOMMENT *///acclabel->set_accel (GDK_KEY_1, Gdk::CONTROL_MASK | Gdk::MOD1_MASK);
  /* TO DELETE    */gtk_accel_label_set_accel (acclabel->gobj (),GDK_KEY_1, GdkModifierType (GDK_CONTROL_MASK | GDK_MOD1_MASK));
  acclabel->show ();

  menu_item->add (*acclabel);

  return menu_item;
}


void TableofcontentNoteAddin::on_note_opened ()
{
  // TOC menu
  m_toc_menu = manage(new Gtk::Menu());
  m_toc_menu->signal_hide().connect(
                sigc::mem_fun(*this, &TableofcontentNoteAddin::on_menu_hidden));
  m_toc_menu->show_all ();

  m_menu_item = new_toc_menu_item ();
  m_menu_item->set_submenu(*m_toc_menu);
  m_menu_item->signal_activate().connect(
                 sigc::mem_fun(*this, &TableofcontentNoteAddin::on_menu_item_activated));
  m_menu_item->show ();

  add_plugin_menu_item (m_menu_item);

  // Reacts to key press events
  get_note()->get_window()->signal_key_press_event().connect(
    sigc::mem_fun(*this, &TableofcontentNoteAddin::on_key_pressed));

  // TOC can show up also in the contextual menu
  get_note()->get_window()->editor()->signal_populate_popup().connect(
    sigc::mem_fun(*this, &TableofcontentNoteAddin::on_populate_popup));

  // Header tags
  m_tag_bold  = get_note()->get_tag_table()->lookup ("bold");
  m_tag_large = get_note()->get_tag_table()->lookup ("size:large");
  m_tag_huge  = get_note()->get_tag_table()->lookup ("size:huge");
}


void TableofcontentNoteAddin::on_menu_item_activated ()
{
  if(m_toc_menu_built) {
    return;
  }
  populate_toc_menu (m_toc_menu);
  m_toc_menu_built = true;
}


void TableofcontentNoteAddin::on_menu_hidden ()
{
  m_toc_menu_built = false; //force the submenu to rebuild next time it's supposed to show
}


void TableofcontentNoteAddin::populate_toc_menu (Gtk::Menu *toc_menu, bool has_action_entries)
//populate a menu with Note's table of content
{
  // Clear out the old list
  std::vector<Gtk::Widget*> menu_items = toc_menu->get_children();
  for(std::vector<Gtk::Widget*>::reverse_iterator iter = menu_items.rbegin();
      iter != menu_items.rend(); ++iter) {
    toc_menu->remove(**iter);
  }

  // Build a new list
  std::list<TableofcontentMenuItem*> items;
  get_tableofcontent_menu_items(items);

  for(std::list<TableofcontentMenuItem*>::iterator iter = items.begin();
      iter != items.end(); ++iter) {
    TableofcontentMenuItem *item(*iter);
    item->show_all();
    toc_menu->append(*item);
  }

  // Action menu items, or nothing
  if (has_action_entries == false) {
    if (toc_menu->get_children().size() == 0) { // no toc items, and no action entries = empty menu
      Gtk::MenuItem *item = manage(new Gtk::MenuItem(_("(empty table of content)")));
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

    item = manage(new Gtk::MenuItem (_("Header Level 1")));
    item->add_accelerator("activate", get_note()->get_window()->get_accel_group(), GDK_KEY_1, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    item->signal_activate().connect(sigc::mem_fun(*this, &TableofcontentNoteAddin::on_level_1_activated));
    item->show ();
    toc_menu->append(*item);

    item = manage(new Gtk::MenuItem (_("Header Level 2")));
    item->add_accelerator("activate", get_note()->get_window()->get_accel_group(), GDK_KEY_2, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    item->signal_activate().connect(sigc::mem_fun(*this, &TableofcontentNoteAddin::on_level_2_activated));
    item->show ();
    toc_menu->append(*item);

    item = manage(new Gtk::MenuItem (_("Table of Content Help")));
    item->signal_activate().connect(sigc::mem_fun(*this, &TableofcontentNoteAddin::on_toc_help_activated));
    item->show ();
    toc_menu->append(*item);
  }

}


void TableofcontentNoteAddin::on_populate_popup (Gtk::Menu* popup_menu)
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


bool TableofcontentNoteAddin::has_tag_over_range (Glib::RefPtr<Gtk::TextTag> tag, Gtk::TextIter start, Gtk::TextIter end)
//return true if tag is set from start to end
{
  bool has = false;
  Gtk::TextIter iter = start;
  while (iter.compare(end) != 0 && (has = iter.has_tag(tag))){
    iter.forward_char();
  }
  return has;
}


Header::Type TableofcontentNoteAddin::get_header_level_for_range (Gtk::TextIter start, Gtk::TextIter end)
//return the header level from start to end
{
  if (has_tag_over_range (m_tag_bold, start, end)) {

    if (has_tag_over_range (m_tag_huge , start, end)) {
        return Header::Level_1;
    }
    else if (has_tag_over_range (m_tag_large, start, end)) {
        return Header::Level_2;
    }
    else {
        return Header::None;
    }
  }
  else {
      return Header::None;
  }
}


void TableofcontentNoteAddin::get_tableofcontent_menu_items(std::list<TableofcontentMenuItem*> & items)
//go through the note text, and list all lines tagged as header,
//and for each header, create a new TableofcontentMenuItem.
{
  TableofcontentMenuItem *item = NULL;

  std::string header;
  Header::Type header_level;
  int         header_position;

  Gtk::TextIter iter, iter_end, eol;

  //for each line of the buffer,
  //check if the full line has bold and (large or huge) tags
  iter     = get_note()->get_buffer()->begin();
  iter_end = get_note()->get_buffer()->end();

  while (iter != iter_end) {
    eol = iter;
    eol.forward_to_line_end();

    header_level = get_header_level_for_range (iter, eol);

    if (header_level == Header::Level_1 || header_level == Header::Level_2) {
      header_position = iter.get_offset();
      header = iter.get_text(eol);

      if (items.size() == 0) {
        //It's the first header found,
        //we also insert an entry linked to the Note's title:
        item = manage(new TableofcontentMenuItem (get_note(), get_note()->get_title(), Header::Title, 0));
        items.push_back(item);
      }
      item = manage(new TableofcontentMenuItem (get_note(), header, header_level, header_position));
      items.push_back(item);
    }
    iter.forward_visible_line(); //next line
  }
}

void TableofcontentNoteAddin::on_level_1_activated()
{
  headification_switch (Header::Level_1);
}
void TableofcontentNoteAddin::on_level_2_activated()
{
  headification_switch (Header::Level_2);
}
void TableofcontentNoteAddin::on_toc_popup_activated()
{
  if(m_toc_menu_built == false) {
    populate_toc_menu(m_toc_menu, false);
    m_toc_menu_built = true;
  }
  m_toc_menu->popup(0, 0);
}
void TableofcontentNoteAddin::on_toc_help_activated()
{
  gnote::NoteWindow* window = get_note()->get_window();
  gnote::utils::show_help("gnote", "addin-tableofcontent",
    window->get_screen()->gobj(), dynamic_cast<Gtk::Window*>(window->host()));
}


bool TableofcontentNoteAddin::on_key_pressed(GdkEventKey *ev)
//return true if signal handled, false otherwise
//NOTE: if a menu item has an accelerator,
//      its entry is needed until the toc menu is built a first time,
//      then the menu item accelerator takes the signals.
{
  switch(ev->keyval) {

  case GDK_KEY_1:
      if (ev->state == (GDK_CONTROL_MASK | GDK_MOD1_MASK)) {// Ctrl-Alt-1
        on_toc_popup_activated();
        return true;
      }
      else if (ev->state == GDK_CONTROL_MASK) { // Ctrl-1
        on_level_1_activated ();
        return true;
      }
      else {
        return false;
      }
  break;

  case GDK_KEY_2:
      if (ev->state == GDK_CONTROL_MASK) { // Ctrl-2
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


void TableofcontentNoteAddin::headification_switch (Header::Type header_request)
//apply the correct header style to the current selection
//switch:  Level_1 <--> Level_2 <--> text
{
  Glib::RefPtr<gnote::NoteBuffer> buffer = get_note()->get_buffer();
  Gtk::TextIter start, end;

  buffer->get_selection_bounds (start, end);

  Header::Type current_header = get_header_level_for_range (start, end);

  buffer->remove_all_tags (start, end);//reset all tags

  if( current_header == Header::Level_1 && header_request == Header::Level_2) { //existing vs requested
    buffer->set_active_tag ("bold");
    buffer->set_active_tag ("size:large");
  }
  else if( current_header == Header::Level_2 && header_request == Header::Level_1) {
    buffer->set_active_tag ("bold");
    buffer->set_active_tag ("size:huge");
  }
  else if( current_header == Header::None) {
    buffer->set_active_tag ("bold");
    buffer->set_active_tag ( (header_request == Header::Level_1)?"size:huge":"size:large");
  }

}


} //namespace