/*
 * gnote
 *
 * Copyright (C) 2015-2016 Aurimas Cernius
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

#include "mainwindowaction.hpp"

namespace gnote {

MainWindowAction::Ptr MainWindowAction::create(const Glib::ustring & name)
{
  return Ptr(new MainWindowAction(name));
}

MainWindowAction::Ptr MainWindowAction::create(const Glib::ustring & name, bool state)
{
  return Ptr(new MainWindowAction(name, state));
}

MainWindowAction::Ptr MainWindowAction::create(const Glib::ustring & name, int state)
{
  return Ptr(new MainWindowAction(name, state));
}

MainWindowAction::Ptr MainWindowAction::create(const Glib::ustring & name, const Glib::ustring & state)
{
  return Ptr(new MainWindowAction(name, state));
}

MainWindowAction::MainWindowAction(const Glib::ustring & name)
  : Gio::SimpleAction(name)
  , m_modifying(true)
{
}

MainWindowAction::MainWindowAction(const Glib::ustring & name, bool state)
  : Gio::SimpleAction(name, Glib::Variant<bool>::create(state))
  , m_modifying(true)
{
}

MainWindowAction::MainWindowAction(const Glib::ustring & name, int state)
  : Gio::SimpleAction(name, Glib::VARIANT_TYPE_INT32, Glib::Variant<gint32>::create(state))
  , m_modifying(true)
{
}

MainWindowAction::MainWindowAction(const Glib::ustring & name, const Glib::ustring & state)
  : Gio::SimpleAction(name, Glib::VARIANT_TYPE_STRING, Glib::Variant<Glib::ustring>::create(state))
  , m_modifying(true)
{
}

}
