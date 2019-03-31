/*
 * gnote
 *
 * Copyright (C) 2019 Aurimas Cernius
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

#include "iactionmanager.hpp"
#include "popoverwidgets.hpp"

namespace gnote {

const int NOTE_SECTION_NEW = 1;
const int NOTE_SECTION_UNDO = 2;
const int NOTE_SECTION_CUSTOM_SECTIONS = 10;
const int NOTE_SECTION_FLAGS = 20;
const int NOTE_SECTION_ACTIONS = 30;

PopoverWidget PopoverWidget::create_for_app(int ord, Gtk::Widget *w)
{
  return PopoverWidget(IActionManager::APP_ACTION_MANAGE, ord, w);
}

PopoverWidget PopoverWidget::create_for_note(int ord, Gtk::Widget *w)
{
  return PopoverWidget(NOTE_SECTION_ACTIONS, ord, w);
}

PopoverWidget PopoverWidget::create_custom_section(Gtk::Widget *w)
{
  return PopoverWidget(IActionManager::APP_CUSTOM_SECTION, 0, w);
}

}

