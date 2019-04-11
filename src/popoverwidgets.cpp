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

#include <gtkmm/label.h>
#include <gtkmm/modelbutton.h>

#include "iactionmanager.hpp"
#include "popoverwidgets.hpp"

namespace gnote {
  namespace {
    class PopoverSubmenuBox
      : public Gtk::Box
      , public PopoverSubmenu
    {
    public:
      PopoverSubmenuBox(const Glib::ustring & submenu)
        : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
        , PopoverSubmenu(submenu)
      {
        utils::set_common_popover_widget_props(*this);
      }
    };


    void set_common_popover_button_props(Gtk::ModelButton & button)
    {
      button.set_use_underline(true);
      button.property_margin_top() = 3;
      button.property_margin_bottom() = 3;
      auto lbl = dynamic_cast<Gtk::Label*>(button.get_child());
      if(lbl) {
        lbl->set_xalign(0.0f);
      }
      utils::set_common_popover_widget_props(button);
    }
  }


const int APP_SECTION_NEW = 1;
const int APP_SECTION_MANAGE = 2;
const int APP_SECTION_LAST = 3;
const int APP_CUSTOM_SECTION = 1000;

const int NOTE_SECTION_NEW = 1;
const int NOTE_SECTION_UNDO = 2;
const int NOTE_SECTION_CUSTOM_SECTIONS = 10;
const int NOTE_SECTION_FLAGS = 20;
const int NOTE_SECTION_ACTIONS = 30;

PopoverWidget PopoverWidget::create_for_app(int ord, Gtk::Widget *w)
{
  return PopoverWidget(APP_SECTION_MANAGE, ord, w);
}

PopoverWidget PopoverWidget::create_for_note(int ord, Gtk::Widget *w)
{
  return PopoverWidget(NOTE_SECTION_ACTIONS, ord, w);
}

PopoverWidget PopoverWidget::create_custom_section(Gtk::Widget *w)
{
  return PopoverWidget(APP_CUSTOM_SECTION, 0, w);
}

namespace utils {

  Gtk::Widget * create_popover_button(const Glib::ustring & action, const Glib::ustring & label)
  {
    Gtk::ModelButton *item = new Gtk::ModelButton;
    gtk_actionable_set_action_name(GTK_ACTIONABLE(item->gobj()), action.c_str());
    item->set_label(label);
    set_common_popover_button_props(*item);
    return item;
  }


  Gtk::Widget * create_popover_submenu_button(const Glib::ustring & submenu, const Glib::ustring & label)
  {
    Gtk::ModelButton *button = new Gtk::ModelButton;
    button->property_menu_name() = submenu;
    button->set_label(label);
    set_common_popover_button_props(*button);
    return button;
  }


  Gtk::Box * create_popover_submenu(const Glib::ustring & name)
  {
    return new PopoverSubmenuBox(name);
  }


  void set_common_popover_widget_props(Gtk::Widget & widget)
  {
    widget.property_hexpand() = true;
  }

  void set_common_popover_widget_props(Gtk::Box & widget)
  {
    widget.property_margin_top() = 9;
    widget.property_margin_bottom() = 9;
    widget.property_margin_left() = 12;
    widget.property_margin_right() = 12;
    set_common_popover_widget_props(static_cast<Gtk::Widget&>(widget));
  }

}

}

