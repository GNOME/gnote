/*
 * gnote
 *
 * Copyright (C) 2010-2015,2017,2019-2021 Aurimas Cernius
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


#include <glibmm/i18n.h>
#include <gtkmm/alignment.h>
#include <gtkmm/linkbutton.h>
#include <gtkmm/liststore.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/stock.h>

#include "debug.hpp"
#include "iactionmanager.hpp"
#include "iconmanager.hpp"
#include "ignote.hpp"
#include "mainwindow.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "recenttreeview.hpp"
#include "search.hpp"
#include "searchnoteswidget.hpp"
#include "itagmanager.hpp"
#include "notebooks/notebookmanager.hpp"
#include "notebooks/specialnotebooks.hpp"
#include "sharp/string.hpp"


namespace gnote {


Glib::RefPtr<Gdk::Pixbuf> SearchNotesWidget::get_note_icon(IconManager & manager)
{
  return manager.get_icon(IconManager::NOTE, 22);
}


SearchNotesWidget::SearchNotesWidget(IGnote & g, NoteManagerBase & m)
  : m_accel_group(Gtk::AccelGroup::create())
  , m_no_matches_box(NULL)
  , m_gnote(g)
  , m_manager(m)
  , m_clickX(0), m_clickY(0)
  , m_matches_column(NULL)
  , m_note_list_context_menu(NULL)
  , m_notebook_list_context_menu(NULL)
  , m_initial_position_restored(false)
  , m_sort_column_id(2)
  , m_sort_column_order(Gtk::SORT_DESCENDING)
{
  set_hexpand(true);
  set_vexpand(true);
  make_actions();

  // Notebooks Pane
  Gtk::Widget *notebooksPane = Gtk::manage(make_notebooks_pane());
  notebooksPane->show();

  set_position(150);
  add1(*notebooksPane);
  add2(m_matches_window);

  make_recent_tree();
  m_tree->set_enable_search(false);
  m_tree->show();

  update_results();

  m_matches_window.property_hscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
  m_matches_window.property_vscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
  m_matches_window.add(*m_tree);
  m_matches_window.show();

  // Update on changes to notes
  m.signal_note_deleted.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_deleted));
  m.signal_note_added.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_added));
  m.signal_note_renamed.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_renamed));
  m.signal_note_saved.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_saved));

  // Watch when notes are added to notebooks so the search
  // results will be updated immediately instead of waiting
  // until the note's queue_save () kicks in.
  notebooks::NotebookManager & notebook_manager = g.notebook_manager();
  notebook_manager.signal_note_added_to_notebook()
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_added_to_notebook));
  notebook_manager.signal_note_removed_from_notebook()
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_removed_from_notebook));
  notebook_manager.signal_note_pin_status_changed
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_pin_status_changed));

  g.preferences().signal_open_notes_in_new_window_changed.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_settings_changed));
  parse_sorting_setting(g.preferences().search_sorting());
  g.preferences().signal_desktop_gnome_clock_format_changed.connect(sigc::mem_fun(*this, &SearchNotesWidget::update_results));
}

SearchNotesWidget::~SearchNotesWidget()
{
  if(m_note_list_context_menu) {
    delete m_note_list_context_menu;
  }
  if(m_notebook_list_context_menu) {
    delete m_notebook_list_context_menu;
  }
  if(m_no_matches_box) {
    delete m_no_matches_box;
  }
}

Glib::ustring SearchNotesWidget::get_name() const
{
  notebooks::Notebook::Ptr selected_notebook = get_selected_notebook();
  if(!selected_notebook
     || std::dynamic_pointer_cast<notebooks::AllNotesNotebook>(selected_notebook)) {
    return "";
  }
  return selected_notebook->get_name();
}

void SearchNotesWidget::make_actions()
{
  m_open_note_action = Gtk::Action::create("OpenNoteAction", _("_Open"));
  m_open_note_action->signal_activate().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_open_note));

  m_open_note_new_window_action = Gtk::Action::create("OpenNoteNewWindowAction", _("Open In New _Window"));
  m_open_note_new_window_action->signal_activate()
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_open_note_new_window));

  m_delete_note_action = Gtk::Action::create("DeleteNoteAction", _("_Delete"));
  m_delete_note_action->signal_activate().connect(sigc::mem_fun(*this, &SearchNotesWidget::delete_selected_notes));

  m_delete_notebook_action = Gtk::Action::create("DeleteNotebookAction", _("_Delete"));
  m_delete_notebook_action->signal_activate().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_delete_notebook));

  m_rename_notebook_action = Gtk::Action::create("RenameNotebookAction", _("Re_name..."));
  m_rename_notebook_action->signal_activate().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_rename_notebook));
}

void SearchNotesWidget::perform_search(const Glib::ustring & search_text)
{
  restore_matches_window();
  m_search_text = search_text;
  perform_search();
}

void SearchNotesWidget::perform_search()
{
  // For some reason, the matches column must be rebuilt
  // every time because otherwise, it's not sortable.
  remove_matches_column();
  Search search(m_manager);

  Glib::ustring text = m_search_text;
  if(text.empty()) {
    m_current_matches.clear();
    m_store_filter->refilter();
    if(m_tree->get_realized()) {
      m_tree->scroll_to_point (0, 0);
    }
    return;
  }
  text = text.lowercase();

  m_current_matches.clear();

  // Search using the currently selected notebook
  notebooks::Notebook::Ptr selected_notebook = get_selected_notebook();
  if(std::dynamic_pointer_cast<notebooks::SpecialNotebook>(selected_notebook)) {
    selected_notebook = notebooks::Notebook::Ptr();
  }

  Search::ResultsPtr results = search.search_notes(text, false, selected_notebook);
  // if no results found in current notebook ask user whether
  // to search in all notebooks
  if(results->size() == 0 && selected_notebook != NULL) {
    no_matches_found_action();
  }
  else {
    for(Search::Results::const_reverse_iterator iter = results->rbegin();
        iter != results->rend(); iter++) {
      m_current_matches[iter->second->uri()] = iter->first;
    }

    add_matches_column();
    m_store_filter->refilter();
    if(m_tree->get_realized()) {
      m_tree->scroll_to_point(0, 0);
    }
  }
}

void SearchNotesWidget::restore_matches_window()
{
  if(m_no_matches_box && get_child2() == m_no_matches_box) {
    remove(*m_no_matches_box);
    add2(m_matches_window);
  }
}

Gtk::Widget *SearchNotesWidget::make_notebooks_pane()
{
  m_notebooksTree = Gtk::manage(new notebooks::NotebooksTreeView(m_manager, m_gnote.notebook_manager().get_notebooks_with_special_items()));

  m_notebooksTree->get_selection()->set_mode(Gtk::SELECTION_SINGLE);
  m_notebooksTree->set_headers_visible(true);
  m_notebooksTree->set_rules_hint(false);

  Gtk::CellRenderer *renderer;

  Gtk::TreeViewColumn *column = manage(new Gtk::TreeViewColumn());
  column->set_title(_("Notebooks"));
  column->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
  column->set_resizable(false);

  renderer = manage(new Gtk::CellRendererPixbuf());
  column->pack_start(*renderer, false);
  column->set_cell_data_func(*renderer,
                             sigc::mem_fun(*this, &SearchNotesWidget::notebook_pixbuf_cell_data_func));

  Gtk::CellRendererText *text_renderer = manage(new Gtk::CellRendererText());
  text_renderer->property_editable() = true;
  column->pack_start(*text_renderer, true);
  column->set_cell_data_func(*text_renderer,
                             sigc::mem_fun(*this, &SearchNotesWidget::notebook_text_cell_data_func));
  text_renderer->signal_edited().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_notebook_row_edited));

  m_notebooksTree->append_column(*column);

  m_notebooksTree->signal_row_activated()
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_notebook_row_activated));
  m_on_notebook_selection_changed_cid = m_notebooksTree->get_selection()->signal_changed()
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_notebook_selection_changed));
  m_notebooksTree->signal_button_press_event()
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_notebooks_tree_button_pressed), false);
  m_notebooksTree->signal_key_press_event()
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_notebooks_key_pressed));

  m_notebooksTree->show();
  Gtk::ScrolledWindow *sw = new Gtk::ScrolledWindow();
  sw->property_hscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
  sw->property_vscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
  sw->add(*m_notebooksTree);
  sw->show();

  return sw;
}

void SearchNotesWidget::save_position()
{
  int width;
  int height;

  EmbeddableWidgetHost *current_host = host();
  if(!current_host || !current_host->running()) {
    return;
  }

  m_gnote.preferences().search_window_splitter_pos(get_position());

  Gtk::Window *window = dynamic_cast<Gtk::Window*>(current_host);
  if(!window || (window->get_window()->get_state() & Gdk::WINDOW_STATE_MAXIMIZED) != 0) {
    return;
  }

  window->get_size(width, height);

  m_gnote.preferences().search_window_width(width);
  m_gnote.preferences().search_window_height(height);
}

void SearchNotesWidget::notebook_pixbuf_cell_data_func(Gtk::CellRenderer * renderer,
                                                       const Gtk::TreeIter & iter)
{
  notebooks::Notebook::Ptr notebook;
  iter->get_value(0, notebook);
  if(!notebook) {
    return;
  }

  Gtk::CellRendererPixbuf *crp = dynamic_cast<Gtk::CellRendererPixbuf*>(renderer);
  notebooks::SpecialNotebook::Ptr special_nb = std::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook);
  if(special_nb) {
    crp->property_pixbuf() = special_nb->get_icon(m_gnote.icon_manager());
  }
  else {
    crp->property_pixbuf() = m_gnote.icon_manager().get_icon(IconManager::NOTEBOOK, 22);
  }
}

void SearchNotesWidget::notebook_text_cell_data_func(Gtk::CellRenderer * renderer,
                                                     const Gtk::TreeIter & iter)
{
  Gtk::CellRendererText *crt = dynamic_cast<Gtk::CellRendererText*>(renderer);
  crt->property_ellipsize() = Pango::ELLIPSIZE_END;
  notebooks::Notebook::Ptr notebook;
  iter->get_value(0, notebook);
  if(!notebook) {
    crt->property_text() = "";
    return;
  }

  crt->property_text() = notebook->get_name();

  if(std::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook)) {
    // Bold the "Special" Notebooks
    crt->property_markup() = Glib::ustring::compose("<span weight=\"bold\">%1</span>",
                                 notebook->get_name());
  }
  else {
    crt->property_text() = notebook->get_name();
  }
}

void SearchNotesWidget::on_notebook_row_edited(const Glib::ustring& /*tree_path*/,
                                               const Glib::ustring& new_text)
{
  notebooks::NotebookManager & notebook_manager = m_gnote.notebook_manager();
  if(notebook_manager.notebook_exists(new_text) || new_text == "") {
    return;
  }
  notebooks::Notebook::Ptr old_notebook = this->get_selected_notebook();
  if(std::dynamic_pointer_cast<notebooks::SpecialNotebook>(old_notebook)) {
    return;
  }
  notebooks::Notebook::Ptr new_notebook = notebook_manager.get_or_create_notebook(new_text);
  DBG_OUT("Renaming notebook '{%s}' to '{%s}'", old_notebook->get_name().c_str(),
          new_text.c_str());
  auto notes = old_notebook->get_tag()->get_notes();
  for(NoteBase *note : notes) {
    notebook_manager.move_note_to_notebook(
      std::static_pointer_cast<Note>(note->shared_from_this()), new_notebook);
  }
  notebook_manager.delete_notebook(old_notebook);
  Gtk::TreeIter iter;
  if(notebook_manager.get_notebook_iter(new_notebook, iter)) {
    m_notebooksTree->get_selection()->select(iter);
    m_notebooksTree->set_cursor(m_notebooksTree->get_model()->get_path(iter));
  }
}

