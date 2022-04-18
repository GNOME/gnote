/*
 * gnote
 *
 * Copyright (C) 2015-2016,2019,2022 Aurimas Cernius
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

#ifndef _MAIN_WINDOW_ACTION_HPP_
#define _MAIN_WINDOW_ACTION_HPP_

#include <giomm/simpleaction.h>


namespace gnote {

class MainWindowAction
  : public Gio::SimpleAction
{
public:
  typedef Glib::RefPtr<MainWindowAction> Ptr;

  static Ptr create(Glib::ustring && name);
  static Ptr create(Glib::ustring && name, bool state);
  static Ptr create(Glib::ustring && name, int state);
  static Ptr create(Glib::ustring && name, Glib::ustring && state);

  void set_state(const Glib::VariantBase & value)
    {
      Gio::SimpleAction::set_state(value);
    }
  void is_modifying(bool modifying)
    {
      m_modifying = modifying;
    }
  bool is_modifying() const
    {
      return m_modifying;
    }
protected:
  MainWindowAction(Glib::ustring && name);
  MainWindowAction(Glib::ustring && name, bool state);
  MainWindowAction(Glib::ustring && name, int state);
  MainWindowAction(Glib::ustring && name, Glib::ustring && state);
private:
  bool m_modifying;
};

}

#endif

