/*
 * gnote
 *
 * Copyright (C) 2010-2011,2013-2014 Aurimas Cernius
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

#include "sharp/string.hpp"
#include "backlinksnoteaddin.hpp"
#include "backlinkmenuitem.hpp"
#include "iactionmanager.hpp"
#include "notemanager.hpp"
#include "utils.hpp"

namespace backlinks {

BacklinksModule::BacklinksModule()
{
  ADD_INTERFACE_IMPL(BacklinksNoteAddin);
}



BacklinksNoteAddin::BacklinksNoteAddin()
{
}


void BacklinksNoteAddin::initialize ()
{
}


void BacklinksNoteAddin::shutdown ()
{
}


void BacklinksNoteAddin::on_note_opened ()
{
  Glib::RefPtr<Gtk::Action> action = BacklinkAction::create(
    sigc::mem_fun(*this, &BacklinksNoteAddin::update_menu));
  add_note_action(action, gnote::BACKLINKS_ORDER);
}

void BacklinksNoteAddin::update_menu(Gtk::Menu *menu)
{
  //
  // Clear out the old list
  //
  std::vector<Gtk::Widget*> menu_items = menu->get_children();
  for(std::vector<Gtk::Widget*>::reverse_iterator iter = menu_items.rbegin();
      iter != menu_items.rend(); ++iter) {
    menu->remove(**iter);
  }

  //
  // Build a new list
  //
  std::list<BacklinkMenuItem*> items;
  get_backlink_menu_items(items);
  for(std::list<BacklinkMenuItem*>::iterator iter = items.begin();
      iter != items.end(); ++iter) {
    BacklinkMenuItem * item(*iter);
    item->show_all();
    menu->append (*item);
  }

  // If nothing was found, add in a "dummy" item
  if(menu->get_children().size() == 0) {
    Gtk::MenuItem *blank_item = manage(new Gtk::MenuItem(_("(none)")));
    blank_item->set_sensitive(false);
    blank_item->show_all ();
    menu->append(*blank_item);
  }
}


void BacklinksNoteAddin::get_backlink_menu_items(std::list<BacklinkMenuItem*> & items)
{
  gnote::NoteBase::List notes = get_note()->manager().get_notes_linking_to(get_note()->get_title());
  FOREACH(const gnote::NoteBase::Ptr & note, notes) {
    if(note != get_note()) { // don't match ourself
      BacklinkMenuItem *item = manage(new BacklinkMenuItem(note, get_note()->get_title()));

      items.push_back(item);
    }
  }

  items.sort();
}


bool BacklinksNoteAddin::check_note_has_match(const gnote::Note::Ptr & note, 
                                              const std::string & encoded_title)
{
  Glib::ustring note_text = note->xml_content();
  note_text = note_text.lowercase();
  return note_text.find(encoded_title) != Glib::ustring::npos;
}



}