// Create a new note in the notebook when activated
void SearchNotesWidget::on_notebook_row_activated(const Gtk::TreePath &,
                                                  Gtk::TreeViewColumn*)
{
  new_note();
}

void SearchNotesWidget::on_notebook_selection_changed()
{
  restore_matches_window();
  notebooks::Notebook::Ptr notebook = get_selected_notebook();
  if(!notebook) {
    // Clear out the currently selected tags so that no notebook is selected
    m_selected_tags.clear();


    // Select the "All Notes" item without causing
    // this handler to be called again
    m_on_notebook_selection_changed_cid.block();
    select_all_notes_notebook();
    m_delete_notebook_action->set_sensitive(false);
    m_rename_notebook_action->set_sensitive(false);
    m_on_notebook_selection_changed_cid.unblock();
  }
  else {
    m_selected_tags.clear();
    if(notebook->get_tag()) {
      m_selected_tags.insert(notebook->get_tag());
    }
    bool allow_edit = false;
    if(std::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook)) {
      m_delete_notebook_action->set_sensitive(false);
      m_rename_notebook_action->set_sensitive(false);
    }
    else {
      m_delete_notebook_action->set_sensitive(true);
      m_rename_notebook_action->set_sensitive(true);
      allow_edit = true;
    }

    std::vector<Gtk::CellRenderer*> renderers = m_notebooksTree->get_column(0)->get_cells();
    for(std::vector<Gtk::CellRenderer*>::iterator renderer = renderers.begin();
        renderer != renderers.end(); ++renderer) {
      Gtk::CellRendererText *text_rederer = dynamic_cast<Gtk::CellRendererText*>(*renderer);
      if(text_rederer) {
        text_rederer->property_editable() = allow_edit;
      }
    }
  }

  update_results();
  signal_name_changed(get_name());
}

