/*
 * gnote
 *
 * Copyright (C) 2013,2016 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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

#include "debug.hpp"
#include "iactionmanager.hpp"
#include "itagmanager.hpp"
#include "notewindow.hpp"
#include "readonlynoteaddin.hpp"
#include "tag.hpp"


namespace readonly {

DECLARE_MODULE(ReadOnlyModule);

ReadOnlyModule::ReadOnlyModule()
  : sharp::DynamicModule()
{
  ADD_INTERFACE_IMPL(ReadOnlyNoteAddin);
  enabled(false);
}


ReadOnlyNoteAddin::ReadOnlyNoteAddin()
  : NoteAddin()
{
}

ReadOnlyNoteAddin::~ReadOnlyNoteAddin()
{
}

void ReadOnlyNoteAddin::initialize()
{
}

void ReadOnlyNoteAddin::shutdown()
{
}

void ReadOnlyNoteAddin::on_note_opened()
{
  get_window()->signal_foregrounded.connect(sigc::mem_fun(*this, &ReadOnlyNoteAddin::on_foreground));
  get_window()->signal_backgrounded.connect(sigc::mem_fun(*this, &ReadOnlyNoteAddin::on_background));
}

std::map<int, Gtk::Widget*> ReadOnlyNoteAddin::get_actions_popover_widgets() const
{
  auto widgets = NoteAddin::get_actions_popover_widgets();
  auto button = gnote::utils::create_popover_button("win.readonly-toggle", _("Read Only"));
  gnote::utils::add_item_to_ordered_map(widgets, gnote::READ_ONLY_ORDER, button);
  return widgets;
}

void ReadOnlyNoteAddin::on_foreground()
{
  auto action = get_window()->host()->find_action("readonly-toggle");
  gnote::ITagManager & m = gnote::ITagManager::obj();
  const gnote::Tag::Ptr ro_tag = m.get_or_create_system_tag("read-only");

  m_readonly_toggle_cid = action->signal_change_state()
    .connect(sigc::mem_fun(*this, &ReadOnlyNoteAddin::on_menu_item_toggled));
  action->change_state(Glib::Variant<bool>::create(get_note()->contains_tag(ro_tag)));
}

void ReadOnlyNoteAddin::on_background()
{
  m_readonly_toggle_cid.disconnect();
}

void ReadOnlyNoteAddin::on_menu_item_toggled(const Glib::VariantBase & state)
{
  gnote::ITagManager & m = gnote::ITagManager::obj();
  const gnote::Tag::Ptr ro_tag = m.get_or_create_system_tag("read-only");
  bool read_only = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(state).get();
  auto action = get_window()->host()->find_action("readonly-toggle");
  action->set_state(state);
  if(read_only) {
    get_note()->enabled(false);
    get_note()->add_tag(ro_tag);
  }
  else {
    get_note()->enabled(true);
    get_note()->remove_tag(ro_tag);
  }
}

}
