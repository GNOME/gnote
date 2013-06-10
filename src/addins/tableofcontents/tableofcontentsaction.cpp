/*
 * "Table of Contents" is a Note add-in for Gnote.
 *  It lists note's table of contents in a menu.
 *
 * Copyright (C) 2013 Aurimas Cernius
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

#include <glibmm.h>
#include <glibmm/i18n.h>
#include <gtkmm.h>

#include "tableofcontentsaction.hpp"

namespace tableofcontents {


Glib::RefPtr<Gtk::Action> TableofcontentsAction::create(const sigc::slot<void, Gtk::Menu*> & slot)
{
  return Glib::RefPtr<Gtk::Action>(new TableofcontentsAction(slot));
}

TableofcontentsAction::TableofcontentsAction(const sigc::slot<void, Gtk::Menu*> & slot)
  : Gtk::Action("TableofcontentsAction", Gtk::Stock::JUMP_TO,
                                        _("Table of Contents"),
                                        _("Table of Contents")
                )
  , m_update_menu_slot(slot)
{
}

Gtk::Widget *TableofcontentsAction::create_menu_item_vfunc()
{
  m_submenu_built = false;
  Gtk::ImageMenuItem *menu_item = new Gtk::ImageMenuItem;
  m_menu = manage(new Gtk::Menu);
  m_menu->signal_hide().connect(
    sigc::mem_fun(*this, &TableofcontentsAction::on_menu_hidden));
  menu_item->set_submenu(*m_menu);
  return menu_item;
}

void TableofcontentsAction::on_activate()
{
  Gtk::Action::on_activate();
  update_menu();
}

void TableofcontentsAction::update_menu()
{
  m_update_menu_slot(m_menu);
  m_submenu_built = true;
}

void TableofcontentsAction::on_menu_hidden()
{
  m_submenu_built = false;
}




} //namespace