bool SearchNotesWidget::on_notebooks_tree_button_pressed(GdkEventButton *ev)
{
  auto event = (GdkEvent*)ev;
  guint button;
  gdk_event_get_button(event, &button);
  if(button == 3) {
    // third mouse button (right-click)
    Gtk::TreeViewColumn * col = 0; // unused
    Gtk::TreePath p;
    int cell_x, cell_y;            // unused
    const Glib::RefPtr<Gtk::TreeSelection> selection
      = m_notebooksTree->get_selection();

    gdouble x, y;
    gdk_event_get_coords(event, &x, &y);
    if(m_notebooksTree->get_path_at_pos(x, y, p, col, cell_x, cell_y)) {
      selection->select(p);
    }

    Gtk::Menu *menu = get_notebook_list_context_menu();
    popup_context_menu_at_location(menu, event);
    return true;
  }
  return false;
}

bool SearchNotesWidget::on_notebooks_key_pressed(GdkEventKey *ev)
{
  auto event = (GdkEvent*)ev;
  guint keyval;
  gdk_event_get_keyval(event, &keyval);
  switch(keyval) {
  case GDK_KEY_F2:
    on_rename_notebook();
    break;
  case GDK_KEY_Menu:
  {
    Gtk::Menu *menu = get_notebook_list_context_menu();
    popup_context_menu_at_location(menu, event);
    break;
  }
  default:
    break;
  }

  return false; // Let Escape be handled by the window.
}

notebooks::Notebook::Ptr SearchNotesWidget::get_selected_notebook() const
{
  Gtk::TreeIter iter;

  Glib::RefPtr<Gtk::TreeSelection> selection = m_notebooksTree->get_selection();
  if(!selection) {
    return notebooks::Notebook::Ptr();
  }
  iter = selection->get_selected();
  if(!iter) {
    return notebooks::Notebook::Ptr(); // Nothing selected
  }

  notebooks::Notebook::Ptr notebook;
  iter->get_value(0, notebook);
  return notebook;
}

void SearchNotesWidget::select_all_notes_notebook()
{
  Glib::RefPtr<Gtk::TreeModel> model = m_notebooksTree->get_model();
  DBG_ASSERT(model, "model is NULL");
  if(!model) {
    return;
  }
  for(Gtk::TreeIter iter = model->children().begin(); iter; ++iter) {
    notebooks::Notebook::Ptr notebook;
    iter->get_value(0, notebook);
    if(std::dynamic_pointer_cast<notebooks::AllNotesNotebook>(notebook) != NULL) {
      m_notebooksTree->get_selection()->select(iter);
      break;
    }
  }
}

void SearchNotesWidget::update_results()
{
  // Save the currently selected notes
  Note::List selected_notes = get_selected_notes();

  int sort_column = 2; /* change date */
  Gtk::SortType sort_type = Gtk::SORT_DESCENDING;
  if(m_store_sort) {
    m_store_sort->get_sort_column_id(sort_column, sort_type);
  }

  m_store = Gtk::ListStore::create(m_column_types);

  m_store_filter = Gtk::TreeModelFilter::create(m_store);
  m_store_filter->set_visible_func(sigc::mem_fun(*this, &SearchNotesWidget::filter_notes));
  m_store_sort = Gtk::TreeModelSort::create(m_store_filter);
  m_store_sort->set_sort_func(1 /* title */,
                              sigc::mem_fun(*this, &SearchNotesWidget::compare_titles));
  m_store_sort->set_sort_func(2 /* change date */,
                              sigc::mem_fun(*this, &SearchNotesWidget::compare_dates));
  m_store_sort->set_sort_column(m_sort_column_id, m_sort_column_order);
  m_store_sort->signal_sort_column_changed()
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_sorting_changed));

  int cnt = 0;

  for(const NoteBase::Ptr & note_iter : m_manager.get_notes()) {
    Note::Ptr note(std::static_pointer_cast<Note>(note_iter));
    Glib::ustring nice_date = utils::get_pretty_print_date(note->change_date(), true, m_gnote.preferences());

    Gtk::TreeIter iter = m_store->append();
    iter->set_value(0, get_note_icon(m_gnote.icon_manager()));  /* icon */
    iter->set_value(1, note->get_title()); /* title */
    iter->set_value(2, nice_date);  /* change date */
    iter->set_value(3, note);      /* note */
    cnt++;
  }

  m_tree->set_model(m_store_sort);

  perform_search();

  if(sort_column >= 0 && m_no_matches_box && get_child2() != m_no_matches_box) {
    // Set the sort column after loading data, since we
    // don't want to resort on every append.
    m_store_sort->set_sort_column(sort_column, sort_type);
  }

  // Restore the previous selection
  if(!selected_notes.empty()) {
    select_notes(selected_notes);
  }
}

void SearchNotesWidget::popup_context_menu_at_location(Gtk::Menu *menu, GdkEvent *trigger_event)
{
  menu->show_all();
  Glib::RefPtr<Gdk::Window> rect_window;
  Gdk::Rectangle cell_rect;
  if(trigger_event->type != GDK_BUTTON_PRESS) {
    Gtk::Window *parent = get_owning_window();
    if(parent) {
      Gtk::Widget *focus_widget = parent->get_focus();
      if(focus_widget) {
        int x, y;
        focus_widget->get_window()->get_origin(x, y);

        Gtk::TreeView *tree = dynamic_cast<Gtk::TreeView*>(focus_widget);
        if(tree) {
          rect_window = tree->get_bin_window();
          if(rect_window) {
            rect_window->get_origin(x, y);
          }

          const Glib::RefPtr<Gtk::TreeSelection> selection = tree->get_selection();
          const std::vector<Gtk::TreePath> selected_rows = selection->get_selected_rows();
          if(selected_rows.empty()) {
            rect_window.reset();
          }
          else {
            const Gtk::TreePath & dest_path = selected_rows.front();
            const std::vector<Gtk::TreeViewColumn*> columns = tree->get_columns();
            tree->get_cell_area(dest_path, *columns.front(), cell_rect);
          }
        }
      }
    }
  }

  if(rect_window) {
    menu->popup_at_rect(rect_window, cell_rect, Gdk::GRAVITY_NORTH_WEST, Gdk::GRAVITY_NORTH_WEST, trigger_event);
  }
  else {
    menu->popup_at_pointer(trigger_event);
  }
}

