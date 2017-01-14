/*
 * gnote
 *
 * Copyright (C) 2011-2017 Aurimas Cernius
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



/* ported from */
/***************************************************************************
 *  ActionManager.cs
 *
 *  Copyright (C) 2005-2006 Novell, Inc.
 *  Written by Aaron Bockover <aaron@abock.org>
 ****************************************************************************/

/*  THIS FILE IS LICENSED UNDER THE MIT LICENSE AS OUTLINED IMMEDIATELY BELOW:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/i18n.h>
#include <gtkmm/window.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>

#include "sharp/string.hpp"
#include "debug.hpp"
#include "actionmanager.hpp"
#include "iconmanager.hpp"

namespace gnote {

  ActionManager::ActionManager()
  {
    make_app_actions();
    make_app_menu_items();

    register_main_window_action("close-window", NULL, false);
    register_main_window_action("delete-note", NULL, false);
    register_main_window_action("important-note", &Glib::Variant<bool>::variant_type(), false);
    register_main_window_action("enable-spell-check", &Glib::Variant<bool>::variant_type());
    register_main_window_action("new-notebook", NULL, false);
    register_main_window_action("move-to-notebook", &Glib::Variant<Glib::ustring>::variant_type(), false);
    register_main_window_action("undo", NULL, true);
    register_main_window_action("redo", NULL, true);
    register_main_window_action("link", NULL, true);
    register_main_window_action("change-font-bold", &Glib::Variant<bool>::variant_type(), true);
    register_main_window_action("change-font-italic", &Glib::Variant<bool>::variant_type(), true);
    register_main_window_action("change-font-strikeout", &Glib::Variant<bool>::variant_type(), true);
    register_main_window_action("change-font-highlight", &Glib::Variant<bool>::variant_type(), true);
    register_main_window_action("change-font-size", &Glib::Variant<Glib::ustring>::variant_type(), true);
    register_main_window_action("enable-bullets", &Glib::Variant<bool>::variant_type(), true);
    register_main_window_action("increase-indent", NULL, true);
    register_main_window_action("decrease-indent", NULL, true);
  }


  void ActionManager::make_app_actions()
  {
    add_app_action("new-note");
    add_app_action("new-window");
    add_app_action("show-preferences");
    add_app_action("about");
    add_app_action("help-contents");
    add_app_action("help-shortcuts");
    add_app_action("quit");
  }

  Glib::RefPtr<Gio::SimpleAction> ActionManager::get_app_action(const Glib::ustring & name) const
  {
    for(std::vector<Glib::RefPtr<Gio::SimpleAction> >::const_iterator iter = m_app_actions.begin();
        iter != m_app_actions.end(); ++iter) {
      if((*iter)->get_name() == name) {
        return *iter;
      }
    }
    
    return Glib::RefPtr<Gio::SimpleAction>();
  }

  Glib::RefPtr<Gio::SimpleAction> ActionManager::add_app_action(const Glib::ustring & name)
  {
    Glib::RefPtr<Gio::SimpleAction> action = Gio::SimpleAction::create(name);
    m_app_actions.push_back(action);
    return action;
  }

  void ActionManager::add_app_menu_item(int section, int order, const Glib::ustring & label,
                                        const Glib::ustring & action_def)
  {
    m_app_menu_items.insert(std::make_pair(section, AppMenuItem(order, label, action_def)));
  }

  void ActionManager::make_app_menu_items()
  {
    add_app_menu_item(APP_ACTION_NEW, 100, _("_New Note"), "app.new-note");
    add_app_menu_item(APP_ACTION_NEW, 200, _("New _Window"), "app.new-window");
    add_app_menu_item(APP_ACTION_MANAGE, 100, _("_Preferences"), "app.show-preferences");
    add_app_menu_item(APP_ACTION_LAST, 100, _("_Shortcuts"), "app.help-shortcuts");
    add_app_menu_item(APP_ACTION_LAST, 150, _("_Help"), "app.help-contents");
    add_app_menu_item(APP_ACTION_LAST, 200, _("_About"), "app.about");
    add_app_menu_item(APP_ACTION_LAST, 300, _("_Quit"), "app.quit");
  }

  Glib::RefPtr<Gio::Menu> ActionManager::get_app_menu() const
  {
    Glib::RefPtr<Gio::Menu> menu = Gio::Menu::create();

    int pos = 0;
    Glib::RefPtr<Gio::Menu> section = make_app_menu_section(APP_ACTION_NEW);
    if(section) {
      menu->insert_section(pos++, section);
    }

    section = make_app_menu_section(APP_ACTION_MANAGE);
    if(section) {
      menu->insert_section(pos++, section);
    }

    section = make_app_menu_section(APP_ACTION_LAST);
    if(section) {
      menu->insert_section(pos++, section);
    }

    return menu;
  }

  Glib::RefPtr<Gio::Menu> ActionManager::make_app_menu_section(int sec) const
  {
    std::pair<AppMenuItemMultiMap::const_iterator, AppMenuItemMultiMap::const_iterator>
    range = m_app_menu_items.equal_range(sec);

    Glib::RefPtr<Gio::Menu> section;
    if(range.first != m_app_menu_items.end()) {
      std::vector<const AppMenuItem*> menu_items;
      for(AppMenuItemMultiMap::const_iterator iter = range.first; iter != range.second; ++iter) {
        menu_items.push_back(&iter->second);
      }
      std::sort(menu_items.begin(), menu_items.end(), AppMenuItem::ptr_comparator());

      section = Gio::Menu::create();
      for(std::vector<const AppMenuItem*>::iterator iter = menu_items.begin(); iter != menu_items.end(); ++iter) {
        section->append((*iter)->label, (*iter)->action_def);
      }
    }

    return section;
  }

  void ActionManager::register_main_window_action(const Glib::ustring & action, const Glib::VariantType *state_type, bool modifying)
  {
    if(m_main_window_actions.find(action) == m_main_window_actions.end()) {
      m_main_window_actions[action] = state_type;
      if(!modifying) {
        m_non_modifying_actions.push_back(action);
      }
    }
    else {
      if(m_main_window_actions[action] != state_type) {
        ERR_OUT("Action %s already registerred with different state type", action.c_str());
      }
    }
  }

  std::map<Glib::ustring, const Glib::VariantType*> ActionManager::get_main_window_actions() const
  {
    return m_main_window_actions;
  }

  bool ActionManager::is_modifying_main_window_action(const Glib::ustring & action) const
  {
    return std::find(m_non_modifying_actions.begin(), m_non_modifying_actions.end(), action) == m_non_modifying_actions.end();
  }

  void ActionManager::register_main_window_search_callback(const Glib::ustring & id, const Glib::ustring & action,
                                                    sigc::slot<void, const Glib::VariantBase&> callback)
  {
    DBG_ASSERT(m_main_window_search_actions.find(id) == m_main_window_search_actions.end(), "Duplicate callback for main window search");

    m_main_window_search_actions[id] = std::make_pair(action, callback);
    signal_main_window_search_actions_changed();
  }

  void ActionManager::unregister_main_window_search_callback(const Glib::ustring & id)
  {
    m_main_window_search_actions.erase(id);
    signal_main_window_search_actions_changed();
  }

  std::map<Glib::ustring, sigc::slot<void, const Glib::VariantBase&>> ActionManager::get_main_window_search_callbacks()
  {
    std::map<Glib::ustring, sigc::slot<void, const Glib::VariantBase&>> cbacks;
    for(auto & iter : m_main_window_search_actions) {
      cbacks.insert(iter.second);
    }
    return cbacks;
  }

}
