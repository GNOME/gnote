/*
 * gnote
 *
 * Copyright (C) 2011,2017,2020,2022 Aurimas Cernius
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

#include <gio/gio.h>
#include <giomm/dbuserror.h>

#include "debug.hpp"
#include "remotecontrol-glue.hpp"

using namespace org::gnome::Gnote;

RemoteControl_adaptor::RemoteControl_adaptor(const Glib::RefPtr<Gio::DBus::Connection> & conn,
                                             const char *object_path,
                                             const char *interface_name,
                                             const Glib::RefPtr<Gio::DBus::InterfaceInfo> & gnote_interface)
  : Gio::DBus::InterfaceVTable(sigc::mem_fun(*this, &RemoteControl_adaptor::on_method_call))
  , m_connection(conn)
  , m_path(object_path)
  , m_interface_name(interface_name)
{
  conn->register_object(object_path, gnote_interface, *this);

  m_stubs["AddTagToNote"] = &RemoteControl_adaptor::AddTagToNote_stub;
  m_stubs["CreateNamedNote"] = &RemoteControl_adaptor::CreateNamedNote_stub;
  m_stubs["CreateNote"] = &RemoteControl_adaptor::CreateNote_stub;
  m_stubs["DeleteNote"] = &RemoteControl_adaptor::DeleteNote_stub;
  m_stubs["DisplayNote"] = &RemoteControl_adaptor::DisplayNote_stub;
  m_stubs["DisplayNoteWithSearch"] = &RemoteControl_adaptor::DisplayNoteWithSearch_stub;
  m_stubs["DisplaySearch"] = &RemoteControl_adaptor::DisplaySearch_stub;
  m_stubs["DisplaySearchWithText"] = &RemoteControl_adaptor::DisplaySearchWithText_stub;
  m_stubs["FindNote"] = &RemoteControl_adaptor::FindNote_stub;
  m_stubs["FindStartHereNote"] = &RemoteControl_adaptor::FindStartHereNote_stub;
  m_stubs["GetAllNotesWithTag"] = &RemoteControl_adaptor::GetAllNotesWithTag_stub;
  m_stubs["GetNoteChangeDate"] = &RemoteControl_adaptor::GetNoteChangeDate_stub;
  m_stubs["GetNoteChangeDateUnix"] = &RemoteControl_adaptor::GetNoteChangeDateUnix_stub;
  m_stubs["GetNoteCompleteXml"] = &RemoteControl_adaptor::GetNoteCompleteXml_stub;
  m_stubs["GetNoteContents"] = &RemoteControl_adaptor::GetNoteContents_stub;
  m_stubs["GetNoteContentsXml"] = &RemoteControl_adaptor::GetNoteContentsXml_stub;
  m_stubs["GetNoteCreateDate"] = &RemoteControl_adaptor::GetNoteCreateDate_stub;
  m_stubs["GetNoteCreateDateUnix"] = &RemoteControl_adaptor::GetNoteCreateDateUnix_stub;
  m_stubs["GetNoteTitle"] = &RemoteControl_adaptor::GetNoteTitle_stub;
  m_stubs["GetTagsForNote"] = &RemoteControl_adaptor::GetTagsForNote_stub;
  m_stubs["HideNote"] = &RemoteControl_adaptor::HideNote_stub;
  m_stubs["ListAllNotes"] = &RemoteControl_adaptor::ListAllNotes_stub;
  m_stubs["NoteExists"] = &RemoteControl_adaptor::NoteExists_stub;
  m_stubs["RemoveTagFromNote"] = &RemoteControl_adaptor::RemoveTagFromNote_stub;
  m_stubs["SearchNotes"] = &RemoteControl_adaptor::SearchNotes_stub;
  m_stubs["SetNoteCompleteXml"] = &RemoteControl_adaptor::SetNoteCompleteXml_stub;
  m_stubs["SetNoteContents"] = &RemoteControl_adaptor::SetNoteContents_stub;
  m_stubs["SetNoteContentsXml"] = &RemoteControl_adaptor::SetNoteContentsXml_stub;
  m_stubs["Version"] = &RemoteControl_adaptor::Version_stub;
}

void RemoteControl_adaptor::NoteAdded(const Glib::ustring & uri)
{
  emit_signal("NoteAdded", Glib::VariantContainerBase::create_tuple(Glib::Variant<Glib::ustring>::create(uri)));
}

void RemoteControl_adaptor::NoteDeleted(const Glib::ustring & uri, const Glib::ustring & title)
{
  std::vector<Glib::VariantBase> parameters;
  parameters.push_back(Glib::Variant<Glib::ustring>::create(uri));
  parameters.push_back(Glib::Variant<Glib::ustring>::create(title));
  emit_signal("NoteDeleted", Glib::VariantContainerBase::create_tuple(parameters));
}

void RemoteControl_adaptor::NoteSaved(const Glib::ustring & uri)
{
  emit_signal("NoteSaved", Glib::VariantContainerBase::create_tuple(Glib::Variant<Glib::ustring>::create(uri)));
}

void RemoteControl_adaptor::on_method_call(const Glib::RefPtr<Gio::DBus::Connection> &,
                                           const Glib::ustring &,
                                           const Glib::ustring &,
                                           const Glib::ustring &,
                                           const Glib::ustring & method_name,
                                           const Glib::VariantContainerBase & parameters,
                                           const Glib::RefPtr<Gio::DBus::MethodInvocation> & invocation)
{
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


void RemoteControl_adaptor::emit_signal(const Glib::ustring & name, const Glib::VariantContainerBase & parameters)
{
  //work-around glibmm bug 645072
  g_dbus_connection_emit_signal(m_connection->gobj(), NULL, m_path, m_interface_name, name.c_str(),
                                  const_cast<GVariant*>(parameters.gobj()), NULL);
}


Glib::VariantContainerBase RemoteControl_adaptor::AddTagToNote_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_bool_string_string(parameters, &RemoteControl_adaptor::AddTagToNote);
}


Glib::VariantContainerBase RemoteControl_adaptor::CreateNamedNote_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_string_string(parameters, &RemoteControl_adaptor::CreateNamedNote);
}


Glib::VariantContainerBase RemoteControl_adaptor::CreateNote_stub(const Glib::VariantContainerBase &)
{
  return Glib::VariantContainerBase::create_tuple(Glib::Variant<Glib::ustring>::create(CreateNote()));
}


Glib::VariantContainerBase RemoteControl_adaptor::DeleteNote_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_bool_string(parameters, &RemoteControl_adaptor::DeleteNote);
}


Glib::VariantContainerBase RemoteControl_adaptor::DisplayNote_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_bool_string(parameters, &RemoteControl_adaptor::DisplayNote);
}


Glib::VariantContainerBase RemoteControl_adaptor::DisplayNoteWithSearch_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_bool_string_string(parameters, &RemoteControl_adaptor::DisplayNoteWithSearch);
}


Glib::VariantContainerBase RemoteControl_adaptor::DisplaySearch_stub(const Glib::VariantContainerBase &)
{
  DisplaySearch();
  return Glib::VariantContainerBase();
}


Glib::VariantContainerBase RemoteControl_adaptor::DisplaySearchWithText_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_void_string(parameters, &RemoteControl_adaptor::DisplaySearchWithText);
}


Glib::VariantContainerBase RemoteControl_adaptor::FindNote_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_string_string(parameters, &RemoteControl_adaptor::FindNote);
}


Glib::VariantContainerBase RemoteControl_adaptor::FindStartHereNote_stub(const Glib::VariantContainerBase &)
{
  return Glib::VariantContainerBase::create_tuple(Glib::Variant<Glib::ustring>::create(FindStartHereNote()));
}


Glib::VariantContainerBase RemoteControl_adaptor::GetAllNotesWithTag_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_vectorstring_string(parameters, &RemoteControl_adaptor::GetAllNotesWithTag);
}


Glib::VariantContainerBase RemoteControl_adaptor::GetNoteChangeDate_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_int_string(parameters, &RemoteControl_adaptor::GetNoteChangeDate);
}


Glib::VariantContainerBase RemoteControl_adaptor::GetNoteChangeDateUnix_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_int64_string(parameters, &RemoteControl_adaptor::GetNoteChangeDateUnix);
}


Glib::VariantContainerBase RemoteControl_adaptor::GetNoteCompleteXml_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_string_string(parameters, &RemoteControl_adaptor::GetNoteCompleteXml);
}


Glib::VariantContainerBase RemoteControl_adaptor::GetNoteContents_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_string_string(parameters, &RemoteControl_adaptor::GetNoteContents);
}


Glib::VariantContainerBase RemoteControl_adaptor::GetNoteContentsXml_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_string_string(parameters, &RemoteControl_adaptor::GetNoteContentsXml);
}


Glib::VariantContainerBase RemoteControl_adaptor::GetNoteCreateDate_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_int_string(parameters, &RemoteControl_adaptor::GetNoteCreateDate);
}


Glib::VariantContainerBase RemoteControl_adaptor::GetNoteCreateDateUnix_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_int64_string(parameters, &RemoteControl_adaptor::GetNoteCreateDateUnix);
}


Glib::VariantContainerBase RemoteControl_adaptor::GetNoteTitle_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_string_string(parameters, &RemoteControl_adaptor::GetNoteTitle);
}


Glib::VariantContainerBase RemoteControl_adaptor::GetTagsForNote_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_vectorstring_string(parameters, &RemoteControl_adaptor::GetTagsForNote);
}


Glib::VariantContainerBase RemoteControl_adaptor::HideNote_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_bool_string(parameters, &RemoteControl_adaptor::HideNote);
}


Glib::VariantContainerBase RemoteControl_adaptor::ListAllNotes_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_vectorstring_void(parameters, &RemoteControl_adaptor::ListAllNotes);
}


Glib::VariantContainerBase RemoteControl_adaptor::NoteExists_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_bool_string(parameters, &RemoteControl_adaptor::NoteExists);
}


Glib::VariantContainerBase RemoteControl_adaptor::RemoveTagFromNote_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_bool_string_string(parameters, &RemoteControl_adaptor::RemoveTagFromNote);
}


Glib::VariantContainerBase RemoteControl_adaptor::SearchNotes_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_vectorstring_string_bool(parameters, &RemoteControl_adaptor::SearchNotes);
}


Glib::VariantContainerBase RemoteControl_adaptor::SetNoteCompleteXml_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_bool_string_string(parameters, &RemoteControl_adaptor::SetNoteCompleteXml);
}


Glib::VariantContainerBase RemoteControl_adaptor::SetNoteContents_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_bool_string_string(parameters, &RemoteControl_adaptor::SetNoteContents);
}


Glib::VariantContainerBase RemoteControl_adaptor::SetNoteContentsXml_stub(const Glib::VariantContainerBase & parameters)
{
  return stub_bool_string_string(parameters, &RemoteControl_adaptor::SetNoteContentsXml);
}


Glib::VariantContainerBase RemoteControl_adaptor::Version_stub(const Glib::VariantContainerBase &)
{
  return Glib::VariantContainerBase::create_tuple(Glib::Variant<Glib::ustring>::create(Version()));
}


Glib::VariantContainerBase RemoteControl_adaptor::stub_void_string(const Glib::VariantContainerBase & parameters,
                                                                   void_string_func func)
{
  if(parameters.get_n_children() == 1) {
    Glib::Variant<Glib::ustring> param;
    parameters.get_child(param);
    (this->*func)(param.get());
  }

  return Glib::VariantContainerBase();
}


Glib::VariantContainerBase RemoteControl_adaptor::stub_bool_string(const Glib::VariantContainerBase & parameters,
                                                                   bool_string_func func)
{
  bool result = false;
  if(parameters.get_n_children() == 1) {
    Glib::Variant<Glib::ustring> param;
    parameters.get_child(param);
    result = (this->*func)(param.get());
  }

  return Glib::VariantContainerBase::create_tuple(Glib::Variant<bool>::create(result));
}


Glib::VariantContainerBase RemoteControl_adaptor::stub_bool_string_string(const Glib::VariantContainerBase & parameters,
                                                                          bool_string_string_func func)
{
  bool result = false;
  if(parameters.get_n_children() == 2) {
    Glib::Variant<Glib::ustring> param1;
    parameters.get_child(param1, 0);
    Glib::Variant<Glib::ustring> param2;
    parameters.get_child(param2, 1);
    result = (this->*func)(param1.get(), param2.get());
  }

  return Glib::VariantContainerBase::create_tuple(Glib::Variant<bool>::create(result));
}


Glib::VariantContainerBase RemoteControl_adaptor::stub_int_string(const Glib::VariantContainerBase & parameters, int_string_func func)
{
  gint32 result = 0;
  if(parameters.get_n_children() == 1) {
    Glib::Variant<Glib::ustring> param;
    parameters.get_child(param);
    result = (this->*func)(param.get());
  }

  return Glib::VariantContainerBase::create_tuple(Glib::Variant<gint32>::create(result));
}


Glib::VariantContainerBase RemoteControl_adaptor::stub_int64_string(const Glib::VariantContainerBase & parameters, int64_string_func func)
{
  gint64 result = 0;
  if(parameters.get_n_children() == 1) {
    Glib::Variant<Glib::ustring> param;
    parameters.get_child(param);
    result = (this->*func)(param.get());
  }

  return Glib::VariantContainerBase::create_tuple(Glib::Variant<gint64>::create(result));
}


Glib::VariantContainerBase RemoteControl_adaptor::stub_string_string(const Glib::VariantContainerBase & parameters,
                                                                     string_string_func func)
{
  Glib::ustring result;
  if(parameters.get_n_children() == 1) {
    Glib::Variant<Glib::ustring> param;
    parameters.get_child(param);
    result = (this->*func)(param.get());
  }

  return Glib::VariantContainerBase::create_tuple(Glib::Variant<Glib::ustring>::create(result));
}


Glib::VariantContainerBase RemoteControl_adaptor::stub_vectorstring_void(const Glib::VariantContainerBase &,
                                                                         vectorstring_void_func func)
{
  std::vector<Glib::ustring> result = (this->*func)();

  return Glib::VariantContainerBase::create_tuple(Glib::Variant<std::vector<Glib::ustring> >::create(result));
}


Glib::VariantContainerBase RemoteControl_adaptor::stub_vectorstring_string(const Glib::VariantContainerBase & parameters,
                                                                           vectorstring_string_func func)
{
  std::vector<Glib::ustring> result;
  if(parameters.get_n_children() == 1) {
    Glib::Variant<Glib::ustring> param;
    parameters.get_child(param);
    result = (this->*func)(param.get());
  }

  return Glib::VariantContainerBase::create_tuple(Glib::Variant<std::vector<Glib::ustring> >::create(result));
}


Glib::VariantContainerBase RemoteControl_adaptor::stub_vectorstring_string_bool(const Glib::VariantContainerBase & parameters,
                                                                                vectorstring_string_bool_func func)
{
  std::vector<Glib::ustring> result;
  if(parameters.get_n_children() == 2) {
    Glib::Variant<Glib::ustring> param1;
    parameters.get_child(param1, 0);
    Glib::Variant<bool> param2;
    parameters.get_child(param2, 1);
    result = (this->*func)(param1.get(), param2.get());
  }

  return Glib::VariantContainerBase::create_tuple(Glib::Variant<std::vector<Glib::ustring> >::create(result));
}