Note::List SearchNotesWidget::get_selected_notes()
{
  Note::List selected_notes;

  std::vector<Gtk::TreePath> selected_rows =
    m_tree->get_selection()->get_selected_rows();
  for(std::vector<Gtk::TreePath>::const_iterator iter = selected_rows.begin();
      iter != selected_rows.end(); ++iter) {
    Note::Ptr note = get_note(*iter);
    if(!note) {
      continue;
    }

    selected_notes.push_back(note);
  }

  return selected_notes;
}

bool SearchNotesWidget::filter_notes(const Gtk::TreeIter & iter)
{
  Note::Ptr note = (*iter)[m_column_types.note];
  if(!note) {
    return false;
  }

  notebooks::Notebook::Ptr selected_notebook = get_selected_notebook();
  if(!selected_notebook || !selected_notebook->contains_note(note)) {
    return false;
  }

  bool passes_search_filter = filter_by_search(note);
  if(passes_search_filter == false) {
    return false; // don't waste time checking tags if it's already false
  }

  bool passes_tag_filter = filter_by_tag(note);

  // Must pass both filters to appear in the list
  return passes_tag_filter && passes_search_filter;
}

int SearchNotesWidget::compare_titles(const Gtk::TreeIter & a, const Gtk::TreeIter & b)
{
  Glib::ustring title_a = (*a)[m_column_types.title];
  Glib::ustring title_b = (*b)[m_column_types.title];

  if(title_a.empty() || title_b.empty()) {
    return -1;
  }

  return title_a.lowercase().compare(title_b.lowercase());
}

int SearchNotesWidget::compare_dates(const Gtk::TreeIter & a, const Gtk::TreeIter & b)
{
  Note::Ptr note_a = (*a)[m_column_types.note];
  Note::Ptr note_b = (*b)[m_column_types.note];

  if(!note_a || !note_b) {
    return -1;
  }
  else {
    return note_a->change_date().compare(note_b->change_date());
  }
}

void SearchNotesWidget::make_recent_tree()
{
  m_targets.push_back(Gtk::TargetEntry("STRING", Gtk::TARGET_SAME_APP, 0));
  m_targets.push_back(Gtk::TargetEntry("text/plain", Gtk::TARGET_SAME_APP, 0));
  m_targets.push_back(Gtk::TargetEntry("text/uri-list", Gtk::TARGET_SAME_APP, 1));

  m_tree = manage(new RecentTreeView());
  m_tree->set_headers_visible(true);
  m_tree->set_rules_hint(true);
  m_tree->signal_row_activated().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_row_activated));
  m_tree->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
  m_tree->get_selection()->signal_changed().connect(
    sigc::mem_fun(*this, &SearchNotesWidget::on_selection_changed));
  m_tree->signal_button_press_event().connect(
    sigc::mem_fun(*this, &SearchNotesWidget::on_treeview_button_pressed), false);
  m_tree->signal_motion_notify_event().connect(
    sigc::mem_fun(*this, &SearchNotesWidget::on_treeview_motion_notify), false);
  m_tree->signal_button_release_event().connect(
    sigc::mem_fun(*this, &SearchNotesWidget::on_treeview_button_released));
  m_tree->signal_key_press_event().connect(
    sigc::mem_fun(*this, &SearchNotesWidget::on_treeview_key_pressed), false);
  m_tree->signal_drag_data_get().connect(
    sigc::mem_fun(*this, &SearchNotesWidget::on_treeview_drag_data_get));

  m_tree->enable_model_drag_source(m_targets,
    Gdk::BUTTON1_MASK | Gdk::BUTTON3_MASK, Gdk::ACTION_MOVE);

  Gtk::CellRenderer *renderer;

  Gtk::TreeViewColumn *title = manage(new Gtk::TreeViewColumn());
  title->set_title(_("Note"));
  title->set_min_width(150);
  title->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
  title->set_expand(true);
  title->set_resizable(true);

  renderer = manage(new Gtk::CellRendererPixbuf());
  title->pack_start(*renderer, false);
  title->add_attribute(*renderer, "pixbuf", 0 /* icon */);

  renderer = manage(new Gtk::CellRendererText());
  static_cast<Gtk::CellRendererText*>(renderer)->property_ellipsize() = Pango::ELLIPSIZE_END;
  title->pack_start(*renderer, true);
  title->add_attribute(*renderer, "text", 1 /* title */);
  title->set_sort_column(1); /* title */
  title->set_sort_indicator(false);
  title->set_reorderable(false);
  title->set_sort_order(Gtk::SORT_ASCENDING);

  m_tree->append_column(*title);

  Gtk::TreeViewColumn *change = manage(new Gtk::TreeViewColumn());
  change->set_title(_("Modified"));
  change->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
  change->set_resizable(false);

  renderer = manage(new Gtk::CellRendererText());
  renderer->property_xalign() = 1.0;
  change->pack_start(*renderer, false);
  change->add_attribute(*renderer, "text", 2 /* change date */);
  change->set_sort_column(2); /* change date */
  change->set_sort_indicator(false);
  change->set_reorderable(false);
  change->set_sort_order(Gtk::SORT_DESCENDING);

  m_tree->append_column(*change);
}

void SearchNotesWidget::select_notes(const Note::List & notes)
{
  Gtk::TreeIter iter = m_store_sort->children().begin();

  if(!iter) {
    return;
  }

  do {
    Note::Ptr iter_note = (*iter)[m_column_types.note];
    if(find(notes.begin(), notes.end(), iter_note) != notes.end()) {
      // Found one
      m_tree->get_selection()->select(iter);
    }
  } while(++iter);
}

Note::Ptr SearchNotesWidget::get_note(const Gtk::TreePath & p)
{
  Gtk::TreeIter iter = m_store_sort->get_iter(p);
  if(iter) {
    return (*iter)[m_column_types.note];
  }
  return Note::Ptr();
}

