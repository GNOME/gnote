/*
 * gnote
 *
 * Copyright (C) 2017,2019-2020 Aurimas Cernius
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


#ifndef __EXPORT_TO_HTML_DIALOG_HPP_
#define __EXPORT_TO_HTML_DIALOG_HPP_

#include <giomm/settings.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/filechooserdialog.h>


namespace gnote {
  class IGnote;
}


namespace exporttohtml {

class ExportToHtmlDialog
  : public Gtk::FileChooserDialog
{
public:
  ExportToHtmlDialog(gnote::IGnote & ignote, const Glib::ustring &);
  void save_preferences();

  bool get_export_linked() const;
  void set_export_linked(bool);
  bool get_export_linked_all() const;
  void set_export_linked_all(bool);

private:
  void on_export_linked_toggled();
  void load_preferences(const Glib::ustring & );
  gnote::IGnote & m_gnote;
  Gtk::CheckButton m_export_linked;
  Gtk::CheckButton m_export_linked_all;
  Glib::RefPtr<Gio::Settings> m_settings;
};


}


#endif
