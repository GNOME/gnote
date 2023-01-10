/*
 * gnote
 *
 * Copyright (C) 2011-2012,2017,2019-2021,2023 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
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
#include <glibmm/miscutils.h>

#include "sharp/files.hpp"
#include "exporttohtmldialog.hpp"
#include "ignote.hpp"

namespace exporttohtml {

const char * SCHEMA_EXPORTHTML = "org.gnome.gnote.export-html";
const char * EXPORTHTML_LAST_DIRECTORY = "last-directory";
const char * EXPORTHTML_EXPORT_LINKED = "export-linked";
const char * EXPORTHTML_EXPORT_LINKED_ALL = "export-linked-all";


ExportToHtmlDialog::ExportToHtmlDialog(gnote::IGnote & ignote, const Glib::ustring & default_file)
  : Gtk::FileChooserDialog(_("Destination for HTML Export"), Gtk::FileChooser::Action::SAVE)
  , m_gnote(ignote)
  , m_export_linked(_("Export linked notes"))
  , m_export_linked_all(_("Include all other linked notes"))
  , m_settings(Gio::Settings::create(SCHEMA_EXPORTHTML))
{
  add_button(_("_Cancel"), Gtk::ResponseType::CANCEL);
  add_button(_("_Save"), Gtk::ResponseType::OK);

  set_default_response(Gtk::ResponseType::OK);

  auto table = Gtk::make_managed<Gtk::Grid>();

  m_export_linked.signal_toggled().connect(
    sigc::mem_fun(*this, &ExportToHtmlDialog::on_export_linked_toggled));

  table->attach(m_export_linked, 0, 0, 2, 1);
  table->attach(m_export_linked_all, 0, 1, 2, 1);

  get_content_area()->append(*table);

  load_preferences(default_file);
}


bool ExportToHtmlDialog::get_export_linked() const
{
  return m_export_linked.get_active();
}


void ExportToHtmlDialog::set_export_linked(bool value)
{
  m_export_linked.set_active(value);
}


bool ExportToHtmlDialog::get_export_linked_all() const
{
  return m_export_linked_all.get_active();
}


void ExportToHtmlDialog::set_export_linked_all(bool value)
{
  m_export_linked_all.set_active(value);
}


void ExportToHtmlDialog::save_preferences()
{
  Glib::ustring dir = sharp::file_dirname(get_file()->get_path());
  m_settings->set_string(EXPORTHTML_LAST_DIRECTORY, dir);
  m_settings->set_boolean(EXPORTHTML_EXPORT_LINKED, get_export_linked());
  m_settings->set_boolean(EXPORTHTML_EXPORT_LINKED_ALL, get_export_linked_all());
}


void ExportToHtmlDialog::load_preferences(const Glib::ustring & default_file)
{
  Glib::ustring last_dir = m_settings->get_string(EXPORTHTML_LAST_DIRECTORY);
  if (last_dir.empty()) {
    last_dir = Glib::get_home_dir();
  }
  set_current_folder(Gio::File::create_for_path(last_dir));
  set_current_name(default_file);

  set_export_linked(m_settings->get_boolean(EXPORTHTML_EXPORT_LINKED));
  set_export_linked_all(m_settings->get_boolean(EXPORTHTML_EXPORT_LINKED_ALL));
}


void ExportToHtmlDialog::on_export_linked_toggled()
{
  if (m_export_linked.get_active()) {
    m_export_linked_all.set_sensitive(true);
  }
  else {
    m_export_linked_all.set_sensitive(false);
  }
}



}