bool SearchNotesWidget::filter_by_search(const Note::Ptr & note)
{
  if(m_search_text.empty()) {
    return true;
  }

  if(m_current_matches.empty()) {
    return false;
  }

  return note && m_current_matches.find(note->uri()) != m_current_matches.end();
}

bool SearchNotesWidget::filter_by_tag(const Note::Ptr & note)
{
  if(m_selected_tags.empty()) {
    return true;
  }

  //   // FIXME: Ugh!  NOT an O(1) operation.  Is there a better way?
  std::vector<Tag::Ptr> tags = note->get_tags();
  for(auto & tag : tags) {
    if(m_selected_tags.find(tag) != m_selected_tags.end()) {
      return true;
    }
  }

  return false;
}

void SearchNotesWidget::on_row_activated(const Gtk::TreePath & p, Gtk::TreeViewColumn*)
{
  Gtk::TreeIter iter = m_store_sort->get_iter(p);
  if(!iter) {
    return;
  }

  Note::Ptr note = (*iter)[m_column_types.note];
  signal_open_note(note);
}

void SearchNotesWidget::on_selection_changed()
{
  Note::List selected_notes = get_selected_notes();

  if(selected_notes.empty()) {
    m_open_note_action->property_sensitive() = false;
    m_open_note_new_window_action->property_sensitive() = false;
    m_delete_note_action->property_sensitive() = false;
  }
  else {
    m_open_note_action->property_sensitive() = true;
    m_open_note_new_window_action->property_sensitive() = true;
    m_delete_note_action->property_sensitive() = true;
  }
}

bool SearchNotesWidget::on_treeview_button_pressed(GdkEventButton *ev)
{
  auto event = (GdkEvent*)ev;
  if(gdk_event_get_window(event) != m_tree->get_bin_window()->gobj()) {
    return false;
  }

  Gtk::TreePath dest_path;
  Gtk::TreeViewColumn *column = NULL;
  int unused;

  gdouble x, y;
  gdk_event_get_coords(event, &x, &y);
  m_tree->get_path_at_pos(x, y, dest_path, column, unused, unused);

  m_clickX = x;
  m_clickY = y;

  bool retval = false;

  switch(event->type) {
  case GDK_2BUTTON_PRESS:
  {
    guint button;
    gdk_event_get_button(event, &button);
    GdkModifierType state;
    gdk_event_get_state(event, &state);
    if(button != 1 || (state & (Gdk::CONTROL_MASK | Gdk::SHIFT_MASK)) != 0) {
      break;
    }

    m_tree->get_selection()->unselect_all();
    m_tree->get_selection()->select(dest_path);
    // apparently Gtk::TreeView::row_activated() require a dest_path
    // while get_path_at_pos() can return a NULL pointer.
    // See Gtkmm bug 586438
    // https://bugzilla.gnome.org/show_bug.cgi?id=586438
    gtk_tree_view_row_activated(m_tree->gobj(), dest_path.gobj(), column?column->gobj():NULL);
    break;
  }
  case GDK_BUTTON_PRESS:
  {
    guint button;
    gdk_event_get_button(event, &button);
    if(button == 3) {
      const Glib::RefPtr<Gtk::TreeSelection> selection = m_tree->get_selection();

      if(selection->get_selected_rows().size() <= 1) {
        Gtk::TreeViewColumn * col = 0; // unused
        Gtk::TreePath p;
        int cell_x, cell_y;            // unused
        if(m_tree->get_path_at_pos(x, y, p, col, cell_x, cell_y)) {
          selection->unselect_all();
          selection->select(p);
        }
      }
      Gtk::Menu *menu = get_note_list_context_menu();
      popup_context_menu_at_location(menu, event);

      // Return true so that the base handler won't
      // run, which causes the selection to change to
      // the row that was right-clicked.
      retval = true;
      break;
    }

    GdkModifierType state;
    gdk_event_get_state(event, &state);
    if(m_tree->get_selection()->is_selected(dest_path) && (state & (Gdk::CONTROL_MASK | Gdk::SHIFT_MASK)) == 0) {
      if(column && (button == 1)) {
        Gtk::CellRenderer *renderer = column->get_first_cell();
        Gdk::Rectangle background_area;
        m_tree->get_background_area(dest_path, *column, background_area);
        Gdk::Rectangle cell_area;
        m_tree->get_cell_area(dest_path, *column, cell_area);

        renderer->activate(event, *m_tree, dest_path.to_string(), background_area, cell_area, Gtk::CELL_RENDERER_SELECTED);

        Gtk::TreeIter iter = m_tree->get_model()->get_iter (dest_path);
        if(iter) {
          m_tree->get_model()->row_changed(dest_path, iter);
        }
      }

      retval = true;
    }

    break;
  }
  default:
    retval = false;
    break;
  }
  return retval;
}

bool SearchNotesWidget::on_treeview_motion_notify(GdkEventMotion *ev)
{
  auto event = (GdkEvent*)ev;
  GdkModifierType state;
  gdk_event_get_state(event, &state);
  if((state & Gdk::BUTTON1_MASK) == 0) {
    return false;
  }
  else if(gdk_event_get_window(event) != m_tree->get_bin_window()->gobj()) {
    return false;
  }

  bool retval = true;
  gdouble x, y;
  gdk_event_get_coords(event, &x, &y);
  if(!m_tree->drag_check_threshold(m_clickX, m_clickY, x, y)) {
    return retval;
  }

  Gtk::TreePath dest_path;
  Gtk::TreeViewColumn * col = NULL; // unused
  int cell_x, cell_y;               // unused
  if(!m_tree->get_path_at_pos(x, y, dest_path, col, cell_x, cell_y)) {
    return retval;
  }

  m_tree->drag_begin(Gtk::TargetList::create (m_targets), Gdk::ACTION_MOVE, 1, event);
  return retval;
}

bool SearchNotesWidget::on_treeview_button_released(GdkEventButton *ev)
{
  gdouble x, y;
  gdk_event_get_coords((GdkEvent*)ev, &x, &y);
  GdkModifierType state;
  gdk_event_get_state((GdkEvent*)ev, &state);
  if(!m_tree->drag_check_threshold(m_clickX, m_clickY, x, y)
     && ((state & (Gdk::CONTROL_MASK | Gdk::SHIFT_MASK)) == 0)
     && m_tree->get_selection()->count_selected_rows () > 1) {
    Gtk::TreePath dest_path;
    Gtk::TreeViewColumn * col = NULL; // unused
    int cell_x, cell_y;               // unused
    m_tree->get_path_at_pos(x, y, dest_path, col, cell_x, cell_y);
    m_tree->get_selection()->unselect_all();
    m_tree->get_selection()->select(dest_path);
  }
  return false;
}

