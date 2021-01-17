/*
 * gnote
 *
 * Copyright (C) 2010-2013,2016-2017,2019-2021 Aurimas Cernius
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
#include "ignote.hpp"
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


  Glib::ustring InsertTimestampNoteAddin::s_date_format;
  sigc::connection InsertTimestampNoteAddin::s_on_format_setting_changed_cid;

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

    if(s_on_format_setting_changed_cid.empty()) {
      s_on_format_setting_changed_cid = InsertTimestampPreferences::settings()->signal_changed(INSERT_TIMESTAMP_FORMAT)
        .connect(sigc::ptr_fun(InsertTimestampNoteAddin::on_format_setting_changed));
      s_date_format = InsertTimestampPreferences::settings()->get_string(INSERT_TIMESTAMP_FORMAT);
    }
  }


  std::vector<gnote::PopoverWidget> InsertTimestampNoteAddin::get_actions_popover_widgets() const
  {
    auto widgets = NoteAddin::get_actions_popover_widgets();
    auto button = gnote::utils::create_popover_button("win.inserttimestamp-insert", _("Insert Timestamp"));
    widgets.push_back(gnote::PopoverWidget::create_for_note(gnote::INSERT_TIMESTAMP_ORDER, button));
    return widgets;
  }


  void InsertTimestampNoteAddin::on_menu_item_activated(const Glib::VariantBase&)
  {
    Glib::ustring text = sharp::date_time_to_string(Glib::DateTime::create_now_local(), s_date_format);
    Gtk::TextIter cursor = get_buffer()->get_iter_at_mark (get_buffer()->get_insert());
    std::vector<Glib::ustring> names;
    names.push_back("datetime");
    get_buffer()->insert_with_tags_by_name (cursor, text, names);
  }


  void InsertTimestampNoteAddin::on_format_setting_changed(const Glib::ustring &)
  {
    s_date_format = InsertTimestampPreferences::settings()->get_string(INSERT_TIMESTAMP_FORMAT);
  }

}

