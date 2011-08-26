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

#include <giomm/dbusproxy.h>

namespace org {
namespace gnome {
namespace Gnote {

class RemoteControl_proxy
  : public Gio::DBus::Proxy
{
public:
  RemoteControl_proxy(const Glib::RefPtr<Gio::DBus::Connection> & connection,
                      const char * name, const char * p, const char * interface_name,
                      const Glib::RefPtr<Gio::DBus::InterfaceInfo> & gnote_interface);

  std::string CreateNamedNote(const std::string & linked_title);
  std::string CreateNote();
  bool DisplayNote(const std::string & uri);
  bool DisplayNoteWithSearch(const std::string & uri, const std::string & search);
  void DisplaySearch();
  std::string FindNote(const std::string & linked_title);
  std::string FindStartHereNote();
  void DisplaySearchWithText(const std::string & search_text);
  bool SetNoteCompleteXml(const std::string & uri, const std::string & xml_contents);
private:
  Glib::VariantContainerBase call_remote(const Glib::ustring & method_name, const Glib::VariantContainerBase & parameters);
};

}
}
}
