/*
 * gnote
 *
 * Copyright (C) 2011,2013,2017 Aurimas Cernius
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


#include <glibmm/i18n.h>

#include "debug.hpp"
#include "remotecontrol-client-glue.hpp"

using namespace org::gnome::Gnote;

RemoteControl_proxy::RemoteControl_proxy(const Glib::RefPtr<Gio::DBus::Connection> & connection,
                                         const char * name, const char * p, const char * interface_name,
                                         const Glib::RefPtr<Gio::DBus::InterfaceInfo> & gnote_interface)
  : Gio::DBus::Proxy(connection, name, p, interface_name, gnote_interface)
{
}

Glib::ustring RemoteControl_proxy::CreateNamedNote(const Glib::ustring & linked_title)
{
  Glib::VariantContainerBase result = call_remote("CreateNamedNote",
    Glib::VariantContainerBase::create_tuple(Glib::Variant<Glib::ustring>::create(linked_title)));
  if(result.get_n_children() == 0) {
    return "";
  }
  Glib::Variant<Glib::ustring> res;
  result.get_child(res);
  return res.get();
}

Glib::ustring RemoteControl_proxy::CreateNote()
{
  Glib::VariantContainerBase result = call_remote("CreateNote", Glib::VariantContainerBase());
  if(result.get_n_children() == 0) {
    return "";
  }
  Glib::Variant<Glib::ustring> res;
  result.get_child(res);
  return res.get();
}

bool RemoteControl_proxy::DisplayNote(const Glib::ustring & uri)
{
  Glib::VariantContainerBase result = call_remote("DisplayNote",
    Glib::VariantContainerBase::create_tuple(Glib::Variant<Glib::ustring>::create(uri)));
  if(result.get_n_children() == 0) {
    return false;
  }
  Glib::Variant<bool> res;
  result.get_child(res);
  return res.get();
}

bool RemoteControl_proxy::DisplayNoteWithSearch(const Glib::ustring & uri, const Glib::ustring & search)
{
  std::vector<Glib::VariantBase> parameters;
  parameters.push_back(Glib::Variant<Glib::ustring>::create(uri));
  parameters.push_back(Glib::Variant<Glib::ustring>::create(search));
  Glib::VariantContainerBase result = call_remote("DisplayNoteWithSearch",
    Glib::VariantContainerBase::create_tuple(parameters));
  if(result.get_n_children() == 0) {
    return false;
  }
  Glib::Variant<bool> res;
  result.get_child(res);
  return res.get();
}

void RemoteControl_proxy::DisplaySearch()
{
  call_remote("DisplaySearch", Glib::VariantContainerBase());
}

Glib::ustring RemoteControl_proxy::FindNote(const Glib::ustring & linked_title)
{
  Glib::VariantContainerBase result = call_remote("FindNote",
    Glib::VariantContainerBase::create_tuple(Glib::Variant<Glib::ustring>::create(linked_title)));
  if(result.get_n_children() == 0) {
    return "";
  }
  Glib::Variant<Glib::ustring> res;
  result.get_child(res);
  return res.get();
}

Glib::ustring RemoteControl_proxy::FindStartHereNote()
{
  Glib::VariantContainerBase result = call_remote("FindStartHereNote", Glib::VariantContainerBase());
  if(result.get_n_children() == 0) {
    return "";
  }
  Glib::Variant<Glib::ustring> res;
  result.get_child(res);
  return res.get();
}

void RemoteControl_proxy::DisplaySearchWithText(const Glib::ustring & search_text)
{
  call_remote("DisplaySearchWithText", Glib::VariantContainerBase::create_tuple(Glib::Variant<Glib::ustring>::create(search_text)));
}

bool RemoteControl_proxy::SetNoteCompleteXml(const Glib::ustring & uri, const Glib::ustring & xml_contents)
{
  std::vector<Glib::VariantBase> parameters;
  parameters.push_back(Glib::Variant<Glib::ustring>::create(uri));
  parameters.push_back(Glib::Variant<Glib::ustring>::create(xml_contents));
  Glib::VariantContainerBase result = call_remote("SetNoteCompleteXml", Glib::VariantContainerBase::create_tuple(parameters));
  if(result.get_n_children() == 0) {
    return false;
  }
  Glib::Variant<bool> res;
  result.get_child(res);
  return res.get();
}

Glib::VariantContainerBase RemoteControl_proxy::call_remote(const Glib::ustring & method_name, const Glib::VariantContainerBase & parameters)
{
  try {
    return call_sync(method_name, parameters);
  }
  catch(...) {
    ERR_OUT(_("Remote call to %s failed"), method_name.c_str());
    return Glib::VariantContainerBase();
  }
}
