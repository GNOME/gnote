/*
 * gnote
 *
 * Copyright (C) 2010-2013 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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



#ifndef __NOTE_RECENT_CHANGES_HPP_
#define __NOTE_RECENT_CHANGES_HPP_

#include <string>

#include <gtkmm/alignment.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/grid.h>

#include "note.hpp"
#include "searchnoteswidget.hpp"
#include "utils.hpp"

namespace gnote {
  class NoteManager;

class NoteRecentChanges
  : public MainWindow
{
public:
  NoteRecentChanges(NoteManager& m);
  virtual ~NoteRecentChanges();
  virtual void show_search_bar();
  virtual void set_search_text(const std::string & value);
  virtual void present_note(const Note::Ptr & note);
  virtual void new_note();
  virtual void present_search();

  virtual void embed_widget(EmbeddableWidget &);
  virtual void unembed_widget(EmbeddableWidget &);
  virtual void foreground_embedded(EmbeddableWidget &);
  virtual void background_embedded(EmbeddableWidget &);
  virtual bool running()
    {
      return m_mapped;
    }
protected:
  virtual void on_show();
  virtual bool on_map_event(GdkEventAny *evt);
private:
  void on_open_note(const Note::Ptr &);
  void on_open_note_new_window(const Note::Ptr &);
  void on_delete_note();
  void on_close_window();
  bool on_delete(GdkEventAny *);
  bool on_key_pressed(GdkEventKey *);
  bool is_foreground(EmbeddableWidget &);
  EmbeddableWidget *currently_embedded();
  Gtk::Toolbar *make_toolbar();
  void make_search_box();
  void on_embedded_name_changed(const std::string & name);
  void on_entry_changed();
  void on_entry_activated();
  void entry_changed_timeout();
  std::string get_search_text();
  void update_toolbar(EmbeddableWidget & widget);
  void on_show_window_menu();
  void on_search_button_toggled();
  void on_find_next_button_clicked();
  void on_find_prev_button_clicked();
  Gtk::Menu *make_window_menu(Gtk::Button *button, const std::vector<Gtk::MenuItem*> & items);
  std::vector<Gtk::MenuItem*> & make_menu_items(std::vector<Gtk::MenuItem*> & items,
                                                const std::vector<Glib::RefPtr<Gtk::Action> > & actions);
  void on_main_window_actions_changed(Gtk::Menu **menu);

  NoteManager        &m_note_manager;
  SearchNotesWidget   m_search_notes_widget;
  Gtk::Grid           m_content_vbox;
  Gtk::Alignment      m_search_box;
  Gtk::Grid           m_find_next_prev_box;
  Gtk::ToggleButton   m_search_button;
  Gtk::Alignment      m_embedded_toolbar;
  Gtk::Grid           m_embed_box;
  Gtk::Button        *m_all_notes_button;
  Gtk::Button        *m_new_note_button;
  Gtk::Button        *m_window_actions_button;
  Gtk::SearchEntry    m_search_entry;
  std::list<EmbeddableWidget*> m_embedded_widgets;
  bool                m_mapped;
  sigc::connection    m_current_embedded_name_slot;
  sigc::connection    m_current_embedded_actions_slot;
  utils::InterruptableTimeout *m_entry_changed_timeout;
  Gtk::Menu          *m_window_menu_embedded;
  Gtk::Menu          *m_window_menu_default;
  utils::GlobalKeybinder m_keybinder;
};


}

#endif
