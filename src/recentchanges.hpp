/*
 * gnote
 *
 * Copyright (C) 2010-2016 Aurimas Cernius
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
#include <gtkmm/popovermenu.h>

#include "base/macros.hpp"
#include "mainwindowaction.hpp"
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
  virtual void show_search_bar(bool grab_focus = true) override;
  virtual void set_search_text(const std::string & value) override;
  virtual void new_note() override;
  virtual void present_search() override;
  virtual void close_window() override;
  virtual bool is_search() override;

  virtual void embed_widget(EmbeddableWidget &) override;
  virtual void unembed_widget(EmbeddableWidget &) override;
  virtual void foreground_embedded(EmbeddableWidget &) override;
  virtual void background_embedded(EmbeddableWidget &) override;
  virtual bool running()
    {
      return m_mapped;
    }
  virtual bool contains(EmbeddableWidget &) override;
  virtual bool is_foreground(EmbeddableWidget &) override;
  virtual MainWindowAction::Ptr find_action(const Glib::ustring & name) override;
  virtual void enabled(bool is_enabled) override;
protected:
  virtual void present_note(const Note::Ptr & note) override;
  virtual void on_show() override;
  virtual bool on_map_event(GdkEventAny *evt) override;
private:
  void on_open_note(const Note::Ptr &);
  void on_open_note_new_window(const Note::Ptr &);
  void on_delete_note();
  bool on_delete(GdkEventAny *);
  bool on_key_pressed(GdkEventKey *);
  EmbeddableWidget *currently_embedded();
  void make_header_bar();
  void make_search_box();
  bool on_entry_key_pressed(GdkEventKey *);
  void on_entry_changed();
  void on_entry_activated();
  void entry_changed_timeout();
  std::string get_search_text();
  void update_toolbar(EmbeddableWidget & widget);
  void on_all_notes_button_clicked();
  void on_show_window_menu();
  void on_search_button_toggled();
  void on_find_next_button_clicked();
  void on_find_prev_button_clicked();
  Gtk::PopoverMenu *make_window_menu(Gtk::Button *button, const std::vector<Gtk::Widget*> & items);
  void on_embedded_name_changed(const std::string & name);
  void on_settings_changed(const Glib::ustring & key);
  bool on_notes_widget_key_press(GdkEventKey*);
  void on_close_window(const Glib::VariantBase&);
  void add_action(const MainWindowAction::Ptr & action);
  void on_popover_widgets_changed();

  NoteManager        &m_note_manager;
  Gtk::Widget        *m_header_bar;
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
  sigc::connection    m_signal_popover_widgets_changed_cid;
  utils::InterruptableTimeout *m_entry_changed_timeout;
  Gtk::PopoverMenu     *m_window_menu_embedded;
  Gtk::PopoverMenu     *m_window_menu_default;
  utils::GlobalKeybinder m_keybinder;
  bool                m_open_notes_in_new_window;
  bool                m_close_note_on_escape;
  std::map<Glib::ustring, MainWindowAction::Ptr> m_actions;
};


}

#endif
