/*
 * gnote
 *
 * Copyright (C) 2010,2013,2016-2017,2019,2021,2023 Aurimas Cernius
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





#ifndef __INSERTIMESTAMP_NOTEADDIN_HPP_
#define __INSERTIMESTAMP_NOTEADDIN_HPP_

#include <gtkmm/shortcut.h>

#include "sharp/dynamicmodule.hpp"
#include "noteaddin.hpp"

namespace inserttimestamp {

class InsertTimeStampModule
  : public sharp::DynamicModule
{
public:
  InsertTimeStampModule();
};


DECLARE_MODULE(InsertTimeStampModule);


class InsertTimestampNoteAddin
  : public gnote::NoteAddin 
{
public:
  static InsertTimestampNoteAddin* create()
    {
      return new InsertTimestampNoteAddin;
    }
  InsertTimestampNoteAddin() {}
  virtual void initialize() override;
  virtual void shutdown() override;
  virtual void on_note_opened() override;
  virtual std::vector<gnote::PopoverWidget> get_actions_popover_widgets() const override;
private:
  static void on_format_setting_changed(const Glib::ustring & key);

  static Glib::ustring s_date_format;
  static sigc::connection s_on_format_setting_changed_cid;

  void on_note_foregrounded();
  void on_note_backgrounded();
  void on_menu_item_activated(const Glib::VariantBase&);
  void on_insert_activated();

  Glib::RefPtr<Gtk::Shortcut> m_shortcut;
};

}

#endif
