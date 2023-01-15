/*
 * "Table of Contents" is a Note add-in for Gnote.
 *  It lists note's table of contents in a menu.
 *
 * Copyright (C) 2013 Luc Pionchon <pionchon.luc@gmail.com>
 * Copyright (C) 2013,2015-2016,2019,2023 Aurimas Cernius <aurisc4@gmail.com>
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

/* A subclass of NoteAddin, implementing the Table of Contents add-in */

#ifndef __TABLEOFCONTENT_NOTEADDIN_HPP_
#define __TABLEOFCONTENT_NOTEADDIN_HPP_

#include "sharp/dynamicmodule.hpp"
#include "note.hpp"
#include "noteaddin.hpp"

#include "tableofcontents.hpp"


namespace tableofcontents {

class TableofcontentsModule : public sharp::DynamicModule
{
public:
  TableofcontentsModule();
};
DECLARE_MODULE(TableofcontentsModule);

class TableofcontentsMenuItem;


class TableofcontentsNoteAddin : public gnote::NoteAddin
{
public:
  static TableofcontentsNoteAddin *create()
    {
      return new TableofcontentsNoteAddin;
    }
  TableofcontentsNoteAddin();

  virtual void initialize() override;
  virtual void shutdown() override;
  virtual void on_note_opened() override;
  virtual std::vector<gnote::PopoverWidget> get_actions_popover_widgets() const override;

private:
  Glib::RefPtr<Gio::Menu> get_toc_menu() const;
  void on_level_1_activated ();
  void on_level_2_activated ();
  bool on_toc_popup_activated(Gtk::Widget&, const Glib::VariantBase&);
  void on_toc_help_activated ();
  void on_level_1_action(const Glib::VariantBase&);
  void on_level_2_action(const Glib::VariantBase&);
  void on_toc_help_action(const Glib::VariantBase&);
  void on_foregrounded();
  void on_goto_heading(const Glib::VariantBase&);
  void on_note_changed();


  bool has_tag_over_range (Glib::RefPtr<Gtk::TextTag> tag, Gtk::TextIter start, Gtk::TextIter end) const;
  Heading::Type get_heading_level_for_range (Gtk::TextIter start, Gtk::TextIter end) const;

  struct TocItem
  {
    Glib::ustring  heading;
    Heading::Type  heading_level;
    int            heading_position;
  };
  void get_toc_items(std::vector<TocItem> & items) const;
  std::vector<TableofcontentsMenuItem*> get_tableofcontents_menu_items();
  void get_toc_popover_items(std::vector<Glib::RefPtr<Gio::MenuItem>> & items) const;

  void headification_switch (Heading::Type heading_request);

  Glib::RefPtr<Gtk::TextTag> m_tag_bold; // the tags used to mark headings
  Glib::RefPtr<Gtk::TextTag> m_tag_large;
  Glib::RefPtr<Gtk::TextTag> m_tag_huge;
};


}

#endif
