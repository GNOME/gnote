/*
 * gnote
 *
 * Copyright (C) 2011-2019,2022,2024 Aurimas Cernius
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

#include "sharp/string.hpp"
#include "debug.hpp"
#include "actionmanager.hpp"

namespace gnote {

  void ActionManager::init()
  {
    make_app_actions();
    make_app_menu_items();

    register_main_window_action("close-tab", NULL, false);
    register_main_window_action("close-window", NULL, false);
    register_main_window_action("open-note", NULL, false);
    register_main_window_action("open-note-new-window", NULL, false);
    register_main_window_action("delete-note", NULL, false);
    register_main_window_action("delete-selected-notes", NULL, false);
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
    register_main_window_action("increase-indent", NULL, true);
    register_main_window_action("decrease-indent", NULL, true);

    signal_build_main_window_search_popover.connect(sigc::mem_fun(*this, &ActionManager::add_app_menu_items));
  }


  void ActionManager::make_app_actions()
  {
    add_app_action("new-note");
    add_app_action("new-window");
    add_app_action("show-preferences");
    add_app_action("about");
    add_app_action("help-contents");
    add_app_action("help-shortcuts");
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

  Glib::RefPtr<Gio::SimpleAction> ActionManager::add_app_action(Glib::ustring && name)
  {
    auto action = Gio::SimpleAction::create(std::move(name));
    m_app_actions.push_back(action);
    return action;
  }

  void ActionManager::add_app_menu_item(int section, int order, Glib::ustring && label, Glib::ustring && action_def)
  {
    m_app_menu_items.insert(std::make_pair(section, AppMenuItem(order, std::move(label), std::move(action_def))));
  }

  void ActionManager::make_app_menu_items()
  {
    add_app_menu_item(APP_SECTION_NEW, 100, _("_New Note"), "app.new-note");
    add_app_menu_item(APP_SECTION_NEW, 200, _("New _Window"), "app.new-window");
    add_app_menu_item(APP_SECTION_LAST, 50, _("_Preferences"), "app.show-preferences");
    add_app_menu_item(APP_SECTION_LAST, 100, _("_Shortcuts"), "app.help-shortcuts");
    add_app_menu_item(APP_SECTION_LAST, 150, _("_Help"), "app.help-contents");
    add_app_menu_item(APP_SECTION_LAST, 200, _("_About Gnote"), "app.about");
  }

  void ActionManager::register_main_window_action(Glib::ustring && action, const Glib::VariantType *state_type, bool modifying)
  {
    if(m_main_window_actions.find(action) == m_main_window_actions.end()) {
      if(!modifying) {
        m_non_modifying_actions.push_back(action);
      }
      m_main_window_actions.insert(std::make_pair(std::move(action), state_type));
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

  void ActionManager::register_main_window_search_callback(Glib::ustring && id, Glib::ustring && action, const IActionManager::ActionCallback & callback)
  {
    DBG_ASSERT(m_main_window_search_actions.find(id) == m_main_window_search_actions.end(), "Duplicate callback for main window search");

    m_main_window_search_actions.insert(std::make_pair(std::move(id), std::make_pair(std::move(action), callback)));
    signal_main_window_search_actions_changed();
  }

  void ActionManager::unregister_main_window_search_callback(const Glib::ustring & id)
  {
    m_main_window_search_actions.erase(id);
    signal_main_window_search_actions_changed();
  }

  std::map<Glib::ustring, IActionManager::ActionCallback> ActionManager::get_main_window_search_callbacks()
  {
    std::map<Glib::ustring, ActionCallback> cbacks;
    for(auto & iter : m_main_window_search_actions) {
      cbacks.insert(iter.second);
    }
    return cbacks;
  }

  void ActionManager::add_app_menu_items(std::vector<PopoverWidget> & widgets)
  {
    for(auto& iter : m_app_menu_items) {
      auto item = Gio::MenuItem::create(iter.second.label, iter.second.action_def);
      widgets.push_back(PopoverWidget(iter.first, iter.second.order, item));
    }
  }

}
