/*
 * gnote
 *
 * Copyright (C) 2010-2015,2017,2019-2025 Aurimas Cernius
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


#ifndef _SEARCHNOTESWIDGET_
#define _SEARCHNOTESWIDGET_

#include <gtkmm/columnview.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/grid.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <sigc++/sigc++.h>

#include "mainwindowembeds.hpp"
#include "notebooks/notebook.hpp"
#include "notebooks/notebooksview.hpp"


namespace gnote {

class SearchNotesWidget
  : public Gtk::Paned
  , public EmbeddableWidget
  , public SearchableItem
{
public:
  SearchNotesWidget(IGnote & g, NoteManagerBase & m);
  virtual Glib::ustring get_name() const override;
  void embed(EmbeddableWidgetHost *h) override;
  virtual void background() override;
  virtual void size_internals() override;
  virtual void set_initial_focus() override;
  virtual void perform_search(const Glib::ustring & search_text) override;

  void select_all_notes_notebook();
  void new_note();
  void delete_selected_notes();
  Gtk::Widget & notes_widget() const
    {
      return *m_notes_view;
    }
  const Glib::RefPtr<Gtk::EventControllerKey> & notes_widget_key_ctrl() const
    {
      return m_notes_widget_key_ctrl;
    }

  sigc::signal<void(Note&)> signal_open_note;
  sigc::signal<void(Note&)> signal_open_note_new_window;
private:
  void perform_search();
  void restore_matches_window();
  Gtk::Widget *make_notebooks_pane();
  void save_position();
  void on_notebook_selection_changed(const notebooks::Notebook & notebook);
  void on_notebook_list_changed();
  void update_results();
  unsigned selected_note_count() const;
  std::vector<Note::Ref> get_selected_notes();
  void make_recent_notes_view();
  void select_notes(const std::vector<Note::Ref> &);
  bool filter_by_search(const Note &);
  void on_row_activated(guint idx);
  void on_selection_changed(guint, guint);
  bool on_treeview_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
  Glib::RefPtr<Gdk::ContentProvider> on_treeview_drag_data_get(double, double);
  void remove_matches_column();
  void no_matches_found_action();
  void add_matches_column();
  bool show_all_search_results();
  void on_note_deleted(NoteBase & note);
  void on_note_added(NoteBase & note);
  void on_note_renamed(const NoteBase&, const Glib::ustring&);
  void delete_note(NoteBase & note);
  void add_note(NoteBase & note);

  enum class OpenNoteMode
  {
    CURRENT_WINDOW,
    NEW_WINDOW,
    SINGLE_NEW_WINDOW,
  };
  void on_open_note(OpenNoteMode);
  void on_open_note_new_window();
  Gtk::Window *get_owning_window();
  void on_note_added_to_notebook(const Note & note, const notebooks::Notebook & notebook);
  void on_note_removed_from_notebook(const Note & note, const notebooks::Notebook & notebook);
  void on_note_pin_status_changed(const Note &, bool);
  void on_open_notebook_template_note(Note&);
  void on_sorting_changed(Gtk::Sorter::Change);
  void parse_sorting_setting(const Glib::ustring & sorting);

  class RecentSearchColumnTypes
    : public Gtk::TreeModelColumnRecord
  {
  public:
    RecentSearchColumnTypes()
      {
        add(text);
      }
    Gtk::TreeModelColumn<Glib::ustring> text;
  };
  class RecentNotesColumnTypes
    : public Gtk::TreeModelColumnRecord
  {
  public:
    static constexpr int TITLE = 0;
    static constexpr int CHANGE_DATE = 1;
    static constexpr int NOTE = 2;

    RecentNotesColumnTypes()
      {
        add(title); add(change_date); add(note);
      }

    Gtk::TreeModelColumn<Glib::ustring> title;
    Gtk::TreeModelColumn<Glib::ustring> change_date;
    Gtk::TreeModelColumn<Note::Ptr> note;
  };

  Glib::RefPtr<Gtk::EventControllerKey> m_notes_widget_key_ctrl;
  RecentSearchColumnTypes m_find_combo_columns;
  Gtk::Box m_notes_pane;
  Gtk::Button m_delete_note_button;
  Gtk::ScrolledWindow m_matches_window;
  std::shared_ptr<Gtk::Grid> m_no_matches_box;
  notebooks::NotebooksView *m_notebooks_view;
  Glib::RefPtr<Gio::ListModel> m_store;
  Glib::RefPtr<Gio::ListModel> m_store_sort;
  Glib::RefPtr<Gio::ListModel> m_store_filter;
  RecentNotesColumnTypes m_column_types;
  IGnote & m_gnote;
  NoteManagerBase & m_manager;
  Gtk::ColumnView *m_notes_view;
  std::map<Glib::ustring, int> m_current_matches;
  int m_clickX, m_clickY;
  Glib::RefPtr<Gtk::ColumnViewColumn> m_title_column;
  Glib::RefPtr<Gtk::ColumnViewColumn> m_change_column;
  Glib::RefPtr<Gtk::ColumnViewColumn> m_matches_column;
  bool m_initial_position_restored;
  Glib::ustring m_search_text;
  Glib::RefPtr<const Gtk::ColumnViewColumn> m_sort_column;
  Gtk::SortType m_sort_column_order;
};

}

#endif
