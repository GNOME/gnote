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

/* A subclass of NoteAddin, implementing the Table of Content add-in */

#ifndef __TABLEOFCONTENT_NOTEADDIN_HPP_
#define __TABLEOFCONTENT_NOTEADDIN_HPP_

#include <list>

#include <gtkmm/imagemenuitem.h>
#include <gtkmm/menu.h>

#include "sharp/dynamicmodule.hpp"
#include "note.hpp"
#include "noteaddin.hpp"

#include "tableofcontent.hpp"


namespace tableofcontent {

class TableofcontentModule : public sharp::DynamicModule
{
public:
  TableofcontentModule();
};
DECLARE_MODULE(TableofcontentModule);

class TableofcontentMenuItem;


class TableofcontentNoteAddin : public gnote::NoteAddin
{
public:
  static TableofcontentNoteAddin *create()
    {
      return new TableofcontentNoteAddin;
    }
  TableofcontentNoteAddin();

  virtual void initialize ();
  virtual void shutdown ();
  virtual void on_note_opened ();

private:
  void update_menu(Gtk::Menu *menu);
  void on_menu_hidden();
  bool on_key_pressed (GdkEventKey *ev);
  void on_populate_popup (Gtk::Menu* popup_menu);
  void on_level_1_activated ();
  void on_level_2_activated ();
  void on_toc_popup_activated ();
  void on_toc_help_activated ();


  void populate_toc_menu (Gtk::Menu *toc_menu, bool has_action_entries = true);

  bool has_tag_over_range (Glib::RefPtr<Gtk::TextTag> tag, Gtk::TextIter start, Gtk::TextIter end);
  Header::Type get_header_level_for_range (Gtk::TextIter start, Gtk::TextIter end);

  void get_tableofcontent_menu_items (std::list<TableofcontentMenuItem*> & items);

  void headification_switch (Header::Type header_request);

  Gtk::Menu          *m_toc_menu;        // the TOC submenu, containing the TOC
  bool                m_toc_menu_built;  // whereas toc_menu is already built

  Glib::RefPtr<Gtk::TextTag> m_tag_bold; // the tags used to mark headers
  Glib::RefPtr<Gtk::TextTag> m_tag_large;
  Glib::RefPtr<Gtk::TextTag> m_tag_huge;
};


}

#endif
