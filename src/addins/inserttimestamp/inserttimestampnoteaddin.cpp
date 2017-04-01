/*
 * gnote
 *
 * Copyright (C) 2010-2013,2016-2017 Aurimas Cernius
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
#include "iactionmanager.hpp"
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
    register_main_window_action_callback("inserttimestamp-insert",
      sigc::mem_fun(*this, &InsertTimestampNoteAddin::on_menu_item_activated));

    Glib::RefPtr<Gio::Settings> settings = gnote::Preferences::obj().get_schema_settings(SCHEMA_INSERT_TIMESTAMP);
    m_date_format = settings->get_string(INSERT_TIMESTAMP_FORMAT);
    settings->signal_changed().connect(
      sigc::mem_fun(*this, &InsertTimestampNoteAddin::on_format_setting_changed));
  }


  std::map<int, Gtk::Widget*> InsertTimestampNoteAddin::get_actions_popover_widgets() const
  {
    auto widgets = NoteAddin::get_actions_popover_widgets();
    auto button = gnote::utils::create_popover_button("win.inserttimestamp-insert", _("Insert Timestamp"));
    gnote::utils::add_item_to_ordered_map(widgets, gnote::INSERT_TIMESTAMP_ORDER, button);
    return widgets;
  }


  void InsertTimestampNoteAddin::on_menu_item_activated(const Glib::VariantBase&)
  {
    Glib::ustring text = sharp::DateTime::now().to_string(m_date_format);
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

