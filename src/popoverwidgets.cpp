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
      PopoverSubmenuBox(Glib::ustring && submenu)
        : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
        , PopoverSubmenu(std::move(submenu))
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


PopoverButton::PopoverButton(Glib::ustring && label, bool mnemonic)
  : Gtk::Button(std::move(label), mnemonic)
  , m_parent_po(nullptr)
{
}


PopoverSubmenuButton::PopoverSubmenuButton(Glib::ustring && label, bool mnemonic, sigc::slot<Gtk::Widget*()> && submenu_builder)
  : PopoverButton(std::move(label), mnemonic)
  , m_builder(std::move(submenu_builder))
{
  set_common_popover_button_props(*this);
  signal_clicked().connect(sigc::mem_fun(*this, &PopoverSubmenuButton::on_clicked));
}

void PopoverSubmenuButton::on_clicked()
{
  parent_popover()->set_child(*manage(m_builder()));
}


namespace utils {

  Gtk::Button *create_popover_button(const Glib::ustring & action, Glib::ustring && label)
  {
    Gtk::Button *item = new Gtk::Button(std::move(label), true);
    item->set_action_name(action);
    set_common_popover_button_props(*item);
    return item;
  }


  Gtk::Widget *create_popover_submenu_button(const Glib::ustring & submenu, Glib::ustring && label)
  {
    Gtk::ModelButton *button = new Gtk::ModelButton;
    button->property_menu_name() = submenu;
    button->set_label(std::move(label));
    set_common_popover_button_props(*button);
    return button;
  }


  Gtk::Box *create_popover_submenu(Glib::ustring && name)
  {
    return new PopoverSubmenuBox(std::move(name));
  }


  void set_common_popover_widget_props(Gtk::Widget & widget)
  {
    widget.property_hexpand() = true;
  }

  void set_common_popover_widget_props(Gtk::Box & widget)
  {
    widget.property_margin_top() = 9;
    widget.property_margin_bottom() = 9;
    widget.property_margin_start() = 12;
    widget.property_margin_end() = 12;
    set_common_popover_widget_props(static_cast<Gtk::Widget&>(widget));
  }

}

}

