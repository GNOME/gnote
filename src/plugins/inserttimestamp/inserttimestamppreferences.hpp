/*
 * gnote
 *
 * Copyright (C) 2013,2017,2019-2020,2023 Aurimas Cernius
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


#ifndef __INSERTTIMESTAMP_PREFERENCES_HPP_
#define __INSERTTIMESTAMP_PREFERENCES_HPP_


#include <giomm/liststore.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/entry.h>
#include <gtkmm/grid.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/listview.h>

#include "notemanager.hpp"

namespace gnote {
  class IGnote;
  class Preferences;
}


namespace inserttimestamp {

extern const char * SCHEMA_INSERT_TIMESTAMP;
extern const char * INSERT_TIMESTAMP_FORMAT;

class InsertTimestampPreferences
  : public Gtk::Grid
{
public:
  struct Columns
  {
    Glib::ustring formatted;
    Glib::ustring format;
  };
  typedef gnote::utils::ModelRecord<Columns> FormatColumns;

  static Glib::RefPtr<Gio::Settings> & settings();

  InsertTimestampPreferences(gnote::IGnote &, gnote::Preferences &, gnote::NoteManager &);
private:
  static void _init_static();

  void on_selected_radio_toggled();
  void on_selection_changed(guint position, guint n_items);

  static bool       s_static_inited;
  static std::vector<Glib::ustring> s_formats;
  static Glib::RefPtr<Gio::Settings> s_settings;

  Gtk::CheckButton *selected_radio;
  Gtk::CheckButton *custom_radio;

  Gtk::ScrolledWindow         *scroll;
  Gtk::ListView               *m_list;
  Glib::RefPtr<Gio::ListStore<FormatColumns>> m_store;
  Gtk::Entry                  *custom_entry;
};


}

#endif