bool SearchNotesWidget::on_treeview_key_pressed(GdkEventKey * ev)
{
  // create context menu here, so that we have shortcuts and they work
  Gtk::Menu *menu = get_note_list_context_menu();
  auto event = (GdkEvent*)ev;
  guint keyval;
  gdk_event_get_keyval(event, &keyval);

  switch(keyval) {
  case GDK_KEY_Delete:
    delete_selected_notes();
    break;
  case GDK_KEY_Menu:
  {
    // Pop up the context menu if a note is selected
    Note::List selected_notes = get_selected_notes();
    if(!selected_notes.empty()) {
      popup_context_menu_at_location(menu, event);
    }
    break;
  }
  case GDK_KEY_Return:
  case GDK_KEY_KP_Enter:
    // Open all selected notes
    on_open_note();
    return true;
  default:
    break;
  }

  return false; // Let Escape be handled by the window.
}

void SearchNotesWidget::on_treeview_drag_data_get(const Glib::RefPtr<Gdk::DragContext> &,
                                                  Gtk::SelectionData &selection_data,
                                                  guint, guint)
{
  Note::List selected_notes = get_selected_notes();
  if(selected_notes.empty()) {
    return;
  }

  std::vector<Glib::ustring> uris;
  for(Note::List::const_iterator iter = selected_notes.begin();
      iter != selected_notes.end(); ++iter) {

    uris.push_back((*iter)->uri());
  }

  selection_data.set_uris(uris);

  if(selected_notes.size() == 1) {
    selection_data.set_text(selected_notes.front()->get_title());
  }
  else {
    selection_data.set_text(_("Notes"));
  }
}

void SearchNotesWidget::remove_matches_column()
{
  if(m_matches_column == NULL || !m_matches_column->get_visible()) {
    return;
  }

  m_matches_column->set_visible(false);
  m_store_sort->set_sort_column(m_sort_column_id, m_sort_column_order);
}

// called when no search results are found in the selected notebook
void SearchNotesWidget::no_matches_found_action()
{
  remove(m_matches_window);
  if(!m_no_matches_box) {
    Glib::ustring message = _("No results found in the selected notebook.\nClick here to search across all notes.");
    Gtk::LinkButton *link_button = manage(new Gtk::LinkButton("", message));
    link_button->signal_activate_link()
      .connect(sigc::mem_fun(*this, &SearchNotesWidget::show_all_search_results), false);
    link_button->set_tooltip_text(_("Click here to search across all notebooks"));
    link_button->show();
    Gtk::Alignment *no_matches_found = manage(new Gtk::Alignment(Gtk::ALIGN_CENTER, Gtk::ALIGN_CENTER, 0.0, 0.0));
    no_matches_found->add(*link_button);

    no_matches_found->set_hexpand(true);
    no_matches_found->set_vexpand(true);
    no_matches_found->show_all();
    m_no_matches_box = new Gtk::Grid;
    m_no_matches_box->attach(*no_matches_found, 0, 0, 1, 1);
    m_no_matches_box->show();
  }
  add2(*m_no_matches_box);
}

void SearchNotesWidget::add_matches_column()
{
  if(!m_matches_column) {
    Gtk::CellRenderer *renderer;

    m_matches_column = manage(new Gtk::TreeViewColumn());
    m_matches_column->set_title(_("Matches"));
    m_matches_column->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
    m_matches_column->set_resizable(false);

    renderer = manage(new Gtk::CellRendererText());
    renderer->property_width() = 75;
    m_matches_column->pack_start(*renderer, false);
    m_matches_column->set_cell_data_func(
      *renderer,
      sigc::mem_fun(*this, &SearchNotesWidget::matches_column_data_func));
    m_matches_column->set_sort_column(4);
    m_matches_column->set_sort_indicator(true);
    m_matches_column->set_reorderable(false);
    m_matches_column->set_sort_order(Gtk::SORT_DESCENDING);
    m_matches_column->set_clickable(true);
    m_store_sort->set_sort_func(4 /* matches */,
                                sigc::mem_fun(*this, &SearchNotesWidget::compare_search_hits));

    m_tree->append_column(*m_matches_column);
    m_store_sort->set_sort_column(4, Gtk::SORT_DESCENDING);
  }
  else {
    m_matches_column->set_visible(true);
    m_store_sort->set_sort_column(4, Gtk::SORT_DESCENDING);
  }
}

bool SearchNotesWidget::show_all_search_results()
{
  select_all_notes_notebook();
  return true;
}

void SearchNotesWidget::matches_column_data_func(Gtk::CellRenderer * cell,
                                                 const Gtk::TreeIter & iter)
{
  Gtk::CellRendererText *crt = dynamic_cast<Gtk::CellRendererText*>(cell);
  if(!crt) {
    return;
  }

  Glib::ustring match_str = "";

  Note::Ptr note = (*iter)[m_column_types.note];
  if(note) {
    int match_count;
    auto miter = m_current_matches.find(note->uri());
    if(miter != m_current_matches.end()) {
      match_count = miter->second;
      if(match_count == INT_MAX) {
        //TRANSLATORS: search found a match in note title
        match_str = _("Title match");
      }
      else if(match_count > 0) {
        const char * fmt;
        fmt = ngettext("%1 match", "%1 matches", match_count);
        match_str = Glib::ustring::compose(fmt, match_count);
      }
    }
  }

  crt->property_text() = match_str;
}

int SearchNotesWidget::compare_search_hits(const Gtk::TreeIter & a, const Gtk::TreeIter & b)
{
  Note::Ptr note_a = (*a)[m_column_types.note];
  Note::Ptr note_b = (*b)[m_column_types.note];

  if(!note_a || !note_b) {
    return -1;
  }

  int matches_a;
  int matches_b;
  auto iter_a = m_current_matches.find(note_a->uri());
  auto iter_b = m_current_matches.find(note_b->uri());
  bool has_matches_a = (iter_a != m_current_matches.end());
  bool has_matches_b = (iter_b != m_current_matches.end());

  if(!has_matches_a || !has_matches_b) {
    if(has_matches_a) {
      return 1;
    }

    return -1;
  }

  matches_a = iter_a->second;
  matches_b = iter_b->second;
  int result = matches_a - matches_b;
  if(result == 0) {
    // Do a secondary sort by note title in alphabetical order
    result = compare_titles(a, b);

    // Make sure to always sort alphabetically
    if(result != 0) {
      int sort_col_id;
      Gtk::SortType sort_type;
      if(m_store_sort->get_sort_column_id(sort_col_id, sort_type)) {
        if(sort_type == Gtk::SORT_DESCENDING) {
          result = -result; // reverse sign
        }
      }
    }

    return result;
  }

  return result;
}

