/*
 * gnote
 *
 * Copyright (C) 2013-2014,2016-2017,2019,2023 Aurimas Cernius
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
#include <glibmm/i18n.h>

#include "debug.hpp"

#include "exporttogtgnoteaddin.hpp"
#include "iactionmanager.hpp"
#include "notewindow.hpp"
#include "sharp/string.hpp"


namespace {

const char *GTG_INTERFACE =
"<?xml version=\"1.0\" ?>"
"<node name=\"/org/gnome/GTG\">"
"  <interface name=\"org.gnome.GTG\">"
"    <method name=\"OpenNewTask\">"
"      <arg type=\"s\" name=\"title\" direction=\"in\"/>"
"      <arg type=\"s\" name=\"description\" direction=\"in\"/>"
"    </method>"
"  </interface>"
"</node>";

}


namespace exporttogtg {


ExportToGTGModule::ExportToGTGModule()
{
  ADD_INTERFACE_IMPL(ExportToGTGNoteAddin);
}


Glib::RefPtr<Gio::DBus::InterfaceInfo> ExportToGTGNoteAddin::s_gtg_interface;


void ExportToGTGNoteAddin::initialize()
{
}

void ExportToGTGNoteAddin::shutdown()
{
}

void ExportToGTGNoteAddin::on_note_opened()
{
  register_main_window_action_callback("exporttogtg-export", sigc::mem_fun(*this,
    &ExportToGTGNoteAddin::export_button_clicked));
}


std::vector<gnote::PopoverWidget> ExportToGTGNoteAddin::get_actions_popover_widgets() const
{
  auto widgets = NoteAddin::get_actions_popover_widgets();
  auto item = Gio::MenuItem::create(_("Export to Getting Things GNOME"), "win.exporttogtg-export");
  widgets.push_back(gnote::PopoverWidget::create_for_note(gnote::EXPORT_TO_GTG_ORDER, item));
  return widgets;
}


void ExportToGTGNoteAddin::export_button_clicked(const Glib::VariantBase&)
{
  try {
    if (!s_gtg_interface) {
      Glib::RefPtr<Gio::DBus::NodeInfo> node_info = Gio::DBus::NodeInfo::create_for_xml(GTG_INTERFACE);
      s_gtg_interface = node_info->lookup_interface("org.gnome.GTG");
      if(!s_gtg_interface) {
        ERR_OUT(_("GTG XML loaded, but interface not found"));
        return;
      }
    }
  }
  catch(Glib::Error & e) {
    ERR_OUT(_("Failed to create GTG interface from XML: %s"), e.what());
    return;
  }

  try {
    Glib::RefPtr<Gio::DBus::Proxy> proxy = Gio::DBus::Proxy::create_for_bus_sync(
        Gio::DBus::BusType::SESSION, "org.gnome.GTG", "/org/gnome/GTG", "org.gnome.GTG", s_gtg_interface);
    if(!proxy) {
      ERR_OUT(_("Failed to create D-Bus proxy for GTG"));
      return;
    }

    auto & note = get_note();
    Glib::ustring title = note.get_title();
    Glib::ustring body = sharp::string_trim(sharp::string_replace_first(note.text_content(), title, ""));

    std::vector<Glib::VariantBase> parameters;
    parameters.reserve(2);
    parameters.push_back(Glib::Variant<Glib::ustring>::create(title));
    parameters.push_back(Glib::Variant<Glib::ustring>::create(body));
    Glib::VariantContainerBase params = Glib::VariantContainerBase::create_tuple(parameters);
    proxy->call_sync("OpenNewTask", params);
  }
  catch(Glib::Error & e) {
    ERR_OUT(_("Failed to call GTG: %s"), e.what());
  }
}

}
