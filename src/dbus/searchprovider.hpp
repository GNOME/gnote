/*
 * gnote
 *
 * Copyright (C) 2013,2019,2022 Aurimas Cernius
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


#ifndef _DBUS_SEARCHPROVIDER_HPP_
#define _DBUS_SEARCHPROVIDER_HPP_


#include <giomm/dbusinterfacevtable.h>

#include "notemanagerbase.hpp"


namespace org {
namespace gnome {
namespace Gnote {

class SearchProvider
  : Gio::DBus::InterfaceVTable
{
public:
  SearchProvider(const Glib::RefPtr<Gio::DBus::Connection> & conn, const char *object_path,
                 const Glib::RefPtr<Gio::DBus::InterfaceInfo> & search_interface,
                 gnote::IGnote & g, gnote::NoteManagerBase & manager);

  std::vector<Glib::ustring> GetInitialResultSet(const std::vector<Glib::ustring> & terms);
  std::vector<Glib::ustring> GetSubsearchResultSet(const std::vector<Glib::ustring> & previous_results,
                                                   const std::vector<Glib::ustring> & terms);
  std::vector<std::map<Glib::ustring, Glib::ustring> > GetResultMetas(
        const std::vector<Glib::ustring> & identifiers);
  void ActivateResult(const Glib::ustring & identifier, const std::vector<Glib::ustring> & terms, guint32 timestamp);
private:
  void on_method_call(const Glib::RefPtr<Gio::DBus::Connection> & connection,
                      const Glib::ustring & sender,
                      const Glib::ustring & object_path,
                      const Glib::ustring & interface_name,
                      const Glib::ustring & method_name,
                      const Glib::VariantContainerBase & parameters,
                      const Glib::RefPtr<Gio::DBus::MethodInvocation> & invocation);

  Glib::VariantContainerBase GetInitialResultSet_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase GetSubsearchResultSet_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase GetResultMetas_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase ActivateResult_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase LaunchSearch_stub(const Glib::VariantContainerBase &);
  const gchar *get_icon() const;

  typedef Glib::VariantContainerBase (SearchProvider::*stub_func)(const Glib::VariantContainerBase &);
  std::map<Glib::ustring, stub_func> m_stubs;

  gnote::IGnote & m_gnote;
  gnote::NoteManagerBase & m_manager;
};

}
}
}

#endif
