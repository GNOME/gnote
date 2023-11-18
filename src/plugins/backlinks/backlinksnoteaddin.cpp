/*
 * gnote
 *
 * Copyright (C) 2010-2011,2013-2014,2016-2017,2019-2023 Aurimas Cernius
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
#include <gtkmm/separator.h>

#include "sharp/string.hpp"
#include "backlinksnoteaddin.hpp"
#include "iactionmanager.hpp"
#include "ignote.hpp"
#include "mainwindow.hpp"
#include "notemanagerbase.hpp"
#include "preferences.hpp"

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
  manager().find_by_uri(uri, [this](gnote::NoteBase & note) {
    gnote::MainWindow::present_in_new_window(ignote(), static_cast<gnote::Note&>(note));
  });
}

std::vector<gnote::PopoverWidget> BacklinksNoteAddin::get_actions_popover_widgets() const
{
  auto submenu = Gio::Menu::create();
  update_menu(*submenu);

  auto widgets = NoteAddin::get_actions_popover_widgets();
  auto menu_button = Gio::MenuItem::create(_("What links here?"), submenu);
  widgets.push_back(gnote::PopoverWidget(gnote::NOTE_SECTION_CUSTOM_SECTIONS, gnote::BACKLINKS_ORDER, menu_button));

  return widgets;
}

void BacklinksNoteAddin::update_menu(Gio::Menu & menu) const
{
  auto items = get_backlink_menu_items();
  for(auto item : items) {
    menu.append_item(item);
  }

  // If nothing was found, add in a "dummy" item
  if(items.size() == 0) {
    auto blank_item = Gio::MenuItem::create(_("(none)"), "win.backlinks-nonexistent");
    menu.append_item(blank_item);
  }
}


std::vector<Glib::RefPtr<Gio::MenuItem>> BacklinksNoteAddin::get_backlink_menu_items() const
{
  auto & note = get_note();
  std::vector<gnote::NoteBase::Ref> notes(note.manager().get_notes_linking_to(note.get_title()));
  std::sort(notes.begin(), notes.end(), [](const gnote::NoteBase & x, const gnote::NoteBase & y)
    {
      return x.get_title() < y.get_title();
    });

  std::vector<Glib::RefPtr<Gio::MenuItem>> items;
  for(const gnote::NoteBase & n : notes) {
    if(&n != &note) { // don't match ourself
      auto item = Gio::MenuItem::create(n.get_title(), "");
      item->set_action_and_target("win.backlinks-open-note", Glib::Variant<Glib::ustring>::create(n.uri()));
      items.push_back(std::move(item));
    }
  }


  return items;
}


}
