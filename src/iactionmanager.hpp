/*
 * gnote
 *
 * Copyright (C) 2013,2015-2016 Aurimas Cernius
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


#ifndef _IACTIONMANAGER_HPP_
#define _IACTIONMANAGER_HPP_

#include <giomm/simpleaction.h>
#include <gtkmm/action.h>
#include <gtkmm/uimanager.h>

#include "base/singleton.hpp"

namespace gnote {

enum NoteActionOrder {
  NOTEBOOK_ORDER = 50,
  BACKLINKS_ORDER = 100,
  EXPORT_TO_HTML_ORDER = 200,
  EXPORT_TO_GTG_ORDER = 250,
  INSERT_TIMESTAMP_ORDER = 300,
  PRINT_ORDER = 400,
  REPLACE_TITLE_ORDER = 500,
  TABLE_OF_CONTENTS_ORDER = 600,
  READ_ONLY_ORDER = 700,
  SPELL_CHECK_ORDER = 800,
};

class IActionManager
  : public base::Singleton<IActionManager>
{
public:
  static const int APP_ACTION_NEW;
  static const int APP_ACTION_MANAGE;
  static const int APP_ACTION_LAST;

  virtual ~IActionManager();

  virtual Glib::RefPtr<Gio::SimpleAction> get_app_action(const std::string & name) const = 0;
  virtual Glib::RefPtr<Gio::SimpleAction> add_app_action(const std::string & name) = 0;
  virtual void add_app_menu_item(int section, int order, const std::string & label,
                                 const std::string & action_def) = 0;
  virtual void register_main_window_action(const Glib::ustring & action, const Glib::VariantType *state_type,
    bool modifying = true) = 0;
  virtual std::map<Glib::ustring, const Glib::VariantType*> get_main_window_actions() const = 0;
  virtual bool is_modifying_main_window_action(const Glib::ustring & action) const = 0;

  virtual void register_main_window_search_callback(const std::string & id, const Glib::ustring & action,
                                                    sigc::slot<void, const Glib::VariantBase&> callback) = 0;
  virtual void unregister_main_window_search_callback(const std::string & id) = 0;
  virtual std::map<Glib::ustring, sigc::slot<void, const Glib::VariantBase&>> get_main_window_search_callbacks() = 0;
  sigc::signal<void> signal_main_window_search_actions_changed;
  sigc::signal<void, std::map<int, Gtk::Widget*>&> signal_build_main_window_search_popover;
};

}

#endif

