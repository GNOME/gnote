/*
 * gnote
 *
 * Copyright (C) 2012-2013 Aurimas Cernius
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


#ifndef __BACKLINK_MENU_ITEM_HPP_
#define __BACKLINK_MENU_ITEM_HPP_

#include <string>
#include <gtkmm/imagemenuitem.h>

#include "note.hpp"

namespace backlinks {

class BacklinkAction
  : public Gtk::Action
{
public:
  static Glib::RefPtr<Gtk::Action> create(const sigc::slot<void, Gtk::Menu*> & slot);

  virtual Gtk::Widget *create_menu_item_vfunc();
protected:
  virtual void on_activate();
private:
  BacklinkAction(const sigc::slot<void, Gtk::Menu*> & slot);
  void on_menu_hidden();
  void update_menu();

  Gtk::Menu *m_menu;
  bool m_submenu_built;
  sigc::slot<void, Gtk::Menu*> m_update_menu_slot;
};

class BacklinkMenuItem
  : public Gtk::ImageMenuItem
{
public:
  BacklinkMenuItem(const gnote::Note::Ptr &, const std::string &);
  
  gnote::Note::Ptr get_note()
    { return m_note; }
protected:
  virtual void on_activate();
private:
  gnote::Note::Ptr   m_note;
  std::string m_title_search;
  
  static Glib::RefPtr<Gdk::Pixbuf> get_note_icon();
};

}

#endif
