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

  namespace {
    class InsertTimestampAction
      : public Gtk::Action
    {
    public:
      static Glib::RefPtr<Gtk::Action> create(gnote::NoteWindow *note_window)
        {
          return Glib::RefPtr<Gtk::Action>(new InsertTimestampAction(note_window));
        }
    private:
      InsertTimestampAction(gnote::NoteWindow *note_window)
        : Gtk::Action("InsertTimestampAction", "", _("Insert Timestamp"),
                      _("Insert Timestamp into note"))
        , m_note_window(note_window)
      {}

      virtual Gtk::Widget *create_menu_item_vfunc()
        {
          Gtk::MenuItem *item = new Gtk::MenuItem;
          item->add_accelerator("activate", m_note_window->get_accel_group(),
                                GDK_KEY_d, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
          return item;
        }

      gnote::NoteWindow *m_note_window;
    };
  }

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
    Glib::RefPtr<Gtk::Action> action = InsertTimestampAction::create(get_window());
    action->signal_activate().connect(
      sigc::mem_fun(*this, &InsertTimestampNoteAddin::on_menu_item_activated));
    add_note_action(action, 300);

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

