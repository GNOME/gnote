/*
 * gnote
 *
 * Copyright (C) 2011 Aurimas Cernius
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


#include <string>

#include <giomm/dbusconnection.h>
#include <giomm/dbusinterfacevtable.h>

namespace org {
namespace gnome {
namespace Gnote {

class RemoteControl_adaptor
  : Gio::DBus::InterfaceVTable
{
public:
  RemoteControl_adaptor(const Glib::RefPtr<Gio::DBus::Connection> & conn,
                        const char *object_path, const char *interface_name,
                        const Glib::RefPtr<Gio::DBus::InterfaceInfo> & gnote_interface);

  virtual bool AddTagToNote(const std::string& uri, const std::string& tag_name) = 0;
  virtual std::string CreateNamedNote(const std::string& linked_title) = 0;
  virtual std::string CreateNote() = 0;
  virtual bool DeleteNote(const std::string& uri) = 0;
  virtual bool DisplayNote(const std::string& uri) = 0;
  virtual bool DisplayNoteWithSearch(const std::string& uri, const std::string& search) = 0;
  virtual void DisplaySearch() = 0;
  virtual void DisplaySearchWithText(const std::string& search_text) = 0;
  virtual std::string FindNote(const std::string& linked_title) = 0;
  virtual std::string FindStartHereNote() = 0;
  virtual std::vector<std::string> GetAllNotesWithTag(const std::string& tag_name) = 0;
  virtual int32_t GetNoteChangeDate(const std::string& uri) = 0;
  virtual std::string GetNoteCompleteXml(const std::string& uri) = 0;
  virtual std::string GetNoteContents(const std::string& uri) = 0;
  virtual std::string GetNoteContentsXml(const std::string& uri) = 0;
  virtual int32_t GetNoteCreateDate(const std::string& uri) = 0;
  virtual std::string GetNoteTitle(const std::string& uri) = 0;
  virtual std::vector<std::string> GetTagsForNote(const std::string& uri) = 0;
  virtual bool HideNote(const std::string& uri) = 0;
  virtual std::vector<std::string> ListAllNotes() = 0;
  virtual bool NoteExists(const std::string& uri) = 0;
  virtual bool RemoveTagFromNote(const std::string& uri, const std::string& tag_name) = 0;
  virtual std::vector<std::string> SearchNotes(const std::string& query, const bool& case_sensitive) = 0;
  virtual bool SetNoteCompleteXml(const std::string& uri, const std::string& xml_contents) = 0;
  virtual bool SetNoteContents(const std::string& uri, const std::string& text_contents) = 0;
  virtual bool SetNoteContentsXml(const std::string& uri, const std::string& xml_contents) = 0;
  virtual std::string Version() = 0;
  void NoteAdded(const std::string & );
  void NoteDeleted(const std::string &, const std::string &);
  void NoteSaved(const std::string &);
private:
  void on_method_call(const Glib::RefPtr<Gio::DBus::Connection> & connection,
                      const Glib::ustring & sender,
                      const Glib::ustring & object_path,
                      const Glib::ustring & interface_name,
                      const Glib::ustring & method_name,
                      const Glib::VariantContainerBase & parameters,
                      const Glib::RefPtr<Gio::DBus::MethodInvocation> & invocation);
  void emit_signal(const Glib::ustring & name, const Glib::VariantContainerBase & parameters);

  Glib::VariantContainerBase AddTagToNote_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase CreateNamedNote_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase CreateNote_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase DeleteNote_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase DisplayNote_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase DisplayNoteWithSearch_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase DisplaySearch_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase DisplaySearchWithText_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase FindNote_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase FindStartHereNote_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase GetAllNotesWithTag_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase GetNoteChangeDate_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase GetNoteCompleteXml_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase GetNoteContents_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase GetNoteContentsXml_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase GetNoteCreateDate_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase GetNoteTitle_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase GetTagsForNote_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase HideNote_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase ListAllNotes_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase NoteExists_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase RemoveTagFromNote_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase SearchNotes_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase SetNoteCompleteXml_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase SetNoteContents_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase SetNoteContentsXml_stub(const Glib::VariantContainerBase &);
  Glib::VariantContainerBase Version_stub(const Glib::VariantContainerBase &);

  typedef void (RemoteControl_adaptor::*void_string_func)(const std::string &);
  Glib::VariantContainerBase stub_void_string(const Glib::VariantContainerBase &, void_string_func);
  typedef bool (RemoteControl_adaptor::*bool_string_func)(const std::string &);
  Glib::VariantContainerBase stub_bool_string(const Glib::VariantContainerBase &, bool_string_func);
  typedef bool (RemoteControl_adaptor::*bool_string_string_func)(const std::string &, const std::string &);
  Glib::VariantContainerBase stub_bool_string_string(const Glib::VariantContainerBase &, bool_string_string_func);
  typedef int32_t (RemoteControl_adaptor::*int_string_func)(const std::string &);
  Glib::VariantContainerBase stub_int_string(const Glib::VariantContainerBase &, int_string_func);
  typedef std::string (RemoteControl_adaptor::*string_string_func)(const std::string &);
  Glib::VariantContainerBase stub_string_string(const Glib::VariantContainerBase &, string_string_func);
  typedef std::vector<std::string> (RemoteControl_adaptor::*vectorstring_void_func)();
  Glib::VariantContainerBase stub_vectorstring_void(const Glib::VariantContainerBase &, vectorstring_void_func);
  typedef std::vector<std::string> (RemoteControl_adaptor::*vectorstring_string_func)(const std::string &);
  Glib::VariantContainerBase stub_vectorstring_string(const Glib::VariantContainerBase &, vectorstring_string_func);
  typedef std::vector<std::string> (RemoteControl_adaptor::*vectorstring_string_bool_func)(const std::string &, const bool &);
  Glib::VariantContainerBase stub_vectorstring_string_bool(const Glib::VariantContainerBase &, vectorstring_string_bool_func);

  typedef Glib::VariantContainerBase (RemoteControl_adaptor::*stub_func)(const Glib::VariantContainerBase &);
  std::map<Glib::ustring, stub_func> m_stubs;
  Glib::RefPtr<Gio::DBus::Connection> m_connection;
  const char *m_path;
  const char *m_interface_name;
};

}
}
}
