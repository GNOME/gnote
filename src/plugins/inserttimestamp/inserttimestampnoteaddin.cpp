/*
 * gnote
 *
 * Copyright (C) 2010-2013,2016-2017,2019-2021,2023 Aurimas Cernius
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

#include "debug.hpp"
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

    get_window()->signal_foregrounded.connect(sigc::mem_fun(*this, &InsertTimestampNoteAddin::on_note_foregrounded));
    get_window()->signal_backgrounded.connect(sigc::mem_fun(*this, &InsertTimestampNoteAddin::on_note_backgrounded));
  }


  std::vector<gnote::PopoverWidget> InsertTimestampNoteAddin::get_actions_popover_widgets() const
  {
    auto widgets = NoteAddin::get_actions_popover_widgets();
    auto item = Gio::MenuItem::create(_("Insert Timestamp"), "win.inserttimestamp-insert");
    widgets.push_back(gnote::PopoverWidget::create_for_note(gnote::INSERT_TIMESTAMP_ORDER, item));
    return widgets;
  }


  void InsertTimestampNoteAddin::on_note_foregrounded()
  {
    auto window = dynamic_cast<gnote::MainWindow*>(get_window()->host());
    if(!window) {
      ERR_OUT("No host on foregrounded note window");
      return;
    }

    auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_D, Gdk::ModifierType::CONTROL_MASK);
    auto action = Gtk::NamedAction::create("win.inserttimestamp-insert");
    m_shortcut = Gtk::Shortcut::create(trigger, action);
    get_note().get_window()->shortcut_controller().add_shortcut(m_shortcut);
  }


  void InsertTimestampNoteAddin::on_note_backgrounded()
  {
    if(m_shortcut) {
      auto window = get_note().get_window();
      if(window) {
        window->shortcut_controller().remove_shortcut(m_shortcut);
      }

      m_shortcut.reset();
    }
  }


  void InsertTimestampNoteAddin::on_menu_item_activated(const Glib::VariantBase&)
  {
    on_insert_activated();
  }


  void InsertTimestampNoteAddin::on_insert_activated()
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

