/*
 * gnote
 *
 * Copyright (C) 2010-2017,2019-2025 Aurimas Cernius
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

#include <gtkmm/applicationwindow.h>
#include <gtkmm/grid.h>
#include <gtkmm/notebook.h>
#include <gtkmm/popover.h>
#include <gtkmm/togglebutton.h>

#include "mainwindowaction.hpp"
#include "note.hpp"
#include "searchnoteswidget.hpp"

namespace gnote {
  class IGnote;
  class NoteManagerBase;

class NoteRecentChanges
  : public MainWindow
{
public:
  NoteRecentChanges(IGnote & g, NoteManagerBase & m);
  virtual ~NoteRecentChanges();
  virtual void show_search_bar(bool grab_focus = true) override;
  virtual void set_search_text(Glib::ustring && value) override;
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
  void present_note(Note & note) override;
  virtual void on_show() override;
  virtual void on_map() override;
private:
  void on_open_note_new_window(Note &);
  bool on_close();
  bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
  EmbeddableWidget *currently_foreground();
  void on_current_page_changed(Gtk::Widget *new_page, guint page_number);
  void on_foreground_embedded(EmbeddableWidget & widget);
  void add_shortcut(Gtk::Widget & widget, guint keyval, Gdk::ModifierType modifiers = static_cast<Gdk::ModifierType>(0));
  void add_shortcut(Gtk::ShortcutController & controller, guint keyval, Gdk::ModifierType modifiers = static_cast<Gdk::ModifierType>(0));
  void make_header_bar();
  bool make_search_box();
  void make_find_next_prev(Gtk::Button *&find_next, Gtk::Button *&find_prev);
  void show_find_next_prev();
  void hide_find_next_prev();
  bool get_find_next_prev(Gtk::Button *&find_next, Gtk::Button *&find_prev) const;
  bool on_entry_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
  void on_search_changed();
  void on_search_stopped();
  Glib::ustring get_search_text();
  void update_toolbar(EmbeddableWidget & widget);
  void update_search_bar(EmbeddableWidget & widget, bool perform_search);
  void on_show_window_menu();
  void on_show_embed_action_menu();
  void on_search_button_toggled();
  void on_find_next_button_clicked();
  void on_find_prev_button_clicked();
  Gtk::Popover *make_window_menu(Gtk::Button *button, std::vector<PopoverWidget> && items);
  void on_window_menu_closed();
  bool on_notes_widget_key_press(guint keyval, guint keycode, Gdk::ModifierType state);
  void on_close_window(const Glib::VariantBase&);
  void add_action(const MainWindowAction::Ptr & action);
  bool present_active(Note & note);
  void register_actions();
  void register_shortcuts();
  void callbacks_changed();
  void register_callbacks();
  void unregister_callbacks();
  void next_tab();
  void previous_tab();
  void close_current_tab(const Glib::VariantBase&);

  IGnote             &m_gnote;
  NoteManagerBase    &m_note_manager;
  Preferences        &m_preferences;
  Gtk::Widget        *m_header_bar;
  SearchNotesWidget  *m_search_notes_widget;
  Gtk::Box *m_search_box;
  union
  {
    Gtk::SearchEntry *m_search_entry;
    Glib::ustring    *m_search_text;
  };
  Gtk::ToggleButton   m_search_button;
  Gtk::Grid           m_embedded_toolbar;
  Gtk::Notebook       m_embed_book;
  Gtk::Button        *m_window_actions_button;
  Gtk::Button        *m_current_embed_actions_button;
  bool                m_mapped;
  std::map<Glib::ustring, MainWindowAction::Ptr> m_actions;
  std::vector<sigc::connection> m_action_cids;
};


}

#endif