void SearchNotesWidget::on_note_deleted(const NoteBase::Ptr & note)
{
  restore_matches_window();
  delete_note(std::static_pointer_cast<Note>(note));
}

void SearchNotesWidget::on_note_added(const NoteBase::Ptr & note)
{
  restore_matches_window();
  add_note(std::static_pointer_cast<Note>(note));
}

void SearchNotesWidget::on_note_renamed(const NoteBase::Ptr & note,
                                        const Glib::ustring &)
{
  restore_matches_window();
  rename_note(std::static_pointer_cast<Note>(note));
}

void SearchNotesWidget::on_note_saved(const NoteBase::Ptr&)
{
  restore_matches_window();
  update_results();
}

void SearchNotesWidget::delete_note(const Note::Ptr & note)
{
  Gtk::TreeModel::Children rows = m_store->children();

  for(Gtk::TreeModel::iterator iter = rows.begin();
      rows.end() != iter; iter++) {
    if(note == iter->get_value(m_column_types.note)) {
      m_store->erase(iter);
      break;
    }
  }
}

void SearchNotesWidget::add_note(const Note::Ptr & note)
{
  Glib::ustring nice_date = utils::get_pretty_print_date(note->change_date(), true, m_gnote.preferences());
  Gtk::TreeIter iter = m_store->append();
  iter->set_value(m_column_types.icon, get_note_icon(m_gnote.icon_manager()));
  iter->set_value(m_column_types.title, note->get_title());
  iter->set_value(m_column_types.change_date, nice_date);
  iter->set_value(m_column_types.note, note);
}

void SearchNotesWidget::rename_note(const Note::Ptr & note)
{
  Gtk::TreeModel::Children rows = m_store->children();

  for(Gtk::TreeModel::iterator iter = rows.begin();
      rows.end() != iter; iter++) {
    if(note == iter->get_value(m_column_types.note)) {
      iter->set_value(m_column_types.title, note->get_title());
      break;
    }
  }
}

void SearchNotesWidget::on_open_note()
{
  Note::List selected_notes = get_selected_notes ();
  Note::List::iterator iter = selected_notes.begin();
  if(iter != selected_notes.end()) {
    signal_open_note(*iter);
    ++iter;
  }
  for(; iter != selected_notes.end(); ++iter) {
    signal_open_note_new_window(*iter);
  }
}

void SearchNotesWidget::on_open_note_new_window()
{
  Note::List selected_notes = get_selected_notes();
  for(Note::List::iterator iter = selected_notes.begin();
      iter != selected_notes.end(); ++iter) {
    signal_open_note_new_window(*iter);
  }
}

void SearchNotesWidget::delete_selected_notes()
{
  Note::List selected_notes = get_selected_notes();
  if(selected_notes.empty()) {
    return;
  }

  noteutils::show_deletion_dialog(selected_notes, get_owning_window());
}

Gtk::Window *SearchNotesWidget::get_owning_window()
{
  Gtk::Widget *widget = get_parent();
  if(!widget) {
    return NULL;
  }
  Gtk::Widget *widget_parent = widget->get_parent();
  while(widget_parent) {
    widget = widget_parent;
    widget_parent = widget->get_parent();
  }

  return dynamic_cast<Gtk::Window*>(widget);
}

void SearchNotesWidget::on_note_added_to_notebook(const Note &,
                                                  const notebooks::Notebook::Ptr &)
{
  restore_matches_window();
  update_results();
}

void SearchNotesWidget::on_note_removed_from_notebook(const Note &,
                                                      const notebooks::Notebook::Ptr &)
{
  restore_matches_window();
  update_results();
}

void SearchNotesWidget::on_note_pin_status_changed(const Note &, bool)
{
  restore_matches_window();
  update_results();
}

Gtk::Menu *SearchNotesWidget::get_note_list_context_menu()
{
  if(!m_note_list_context_menu) {
    m_note_list_context_menu = new Gtk::Menu;

    Gtk::MenuItem *item;
    if(!m_gnote.preferences().open_notes_in_new_window()) {
      item = manage(new Gtk::MenuItem);
      item->set_related_action(m_open_note_action);
      item->add_accelerator("activate", m_accel_group, GDK_KEY_O, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
      m_note_list_context_menu->add(*item);
    }

    item = manage(new Gtk::MenuItem);
    item->set_related_action(m_open_note_new_window_action);
    item->add_accelerator("activate", m_accel_group, GDK_KEY_W, Gdk::MOD1_MASK, Gtk::ACCEL_VISIBLE);
    m_note_list_context_menu->add(*item);

    item = manage(new Gtk::MenuItem);
    item->set_related_action(m_delete_note_action);
    m_note_list_context_menu->add(*item);

    m_note_list_context_menu->add(*manage(new Gtk::SeparatorMenuItem));
    item = manage(new Gtk::MenuItem(_("_New"), true));
    item->add_accelerator("activate", m_accel_group, GDK_KEY_N, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    item->signal_activate().connect(sigc::mem_fun(*this, &SearchNotesWidget::new_note));
    m_note_list_context_menu->add(*item);
  }

  return m_note_list_context_menu;
}

void SearchNotesWidget::new_note()
{
  Note::Ptr note;
  notebooks::Notebook::Ptr notebook = get_selected_notebook();
  if(!notebook || std::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook)) {
    // Just create a standard note (not in a notebook)
    note = std::static_pointer_cast<Note>(m_manager.create());
  }
  else {
    // Look for the template note and create a new note
    note = notebook->create_notebook_note();
  }

  signal_open_note(note);
}

Gtk::Menu *SearchNotesWidget::get_notebook_list_context_menu()
{
  if(!m_notebook_list_context_menu) {
    m_notebook_list_context_menu = new Gtk::Menu;
    Gtk::MenuItem *item = manage(new Gtk::MenuItem(_("_Open Template Note"), true));
    item->signal_activate()
      .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_open_notebook_template_note));
    m_notebook_list_context_menu->add(*item);
    item = manage(new Gtk::MenuItem);
    item->set_related_action(m_rename_notebook_action);
    m_notebook_list_context_menu->add(*item);
    item = manage(new Gtk::MenuItem);
    item->set_related_action(m_delete_notebook_action);
    m_notebook_list_context_menu->add(*item);
    m_notebook_list_context_menu->add(*manage(new Gtk::SeparatorMenuItem));
    item = manage(new Gtk::MenuItem(_("_New..."), true));
    item->signal_activate()
      .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_new_notebook));
    m_notebook_list_context_menu->add(*item);
  }

  return m_notebook_list_context_menu;
}

