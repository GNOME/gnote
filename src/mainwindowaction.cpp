/*
 * gnote
 *
 * Copyright (C) 2015-2016,2022 Aurimas Cernius
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

MainWindowAction::Ptr MainWindowAction::create(Glib::ustring && name)
{
  return Ptr(new MainWindowAction(std::move(name)));
}

MainWindowAction::Ptr MainWindowAction::create(Glib::ustring && name, bool state)
{
  return Ptr(new MainWindowAction(std::move(name), state));
}

MainWindowAction::Ptr MainWindowAction::create(Glib::ustring && name, int state)
{
  return Ptr(new MainWindowAction(std::move(name), state));
}

MainWindowAction::Ptr MainWindowAction::create(Glib::ustring && name, Glib::ustring && state)
{
  return Ptr(new MainWindowAction(std::move(name), std::move(state)));
}

MainWindowAction::MainWindowAction(Glib::ustring && name)
  : Gio::SimpleAction(std::move(name))
  , m_modifying(true)
{
}

MainWindowAction::MainWindowAction(Glib::ustring && name, bool state)
  : Gio::SimpleAction(std::move(name), Glib::Variant<bool>::create(state))
  , m_modifying(true)
{
}

MainWindowAction::MainWindowAction(Glib::ustring && name, int state)
  : Gio::SimpleAction(std::move(name), Glib::VARIANT_TYPE_INT32, Glib::Variant<gint32>::create(state))
  , m_modifying(true)
{
}

MainWindowAction::MainWindowAction(Glib::ustring && name, Glib::ustring && state)
  : Gio::SimpleAction(std::move(name), Glib::VARIANT_TYPE_STRING, Glib::Variant<Glib::ustring>::create(std::move(state)))
  , m_modifying(true)
{
}

}
