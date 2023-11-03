/*
 * gnote
 *
 * Copyright (C) 2010-2015,2017,2019-2023 Aurimas Cernius
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

#include <set>

#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/grid.h>
#include <gtkmm/liststore.h>
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
      return *m_tree;
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
  void notebook_pixbuf_cell_data_func(Gtk::CellRenderer *, const Gtk::TreeIter<Gtk::TreeConstRow> &);
  void notebook_text_cell_data_func(Gtk::CellRenderer *, const Gtk::TreeIter<Gtk::TreeConstRow> &);
  void on_notebook_row_edited(const Glib::ustring& path, const Glib::ustring& new_text);
  void on_notebook_selection_changed(const notebooks::Notebook::Ptr & notebook);
  void on_notebooks_view_right_click(int n_press, double x, double y);
  bool on_notebooks_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
  void update_results();
  void popup_context_menu_at_location(Gtk::Popover*, Gtk::TreeView*);
  Note::List get_selected_notes();
  bool filter_notes(const Gtk::TreeIter<Gtk::TreeConstRow> &);
  int compare_titles(const Gtk::TreeIter<Gtk::TreeConstRow> &, const Gtk::TreeIter<Gtk::TreeConstRow> &);
  int compare_dates(const Gtk::TreeIter<Gtk::TreeConstRow> &, const Gtk::TreeIter<Gtk::TreeConstRow> &);
  void make_recent_tree();
  void select_notes(const Note::List &);
  Note::Ptr get_note(const Gtk::TreePath & p);
  bool filter_by_search(const Note &);
  bool filter_by_tag(const Note &, const Tag::Ptr &);
  void on_row_activated(const Gtk::TreePath &, Gtk::TreeViewColumn*);
  void on_selection_changed();
  void on_treeview_right_button_pressed(int n_press, double x, double y);
  bool on_treeview_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
  Glib::RefPtr<Gdk::ContentProvider> on_treeview_drag_data_get(double, double);
  void remove_matches_column();
  void no_matches_found_action();
  void add_matches_column();
  bool show_all_search_results();
  void matches_column_data_func(Gtk::CellRenderer *, const Gtk::TreeIter<Gtk::TreeConstRow> &);
  int compare_search_hits(const Gtk::TreeIter<Gtk::TreeConstRow> & , const Gtk::TreeIter<Gtk::TreeConstRow> &);
  void on_note_deleted(NoteBase & note);
  void on_note_added(NoteBase & note);
  void on_note_renamed(const NoteBase&, const Glib::ustring&);
  void on_note_saved(NoteBase&);
  void delete_note(const NoteBase & note);
  void add_note(NoteBase & note);
  void rename_note(const NoteBase & note);
  void on_open_note();
  void on_open_note_new_window();
  Gtk::Window *get_owning_window();
  void on_note_added_to_notebook(const Note & note, const notebooks::Notebook::Ptr & notebook);
  void on_note_removed_from_notebook(const Note & note, const notebooks::Notebook::Ptr & notebook);
  void on_note_pin_status_changed(const Note &, bool);
  Gtk::Popover *get_note_list_context_menu();
  Gtk::Popover *get_notebook_list_context_menu();
  void on_open_notebook_template_note(const Glib::VariantBase&);
  void on_new_notebook(const Glib::VariantBase&);
  void on_delete_notebook(const Glib::VariantBase&);
  void on_sorting_changed();
  void parse_sorting_setting(const Glib::ustring & sorting);
  void on_rename_notebook();

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
  Gtk::ScrolledWindow m_matches_window;
  std::shared_ptr<Gtk::Grid> m_no_matches_box;
  notebooks::NotebooksView *m_notebooks_view;
  Glib::RefPtr<Gtk::ListStore> m_store;
  Glib::RefPtr<Gtk::TreeModelSort> m_store_sort;
  Glib::RefPtr<Gtk::TreeModelFilter> m_store_filter;
  RecentNotesColumnTypes m_column_types;
  IGnote & m_gnote;
  NoteManagerBase & m_manager;
  Gtk::TreeView *m_tree;
  std::map<Glib::ustring, int> m_current_matches;
  int m_clickX, m_clickY;
  Gtk::TreeViewColumn *m_matches_column;
  std::shared_ptr<Gtk::Popover> m_note_list_context_menu;
  std::shared_ptr<Gtk::Popover> m_notebook_list_context_menu;
  bool m_initial_position_restored;
  Glib::ustring m_search_text;
  int m_sort_column_id;
  Gtk::SortType m_sort_column_order;
};

}

#endif
