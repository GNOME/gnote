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


#ifndef _SEARCHNOTESWIDGET_
#define _SEARCHNOTESWIDGET_

#include <set>

#include <gtkmm/grid.h>
#include <gtkmm/liststore.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <sigc++/sigc++.h>

#include "mainwindowembeds.hpp"
#include "notebooks/notebook.hpp"
#include "notebooks/notebookstreeview.hpp"


namespace gnote {

class SearchNotesWidget
  : public Gtk::HPaned
  , public EmbeddableWidget
  , public SearchableItem
  , public HasActions
{
public:
  SearchNotesWidget(NoteManager & m);
  virtual ~SearchNotesWidget();
  virtual std::string get_name() const;
  virtual void foreground();
  virtual void background();
  virtual void hint_position(int & x, int & y);
  virtual void hint_size(int & width, int & height);
  virtual void size_internals();
  virtual void perform_search(const std::string & search_text);
  virtual std::vector<Glib::RefPtr<Gtk::Action> > get_widget_actions();
  virtual sigc::signal<void> & signal_actions_changed();

  void select_all_notes_notebook();
  void new_note();
  void delete_selected_notes();

  sigc::signal<void, const Note::Ptr &> signal_open_note;
  sigc::signal<void, const Note::Ptr &> signal_open_note_new_window;
private:
  void make_actions();
  void perform_search();
  void restore_matches_window();
  Gtk::Widget *make_notebooks_pane();
  void save_position();
  void notebook_pixbuf_cell_data_func(Gtk::CellRenderer *, const Gtk::TreeIter &);
  void notebook_text_cell_data_func(Gtk::CellRenderer *, const Gtk::TreeIter &);
  void on_notebook_row_edited(const Glib::ustring& path, const Glib::ustring& new_text);
  void on_notebook_row_activated(const Gtk::TreePath &, Gtk::TreeViewColumn*);
  void on_notebook_selection_changed();
  bool on_notebooks_tree_button_pressed(GdkEventButton *);
  bool on_notebooks_key_pressed(GdkEventKey *);
  notebooks::Notebook::Ptr get_selected_notebook() const;
  void update_results();
  void popup_context_menu_at_location(Gtk::Menu *, int, int);
  Note::List get_selected_notes();
  bool filter_notes(const Gtk::TreeIter &);
  int compare_titles(const Gtk::TreeIter &, const Gtk::TreeIter &);
  int compare_dates(const Gtk::TreeIter &, const Gtk::TreeIter &);
  void make_recent_tree();
  void select_notes(const Note::List &);
  void position_context_menu(int & x, int & y, bool & push_in);
  Note::Ptr get_note(const Gtk::TreePath & p);
  bool filter_by_search(const Note::Ptr &);
  bool filter_by_tag(const Note::Ptr &);
  void on_row_activated(const Gtk::TreePath &, Gtk::TreeViewColumn*);
  void on_selection_changed();
  bool on_treeview_button_pressed(GdkEventButton *);
  bool on_treeview_motion_notify(GdkEventMotion *);
  bool on_treeview_button_released(GdkEventButton *);
  bool on_treeview_key_pressed(GdkEventKey *);
  void on_treeview_drag_data_get(const Glib::RefPtr<Gdk::DragContext> &,
                                 Gtk::SelectionData &, guint, guint);
  void remove_matches_column();
  void no_matches_found_action();
  void add_matches_column();
  bool show_all_search_results();
  void matches_column_data_func(Gtk::CellRenderer *, const Gtk::TreeIter &);
  int compare_search_hits(const Gtk::TreeIter & , const Gtk::TreeIter &);
  void on_note_deleted(const Note::Ptr & note);
  void on_note_added(const Note::Ptr & note);
  void on_note_renamed(const Note::Ptr&, const std::string&);
  void on_note_saved(const Note::Ptr&);
  void delete_note(const Note::Ptr & note);
  void add_note(const Note::Ptr & note);
  void rename_note(const Note::Ptr & note);
  void on_open_note();
  void on_open_note_new_window();
  Gtk::Window *get_owning_window();
  void on_note_added_to_notebook(const Note & note, const notebooks::Notebook::Ptr & notebook);
  void on_note_removed_from_notebook(const Note & note, const notebooks::Notebook::Ptr & notebook);
  void on_note_pin_status_changed(const Note &, bool);
  Gtk::Menu *get_note_list_context_menu();
  Gtk::Menu *get_notebook_list_context_menu();
  void on_open_notebook_template_note();
  void on_new_notebook();
  void on_delete_notebook();

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
    RecentNotesColumnTypes()
      {
        add(icon); add(title); add(change_date); add(note);
      }

    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
    Gtk::TreeModelColumn<std::string> title;
    Gtk::TreeModelColumn<std::string> change_date;
    Gtk::TreeModelColumn<Note::Ptr> note;
  };

  Glib::RefPtr<Gtk::AccelGroup> m_accel_group;
  Glib::RefPtr<Gtk::Action> m_open_note_action;
  Glib::RefPtr<Gtk::Action> m_open_note_new_window_action;
  Glib::RefPtr<Gtk::Action> m_delete_note_action;
  Glib::RefPtr<Gtk::Action> m_delete_notebook_action;
  RecentSearchColumnTypes m_find_combo_columns;
  Gtk::ScrolledWindow m_matches_window;
  Gtk::Grid *m_no_matches_box;
  notebooks::NotebooksTreeView *m_notebooksTree;
  sigc::connection m_on_notebook_selection_changed_cid;
  std::set<Tag::Ptr>  m_selected_tags;
  Glib::RefPtr<Gtk::ListStore> m_store;
  Glib::RefPtr<Gtk::TreeModelSort> m_store_sort;
  Glib::RefPtr<Gtk::TreeModelFilter> m_store_filter;
  RecentNotesColumnTypes m_column_types;
  NoteManager & m_manager;
  Gtk::TreeView *m_tree;
  std::vector<Gtk::TargetEntry> m_targets;
  std::map<std::string, int> m_current_matches;
  int m_clickX, m_clickY;
  Gtk::TreeViewColumn *m_matches_column;
  Gtk::Menu *m_note_list_context_menu;
  Gtk::Menu *m_notebook_list_context_menu;
  bool m_initial_position_restored;
  std::string m_search_text;

  static Glib::RefPtr<Gdk::Pixbuf> get_note_icon();
};

}

#endif
