/*
 * "Table of Contents" is a Note add-in for Gnote.
 *  It lists note's table of contents in a menu.
 *
 * Copyright (C) 2013 Luc Pionchon <pionchon.luc@gmail.com>
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

/* A subclass of ImageMenuItem to show a toc menu item */

#ifndef __TABLEOFCONTENT_MENU_ITEM_HPP_
#define __TABLEOFCONTENT_MENU_ITEM_HPP_

#include <string>
#include <gtkmm/imagemenuitem.h>

#include "note.hpp"
#include "tableofcontent.hpp"


namespace tableofcontent {

class TableofcontentAction
  : public Gtk::Action
{
public:
  static Glib::RefPtr<Gtk::Action> create(const sigc::slot<void, Gtk::Menu*> & slot);
protected:
  virtual Gtk::Widget *create_menu_item_vfunc();
  virtual void on_activate();
private:
  TableofcontentAction(const sigc::slot<void, Gtk::Menu*> & slot);
  void update_menu();
  void on_menu_hidden();

  bool m_submenu_built;
  Gtk::Menu *m_menu;
  sigc::slot<void, Gtk::Menu*> m_update_menu_slot;
};

class TableofcontentMenuItem : public Gtk::ImageMenuItem
{
public:
  TableofcontentMenuItem ( const gnote::Note::Ptr & note,
                           const std::string      & header,
                           Header::Type             header_level,
                           int                      header_position
                         );

protected:
  virtual void on_activate ();

private:
  gnote::Note::Ptr m_note;            //the Note referenced by the menu item
  int              m_header_position; //the position of the header in the Note
                                        // == offset in the GtkTextBuffer
};


}

#endif
