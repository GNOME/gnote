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


#ifndef __TABLEOFCONTENT_ACTION_HPP_
#define __TABLEOFCONTENT_ACTION_HPP_

namespace tableofcontents {

class TableofcontentsAction
  : public Gtk::Action
{
public:
  static Glib::RefPtr<Gtk::Action> create(const sigc::slot<void, Gtk::Menu*> & slot);
protected:
  virtual Gtk::Widget *create_menu_item_vfunc();
  virtual void on_activate();
private:
  TableofcontentsAction(const sigc::slot<void, Gtk::Menu*> & slot);
  void update_menu();
  void on_menu_hidden();

  bool m_submenu_built;
  Gtk::Menu *m_menu;
  sigc::slot<void, Gtk::Menu*> m_update_menu_slot;
};


}
#endif
