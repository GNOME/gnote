/*
 * gnote
 *
 * Copyright (C) 2010-2011,2013-2014,2016 Aurimas Cernius
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
#include <gtkmm/modelbutton.h>

#include "sharp/string.hpp"
#include "backlinksnoteaddin.hpp"
#include "iactionmanager.hpp"
#include "mainwindow.hpp"
#include "notemanager.hpp"
#include "preferences.hpp"
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
  register_main_window_action_callback("backlinks-open-note",
    sigc::mem_fun(*this, &BacklinksNoteAddin::on_open_note));
}

void BacklinksNoteAddin::on_open_note(const Glib::VariantBase & param)
{
  Glib::ustring uri = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(param).get();
  gnote::NoteBase::Ptr note = get_note()->manager().find_by_uri(uri);
  if(note) {
    gnote::MainWindow::present_in_new_window(static_pointer_cast<gnote::Note>(note),
      gnote::Preferences::obj().get_schema_settings(gnote::Preferences::SCHEMA_GNOTE)->
        get_boolean(gnote::Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE));
  }
}

std::map<int, Gtk::Widget*> BacklinksNoteAddin::get_actions_popover_widgets() const
{
  auto widgets = NoteAddin::get_actions_popover_widgets();
  auto menu_button = gnote::utils::create_popover_submenu_button("backlinks-menu", _("What links here?"));
  gnote::utils::add_item_to_ordered_map(widgets, gnote::BACKLINKS_ORDER, menu_button);

  auto submenu = gnote::utils::create_popover_submenu("backlinks-menu");
  update_menu(submenu);
  gnote::utils::add_item_to_ordered_map(widgets, 100000, submenu);

  return widgets;
}

void BacklinksNoteAddin::update_menu(Gtk::Grid *menu) const
{
  std::list<Gtk::Widget*> items;
  get_backlink_menu_items(items);
  int top = 0;
  auto sub = manage(new Gtk::Grid);
  gnote::utils::set_common_popover_widget_props(*sub);
  int subtop = 0;
  for(auto item : items) {
    dynamic_cast<Gtk::ModelButton*>(item)->property_inverted() = true;
    sub->attach(*item, 0, subtop++, 1, 1);
  }

  // If nothing was found, add in a "dummy" item
  if(subtop == 0) {
    Gtk::Widget *blank_item = manage(gnote::utils::create_popover_button("win.backlinks-nonexistent", _("(none)")));
    sub->attach(*blank_item, 0, subtop++, 1, 1);
  }
  menu->attach(*sub, 0, top++, 1, 1);

  sub = manage(new Gtk::Grid);
  gnote::utils::set_common_popover_widget_props(*sub);
  subtop = 0;
  auto back = gnote::utils::create_popover_submenu_button("main", _("_Back"));
  dynamic_cast<Gtk::ModelButton*>(back)->property_inverted() = true;
  sub->attach(*back, 0, subtop++, 1, 1);
  menu->attach(*sub, 0, top++, 1, 1);
}


void BacklinksNoteAddin::get_backlink_menu_items(std::list<Gtk::Widget*> & items) const
{
  gnote::NoteBase::List notes = get_note()->manager().get_notes_linking_to(get_note()->get_title());
  FOREACH(const gnote::NoteBase::Ptr & note, notes) {
    if(note != get_note()) { // don't match ourself
      auto button = manage(gnote::utils::create_popover_button("win.backlinks-open-note", note->get_title()));
      gtk_actionable_set_action_target_value(GTK_ACTIONABLE(button->gobj()),
        Glib::Variant<Glib::ustring>::create(note->uri()).gobj());
      items.push_back(button);
    }
  }

  items.sort([](Gtk::Widget *x, Gtk::Widget *y)
    {
      return dynamic_cast<Gtk::ModelButton*>(x)->get_label() < dynamic_cast<Gtk::ModelButton*>(y)->get_label();
    });
}


bool BacklinksNoteAddin::check_note_has_match(const gnote::Note::Ptr & note, 
                                              const std::string & encoded_title)
{
  Glib::ustring note_text = note->xml_content();
  note_text = note_text.lowercase();
  return note_text.find(encoded_title) != Glib::ustring::npos;
}



}
