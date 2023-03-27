/*
 * gnote
 *
 * Copyright (C) 2019,2022 Aurimas Cernius
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

#ifndef _POPOVERWIDGETS_HPP_
#define _POPOVERWIDGETS_HPP_

#include <giomm/menuitem.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/popover.h>

namespace gnote {

extern const int APP_SECTION_NEW;
extern const int APP_SECTION_MANAGE;
extern const int APP_SECTION_LAST;
extern const int APP_CUSTOM_SECTION;

extern const int NOTE_SECTION_NEW;
extern const int NOTE_SECTION_UNDO;
extern const int NOTE_SECTION_CUSTOM_SECTIONS;
extern const int NOTE_SECTION_FLAGS;
extern const int NOTE_SECTION_ACTIONS;

enum NoteActionOrder {
  // custom sections
  NOTEBOOK_ORDER = 100,
  BACKLINKS_ORDER = 200,
  TABLE_OF_CONTENTS_ORDER = 100,
  // flags
  READ_ONLY_ORDER = 100,
  SPELL_CHECK_ORDER = 200,
  IMPORTANT_ORDER = 300,
  // actions
  LINK_ORDER = 50,
  EXPORT_TO_HTML_ORDER = 100,
  EXPORT_TO_GTG_ORDER = 200,
  INSERT_TIMESTAMP_ORDER = 300,
  PRINT_ORDER = 400,
  REPLACE_TITLE_ORDER = 500,
};

struct PopoverWidget
{
  Glib::RefPtr<Gio::MenuItem> widget;
  int section;
  int order;
  int secondary_order;

  static PopoverWidget create_for_app(int ord, const Glib::RefPtr<Gio::MenuItem> w);
  static PopoverWidget create_for_note(int ord, const Glib::RefPtr<Gio::MenuItem> w);
  static PopoverWidget create_custom_section(const Glib::RefPtr<Gio::MenuItem> w);

  PopoverWidget(int sec, int ord, const Glib::RefPtr<Gio::MenuItem> w)
    : widget(w)
    , section(sec)
    , order(ord)
    {}

  bool operator< (const PopoverWidget & other)
    {
      if(section != other.section)
        return section < other.section;
      if(order != other.order)
        return order < other.order;
      return secondary_order < other.secondary_order;
    }
};


namespace utils {
  void unparent_popover_on_close(Gtk::Popover *popover);

  template <typename T, typename... Args>
  T *make_popover(Gtk::Widget & parent, Args... args)
  {
    auto popover = Gtk::make_managed<T>(args...);
    popover->set_parent(parent);
    unparent_popover_on_close(popover);
    return popover;
  }

  template <typename T, typename... Args>
  std::shared_ptr<T> make_owned_popover(Gtk::Widget & parent, Args... args)
  {
    auto popover = std::make_shared<T>(args...);
    popover->set_parent(parent);
    unparent_popover_on_close(popover.get());
    return popover;
  }
}

}

#endif

