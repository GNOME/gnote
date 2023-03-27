/*
 * gnote
 *
 * Copyright (C) 2019,2021-2022 Aurimas Cernius
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

#include <gtkmm/label.h>

#include "iactionmanager.hpp"
#include "popoverwidgets.hpp"

namespace gnote {

const int APP_SECTION_NEW = 1;
const int APP_SECTION_MANAGE = 2;
const int APP_SECTION_LAST = 3;
const int APP_CUSTOM_SECTION = 1000;

const int NOTE_SECTION_NEW = 1;
const int NOTE_SECTION_UNDO = 2;
const int NOTE_SECTION_CUSTOM_SECTIONS = 10;
const int NOTE_SECTION_FLAGS = 20;
const int NOTE_SECTION_ACTIONS = 30;

PopoverWidget PopoverWidget::create_for_app(int ord, const Glib::RefPtr<Gio::MenuItem> w)
{
  return PopoverWidget(APP_SECTION_MANAGE, ord, w);
}

PopoverWidget PopoverWidget::create_for_note(int ord, const Glib::RefPtr<Gio::MenuItem> w)
{
  return PopoverWidget(NOTE_SECTION_ACTIONS, ord, w);
}

PopoverWidget PopoverWidget::create_custom_section(const Glib::RefPtr<Gio::MenuItem> w)
{
  return PopoverWidget(APP_CUSTOM_SECTION, 0, w);
}


namespace utils {

  void unparent_popover_on_close(Gtk::Popover *popover)
  {
    auto unparent_fn = [](gpointer data)
    {
      static_cast<Gtk::Popover*>(data)->unparent();
    };
    auto callback = [popover, unparent_fn]
    {
      // unparenting at once breaks popover menu - actions not invoked
      // unparent with small delay
      g_timeout_add_once(1, unparent_fn, popover);
    };
    popover->signal_closed().connect(callback);
  }

}

}