void SearchNotesWidget::on_open_notebook_template_note()
{
  notebooks::Notebook::Ptr notebook = get_selected_notebook();
  if(!notebook) {
    return;
  }

  Note::Ptr templateNote = notebook->get_template_note();
  if(!templateNote) {
    return; // something seriously went wrong
  }

  signal_open_note(templateNote);
}

void SearchNotesWidget::on_new_notebook()
{
  notebooks::NotebookManager::prompt_create_new_notebook(m_gnote, get_owning_window());
}

void SearchNotesWidget::on_delete_notebook()
{
  notebooks::Notebook::Ptr notebook = get_selected_notebook();
  if(!notebook) {
    return;
  }

  notebooks::NotebookManager::prompt_delete_notebook(m_gnote, get_owning_window(), notebook);
}

void SearchNotesWidget::foreground()
{
  EmbeddableWidget::foreground();
  MainWindow *win = dynamic_cast<MainWindow*>(host());
  if(!win) {
    return;
  }

  win->add_accel_group(m_accel_group);
  auto & manager(m_gnote.action_manager());
  register_callbacks();
  m_callback_changed_cid = manager.signal_main_window_search_actions_changed
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::callbacks_changed));
}

void SearchNotesWidget::background()
{
  EmbeddableWidget::background();
  save_position();
  unregister_callbacks();
  m_callback_changed_cid.disconnect();
  Gtk::Window *win = dynamic_cast<Gtk::Window*>(host());
  if(win) {
    win->remove_accel_group(m_accel_group);
  }
}

void SearchNotesWidget::hint_size(int & width, int & height)
{
  width = m_gnote.preferences().search_window_width();
  height = m_gnote.preferences().search_window_height();
}

void SearchNotesWidget::size_internals()
{
  int pos = m_gnote.preferences().search_window_splitter_pos();
  if(pos) {
    set_position(pos);
  }
}

void SearchNotesWidget::set_initial_focus()
{
  Gtk::Window *win = dynamic_cast<Gtk::Window*>(host());
  if(win) {
    win->set_focus(*m_tree);
  }
}

std::vector<PopoverWidget> SearchNotesWidget::get_popover_widgets()
{
  std::vector<PopoverWidget> popover_widgets;
  popover_widgets.reserve(20);
  m_gnote.action_manager().signal_build_main_window_search_popover(popover_widgets);
  for(unsigned i = 0; i < popover_widgets.size(); ++i) {
    popover_widgets[i].secondary_order = i;
  }
  return popover_widgets;
}

std::vector<MainWindowAction::Ptr> SearchNotesWidget::get_widget_actions()
{
  std::vector<MainWindowAction::Ptr> actions;
  return actions;
}

void SearchNotesWidget::on_settings_changed()
{
  if(m_note_list_context_menu) {
    delete m_note_list_context_menu;
    m_note_list_context_menu = NULL;
  }
}

void SearchNotesWidget::on_sorting_changed()
{
  // don't do anything if in search mode
  if(m_matches_column && m_matches_column->get_visible()) {
    return;
  }

  if(m_store_sort) {
    m_store_sort->get_sort_column_id(m_sort_column_id, m_sort_column_order);
    Glib::ustring value;
    switch(m_sort_column_id) {
    case 1:
      value = "note:";
      break;
    case 2:
      value = "change:";
      break;
    default:
      return;
    }
    if(m_sort_column_order == Gtk::SORT_ASCENDING) {
      value += "asc";
    }
    else {
      value += "desc";
    }
    m_gnote.preferences().search_sorting(value);
  }
}

void SearchNotesWidget::parse_sorting_setting(const Glib::ustring & sorting)
{
  std::vector<Glib::ustring> tokens;
  sharp::string_split(tokens, sorting.lowercase(), ":");
  if(tokens.size() != 2) {
    ERR_OUT(_("Failed to parse setting search-sorting (Value: %s):"), sorting.c_str());
    ERR_OUT(_("Expected format 'column:order'"));
    return;
  }
  int column_id;
  Gtk::SortType order;
  if(tokens[0] == "note") {
    column_id = 1;
  }
  else if(tokens[0] == "change") {
    column_id = 2;
  }
  else {
    ERR_OUT(_("Failed to parse setting search-sorting (Value: %s):"), sorting.c_str());
    ERR_OUT(_("Unrecognized column %s"), tokens[0].c_str());
    return;
  }
  if(tokens[1] == "asc") {
    order = Gtk::SORT_ASCENDING;
  }
  else if(tokens[1] == "desc") {
    order = Gtk::SORT_DESCENDING;
  }
  else {
    ERR_OUT(_("Failed to parse setting search-sorting (Value: %s):"), sorting.c_str());
    ERR_OUT(_("Unrecognized order %s"), tokens[1].c_str());
    return;
  }

  m_sort_column_id = column_id;
  m_sort_column_order = order;
}

void SearchNotesWidget::on_rename_notebook()
{
  Glib::RefPtr<Gtk::TreeSelection> selection = m_notebooksTree->get_selection();
  if(!selection) {
    return;
  }
  std::vector<Gtk::TreeModel::Path> selected_row = selection->get_selected_rows();
  if(selected_row.size() != 1) {
    return;
  }
  m_notebooksTree->set_cursor(selected_row[0], *m_notebooksTree->get_column(0), true);
}

void SearchNotesWidget::callbacks_changed()
{
  unregister_callbacks();
  register_callbacks();
  signal_popover_widgets_changed();
}

void SearchNotesWidget::register_callbacks()
{
  MainWindow *win = dynamic_cast<MainWindow*>(host());
  if(!win) {
    return;
  }
  auto & manager(m_gnote.action_manager());
  auto cbacks = manager.get_main_window_search_callbacks();
  for(auto & cback : cbacks) {
    auto action = win->find_action(cback.first);
    if(action) {
      m_action_cids.push_back(action->signal_activate().connect(cback.second));
    }
  }
}

void SearchNotesWidget::unregister_callbacks()
{
  for(auto & cid : m_action_cids) {
    cid.disconnect();
  }
  m_action_cids.clear();
}

}
