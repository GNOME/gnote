/*
 * gnote
 *
 * Copyright (C) 2012-2013,2015-2016 Aurimas Cernius
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

#ifndef __ACTIONMANAGER_HPP_
#define __ACTIONMANAGER_HPP_

#include <map>
#include <string>
#include <list>

#include <glibmm/variant.h>
#include <giomm/menu.h>
#include <giomm/simpleaction.h>
#include <gtkmm/action.h>

#include "base/macros.hpp"
#include "iactionmanager.hpp"

namespace gnote {

class ActionManager
  : public IActionManager
{
public:
  ActionManager();

  virtual Glib::RefPtr<Gio::SimpleAction> get_app_action(const std::string & name) const override;
  const std::vector<Glib::RefPtr<Gio::SimpleAction> > & get_app_actions() const
    {
      return m_app_actions;
    }
  virtual Glib::RefPtr<Gio::SimpleAction> add_app_action(const std::string & name) override;
  virtual void add_app_menu_item(int section, int order, const std::string & label,
                                 const std::string & action_def) override;
  Glib::RefPtr<Gio::Menu> get_app_menu() const;
  virtual void register_main_window_action(const Glib::ustring & action, const Glib::VariantType *state_type,
    bool modifying = true) override;
  virtual std::map<Glib::ustring, const Glib::VariantType*> get_main_window_actions() const override;
  virtual bool is_modifying_main_window_action(const Glib::ustring & action) const override;

  virtual void register_main_window_search_callback(const std::string & id, const Glib::ustring & action,
                                                    sigc::slot<void, const Glib::VariantBase&> callback) override;
  virtual void unregister_main_window_search_callback(const std::string & id) override;
  virtual std::map<Glib::ustring, sigc::slot<void, const Glib::VariantBase&>> get_main_window_search_callbacks() override;
private:
  void make_app_actions();
  void make_app_menu_items();
  Glib::RefPtr<Gio::Menu> make_app_menu_section(int section) const;

  std::vector<Glib::RefPtr<Gio::SimpleAction> > m_app_actions;

  struct AppMenuItem
  {
    int order;
    std::string label;
    std::string action_def;

    AppMenuItem(int ord, const std::string & lbl, const std::string & act_def)
      : order(ord)
      , label(lbl)
      , action_def(act_def)
      {}

    struct ptr_comparator
    {
      bool operator() (const AppMenuItem *x, const AppMenuItem *y)
        {
          return x->order < y->order;
        }
    };
  };
  typedef std::multimap<int, AppMenuItem> AppMenuItemMultiMap;
  AppMenuItemMultiMap m_app_menu_items;
  std::map<Glib::ustring, const Glib::VariantType*> m_main_window_actions;
  std::vector<Glib::ustring> m_non_modifying_actions;
  std::map<std::string, std::pair<Glib::ustring, sigc::slot<void, const Glib::VariantBase&>>> m_main_window_search_actions;
};


}

#endif

