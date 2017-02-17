/*
 * gnote
 *
 * Copyright (C) 2011,2017 Aurimas Cernius
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

  Glib::ustring CreateNamedNote(const Glib::ustring & linked_title);
  Glib::ustring CreateNote();
  bool DisplayNote(const Glib::ustring & uri);
  bool DisplayNoteWithSearch(const Glib::ustring & uri, const Glib::ustring & search);
  void DisplaySearch();
  Glib::ustring FindNote(const Glib::ustring & linked_title);
  Glib::ustring FindStartHereNote();
  void DisplaySearchWithText(const Glib::ustring & search_text);
  bool SetNoteCompleteXml(const Glib::ustring & uri, const Glib::ustring & xml_contents);
private:
  Glib::VariantContainerBase call_remote(const Glib::ustring & method_name, const Glib::VariantContainerBase & parameters);
};

}
}
}
