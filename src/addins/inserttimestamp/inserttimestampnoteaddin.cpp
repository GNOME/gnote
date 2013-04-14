/*
 * gnote
 *
 * Copyright (C) 2010-2013 Aurimas Cernius
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

#include <string.h>

#include <glibmm/i18n.h>

#include "sharp/datetime.hpp"
#include "inserttimestamppreferences.hpp"
#include "inserttimestamppreferencesfactory.hpp"
#include "inserttimestampnoteaddin.hpp"
#include "notewindow.hpp"

namespace inserttimestamp {

  InsertTimeStampModule::InsertTimeStampModule()
  {
    ADD_INTERFACE_IMPL(InsertTimestampNoteAddin);
    ADD_INTERFACE_IMPL(InsertTimestampPreferencesFactory);
    enabled(false);
  }


  void InsertTimestampNoteAddin::initialize()
  {
  }


  void InsertTimestampNoteAddin::shutdown()
  {
  }


  void InsertTimestampNoteAddin::on_note_opened()
  {
    // Add the menu item when the window is created
    m_item = manage(new Gtk::MenuItem(_("Insert Timestamp")));
    m_item->signal_activate().connect(
      sigc::mem_fun(*this, &InsertTimestampNoteAddin::on_menu_item_activated));
    m_item->add_accelerator ("activate", get_window()->get_accel_group(),
                             GDK_KEY_d, Gdk::CONTROL_MASK,
                             Gtk::ACCEL_VISIBLE);
    m_item->show ();
    add_plugin_menu_item (m_item);

    Glib::RefPtr<Gio::Settings> settings = gnote::Preferences::obj().get_schema_settings(SCHEMA_INSERT_TIMESTAMP);
    m_date_format = settings->get_string(INSERT_TIMESTAMP_FORMAT);
    settings->signal_changed().connect(
      sigc::mem_fun(*this, &InsertTimestampNoteAddin::on_format_setting_changed));
  }


  void InsertTimestampNoteAddin::on_menu_item_activated()
  {
    std::string text = sharp::DateTime::now().to_string(m_date_format);
    Gtk::TextIter cursor = get_buffer()->get_iter_at_mark (get_buffer()->get_insert());
    std::vector<Glib::ustring> names;
    names.push_back("datetime");
    get_buffer()->insert_with_tags_by_name (cursor, text, names);
  }


  void InsertTimestampNoteAddin::on_format_setting_changed(const Glib::ustring & key)
  {
    if(key == INSERT_TIMESTAMP_FORMAT) {
      m_date_format = gnote::Preferences::obj().get_schema_settings(
          SCHEMA_INSERT_TIMESTAMP)->get_string(INSERT_TIMESTAMP_FORMAT);
    }
  }

}

