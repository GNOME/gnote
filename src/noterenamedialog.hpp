/*
 * gnote
 *
 * Copyright (C) 2011,2013-2014,2017,2019,2022-2023,2025 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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

#ifndef __NOTE_RENAME_DIALOG_HPP_
#define __NOTE_RENAME_DIALOG_HPP_

#include <map>

#include <giomm/liststore.h>
#include <gtkmm/grid.h>
#include <gtkmm/checkbutton.h>

#include "note.hpp"

namespace gnote {

class IGnote;


// Values should match with those in data/gnote.schemas.in
enum NoteRenameBehavior {
  NOTE_RENAME_ALWAYS_SHOW_DIALOG = 0,
  NOTE_RENAME_ALWAYS_REMOVE_LINKS = 1,
  NOTE_RENAME_ALWAYS_RENAME_LINKS = 2
};

class NoteRenameRecord
  : public Glib::Object
{
public:
  static Glib::RefPtr<NoteRenameRecord> create(const NoteBase & note, bool selected);
  virtual ~NoteRenameRecord();

  bool selected() const
    {
      return m_selected;
    }
  void selected(bool select);
  void set_check_button(Gtk::CheckButton *button)
    {
      m_check_button = button;
    }

  const Glib::ustring note_uri;
  const Glib::ustring note_title;
  sigc::connection signal_cid;
private:
  NoteRenameRecord(const NoteBase & note, bool selected);

  Gtk::CheckButton *m_check_button;
  bool m_selected;
};

class NoteRenameDialog
  : public Gtk::Window
{
public:
  typedef std::map<Glib::ustring, bool> Map;
  enum class Response
  {
    RENAME,
    DONT_RENAME,
    CANCEL,
  };

  NoteRenameDialog(const std::vector<NoteBase::Ref> & notes,
                   const Glib::ustring & old_title,
                   Note & renamed_note,
                   IGnote & g);
  Map get_notes() const;
  NoteRenameBehavior get_selected_behavior() const;

  sigc::signal<void(Response)> signal_response;
private:
  bool on_close();
  void on_rename_clicked();
  void on_dont_rename_clicked();
  void on_advanced_expander_changed(bool expanded);
  void on_always_rename_clicked();
  void on_always_show_dlg_clicked();
  void on_never_rename_clicked();
  void on_notes_view_row_activated(guint pos, const Glib::ustring & old_title);
  void on_select_all_button_clicked(bool select);

  IGnote & m_gnote;
  NoteManagerBase & m_manager;
  Response m_response{Response::CANCEL};
  Glib::RefPtr<Gio::ListStore<NoteRenameRecord>> m_notes_model;
  Gtk::Button m_dont_rename_button;
  Gtk::Button m_rename_button;
  Gtk::Button m_select_all_button;
  Gtk::Button m_select_none_button;
  Gtk::CheckButton m_always_show_dlg_radio;
  Gtk::CheckButton m_always_rename_radio;
  Gtk::CheckButton m_never_rename_radio;
  Gtk::Grid m_notes_box;
};

}

#endif
