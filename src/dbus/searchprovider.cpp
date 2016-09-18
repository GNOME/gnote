/*
 * gnote
 *
 * Copyright (C) 2013-2014 Aurimas Cernius
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


#include <giomm/dbusconnection.h>
#include <giomm/dbuserror.h>

#include <set>

#include "debug.hpp"
#include "iconmanager.hpp"
#include "ignote.hpp"
#include "search.hpp"
#include "searchprovider.hpp"


namespace org {
namespace gnome {
namespace Gnote {


SearchProvider::SearchProvider(const Glib::RefPtr<Gio::DBus::Connection> & conn,
                               const char *object_path,
                               const Glib::RefPtr<Gio::DBus::InterfaceInfo> & search_interface,
                               gnote::NoteManager & manager)
  : Gio::DBus::InterfaceVTable(sigc::mem_fun(*this, &SearchProvider::on_method_call))
  , m_manager(manager)
{
  conn->register_object(object_path, search_interface, *this);

  m_stubs["GetInitialResultSet"] = &SearchProvider::GetInitialResultSet_stub;
  m_stubs["GetSubsearchResultSet"] = &SearchProvider::GetSubsearchResultSet_stub;
  m_stubs["GetResultMetas"] = &SearchProvider::GetResultMetas_stub;
  m_stubs["ActivateResult"] = &SearchProvider::ActivateResult_stub;
  m_stubs["LaunchSearch"] = &SearchProvider::LaunchSearch_stub;
}

void SearchProvider::on_method_call(const Glib::RefPtr<Gio::DBus::Connection> &,
                                    const Glib::ustring &,
                                    const Glib::ustring &,
                                    const Glib::ustring &,
                                    const Glib::ustring & method_name,
                                    const Glib::VariantContainerBase & parameters,
                                    const Glib::RefPtr<Gio::DBus::MethodInvocation> & invocation)
{
  DBG_OUT("Search method %s called", method_name.c_str());
  std::map<Glib::ustring, stub_func>::iterator iter = m_stubs.find(method_name);
  if(iter == m_stubs.end()) {
    invocation->return_error(Gio::DBus::Error(Gio::DBus::Error::UNKNOWN_METHOD,
                             "Unknown method: " + method_name));
  }
  else {
    try {
      stub_func func = iter->second;
      invocation->return_value((this->*func)(parameters));
    }
    catch(Glib::Exception & e) {
      invocation->return_error(Gio::DBus::Error(Gio::DBus::Error::UNKNOWN_METHOD,
                               "Exception in method " + method_name + ": " + e.what()));
    }
    catch(std::exception & e) {
      invocation->return_error(Gio::DBus::Error(Gio::DBus::Error::UNKNOWN_METHOD,
                               "Exception in method " + method_name + ": " + e.what()));
    }
    catch(...) {
      invocation->return_error(Gio::DBus::Error(Gio::DBus::Error::UNKNOWN_METHOD,
                               "Exception in method " + method_name));
    }
  }
}

std::vector<Glib::ustring> SearchProvider::GetInitialResultSet(const std::vector<Glib::ustring> & terms)
{
  std::set<gnote::Note::Ptr> final_result;
  gnote::Search search(m_manager);
  gnote::notebooks::Notebook::Ptr notebook;
  for(std::vector<Glib::ustring>::const_iterator query = terms.begin(); query != terms.end(); ++query) {
    gnote::Search::ResultsPtr results = search.search_notes(*query, false, notebook);
    for(gnote::Search::Results::iterator iter = results->begin(); iter != results->end(); ++iter) {
      final_result.insert(iter->second);
    }
  }

  std::vector<Glib::ustring> ret;
  for(std::set<gnote::Note::Ptr>::iterator iter = final_result.begin(); iter != final_result.end(); ++iter) {
    ret.push_back((*iter)->uri());
  }

  return ret;
}

Glib::VariantContainerBase SearchProvider::GetInitialResultSet_stub(const Glib::VariantContainerBase & params)
{
  if(params.get_n_children() != 1) {
    throw std::invalid_argument("One argument expected");
  }

  Glib::Variant<std::vector<Glib::ustring> > terms;
  params.get_child(terms, 0);
  return Glib::VariantContainerBase::create_tuple(
    Glib::Variant<std::vector<Glib::ustring> >::create(GetInitialResultSet(terms.get())));
}

std::vector<Glib::ustring> SearchProvider::GetSubsearchResultSet(
    const std::vector<Glib::ustring> & previous_results, const std::vector<Glib::ustring> & terms)
{
  std::set<Glib::ustring> previous(previous_results.begin(), previous_results.end());
  if(previous.size() == 0) {
    return std::vector<Glib::ustring>();
  }

  std::vector<Glib::ustring> final_result;
  std::vector<Glib::ustring> new_result = GetInitialResultSet(terms);
  for(std::vector<Glib::ustring>::iterator iter = new_result.begin(); iter != new_result.end(); ++iter) {
    if(previous.find(*iter) != previous.end()) {
      final_result.push_back(*iter);
    }
  }

  return final_result;
}

Glib::VariantContainerBase SearchProvider::GetSubsearchResultSet_stub(const Glib::VariantContainerBase & params)
{
  if(params.get_n_children() != 2) {
    throw std::invalid_argument("Two arguments expected");
  }

  Glib::Variant<std::vector<Glib::ustring> > previous_results, terms;
  params.get_child(previous_results, 0);
  params.get_child(terms, 1);
  return Glib::VariantContainerBase::create_tuple(
    Glib::Variant<std::vector<Glib::ustring> >::create(GetSubsearchResultSet(previous_results.get(), terms.get())));
}

std::vector<std::map<Glib::ustring, Glib::ustring> > SearchProvider::GetResultMetas(
    const std::vector<Glib::ustring> & identifiers)
{
  std::vector<std::map<Glib::ustring, Glib::ustring> > ret;
  for(std::vector<Glib::ustring>::const_iterator iter = identifiers.begin(); iter != identifiers.end(); ++iter) {
    gnote::NoteBase::Ptr note = m_manager.find_by_uri(*iter);
    if(note == 0) {
      continue;
    }

    std::map<Glib::ustring, Glib::ustring> meta;
    meta["id"] = note->uri();
    meta["name"] = note->get_title();
    ret.push_back(meta);
  }

  return ret;
}

Glib::VariantContainerBase SearchProvider::GetResultMetas_stub(const Glib::VariantContainerBase & params)
{
  if(params.get_n_children() != 1) {
    throw std::invalid_argument("One argument expected");
  }

  Glib::Variant<std::vector<Glib::ustring> > identifiers;
  params.get_child(identifiers, 0);
  std::vector<std::map<Glib::ustring, Glib::ustring> > metas = GetResultMetas(identifiers.get());

  GVariantBuilder result;
  g_variant_builder_init(&result, G_VARIANT_TYPE("aa{sv}"));

  for(std::vector<std::map<Glib::ustring, Glib::ustring> >::iterator iter = metas.begin();
      iter != metas.end(); ++iter) {
    g_variant_builder_open(&result, G_VARIANT_TYPE("a{sv}"));
    for(std::map<Glib::ustring, Glib::ustring>::iterator entry = iter->begin(); entry != iter->end(); ++entry) {
      g_variant_builder_add(&result, "{sv}", entry->first.c_str(), g_variant_new_string(entry->second.c_str()));
    }
    g_variant_builder_add(&result, "{sv}", "gicon", g_variant_new_string(get_icon()));
    g_variant_builder_close(&result);
  }

  return Glib::VariantContainerBase(g_variant_new("(aa{sv})", &result));
}

void SearchProvider::ActivateResult(const Glib::ustring & identifier,
                                    const std::vector<Glib::ustring> & /*terms*/,
                                    guint32 /*timestamp*/)
{
  gnote::NoteBase::Ptr note = m_manager.find_by_uri(identifier);
  if(note != 0) {
    gnote::IGnote::obj().open_note(static_pointer_cast<gnote::Note>(note));
  }
}

Glib::VariantContainerBase SearchProvider::ActivateResult_stub(const Glib::VariantContainerBase & params)
{
  if(params.get_n_children() != 3) {
    throw std::invalid_argument("Expected three arguments");
  }

  Glib::Variant<Glib::ustring> identifier;
  Glib::Variant<std::vector<Glib::ustring> > terms;
  Glib::Variant<guint32> timestamp;
  params.get_child(identifier, 0);
  params.get_child(terms, 1);
  params.get_child(timestamp, 2);
  ActivateResult(identifier.get(), terms.get(), timestamp.get());
  return Glib::VariantContainerBase();
}

Glib::VariantContainerBase SearchProvider::LaunchSearch_stub(const Glib::VariantContainerBase &)
{
  // this method does nothing
  return Glib::VariantContainerBase();
}

gchar *SearchProvider::get_icon()
{
  if(!m_note_icon) {
    Gtk::IconInfo info = gnote::IconManager::obj().lookup_icon(gnote::IconManager::NOTE, 48);
    m_note_icon = Gio::Icon::create(info.get_filename());
  }

  return g_icon_to_string(m_note_icon->gobj());
}


}
}
}
