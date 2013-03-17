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


#include <boost/format.hpp>
#include <glibmm/i18n.h>
#include <gtkmm/alignment.h>
#include <gtkmm/linkbutton.h>
#include <gtkmm/liststore.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/stock.h>
#include <gtkmm/table.h>

#include "debug.hpp"
#include "iconmanager.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "recenttreeview.hpp"
#include "search.hpp"
#include "searchnoteswidget.hpp"
#include "itagmanager.hpp"
#include "notebooks/notebookmanager.hpp"
#include "sharp/string.hpp"


namespace gnote {


Glib::RefPtr<Gdk::Pixbuf> SearchNotesWidget::get_note_icon()
{
  return IconManager::obj().get_icon(IconManager::NOTE, 22);
}


SearchNotesWidget::SearchNotesWidget(NoteManager & m)
  : Gtk::VBox(false, 0)
  , m_accel_group(Gtk::AccelGroup::create())
  , m_no_matches_box(NULL)
  , m_manager(m)
  , m_clickX(0), m_clickY(0)
  , m_matches_column(NULL)
  , m_note_list_context_menu(NULL)
  , m_notebook_list_context_menu(NULL)
  , m_initial_position_restored(false)
{
  make_actions();

  // Notebooks Pane
  Gtk::Widget *notebooksPane = Gtk::manage(make_notebooks_pane());
  notebooksPane->show();

  m_hpaned.set_position(150);
  m_hpaned.add1(*notebooksPane);
  m_hpaned.add2(m_matches_window);
  m_hpaned.show();

  make_recent_tree();
  m_tree = manage(m_tree);
  m_tree->show();

  update_results();

  m_matches_window.set_shadow_type(Gtk::SHADOW_IN);

  m_matches_window.property_hscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
  m_matches_window.property_vscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
  m_matches_window.add(*m_tree);
  m_matches_window.show();

  restore_position();

  pack_start(m_hpaned, true, true, 0);

  // Update on changes to notes
  m.signal_note_deleted.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_deleted));
  m.signal_note_added.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_added));
  m.signal_note_renamed.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_renamed));
  m.signal_note_saved.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_saved));

  // Watch when notes are added to notebooks so the search
  // results will be updated immediately instead of waiting
  // until the note's queue_save () kicks in.
  notebooks::NotebookManager::obj().signal_note_added_to_notebook()
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_added_to_notebook));
  notebooks::NotebookManager::obj().signal_note_removed_from_notebook()
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_removed_from_notebook));
  notebooks::NotebookManager::obj().signal_note_pin_status_changed
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_pin_status_changed));
}

SearchNotesWidget::~SearchNotesWidget()
{
  if(m_note_list_context_menu) {
    delete m_note_list_context_menu;
  }
  if(m_notebook_list_context_menu) {
    delete m_notebook_list_context_menu;
  }
}

std::string SearchNotesWidget::get_name() const
{
  notebooks::Notebook::Ptr selected_notebook = get_selected_notebook();
  if(!selected_notebook
     || std::tr1::dynamic_pointer_cast<notebooks::AllNotesNotebook>(selected_notebook)) {
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

  m_delete_notebook_action = Gtk::Action::create("DeleteNotebookAction", _("_Delete Notebook"));
  m_delete_notebook_action->signal_activate().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_delete_notebook));
}

void SearchNotesWidget::perform_search(const std::string & search_text)
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

  std::string text = m_search_text;
  if(text.empty()) {
    m_current_matches.clear();
    m_store_filter->refilter();
    if(m_tree->get_realized()) {
      m_tree->scroll_to_point (0, 0);
    }
    return;
  }
  text = sharp::string_to_lower(text);

  m_current_matches.clear();

  // Search using the currently selected notebook
  notebooks::Notebook::Ptr selected_notebook = get_selected_notebook();
  if (std::tr1::dynamic_pointer_cast<notebooks::SpecialNotebook>(selected_notebook)) {
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
    m_tree->scroll_to_point(0, 0);
  }
}

void SearchNotesWidget::restore_matches_window()
{
  if(m_no_matches_box && m_hpaned.get_child2() == m_no_matches_box) {
    m_hpaned.remove(*m_no_matches_box);
    m_hpaned.add2(m_matches_window);
    restore_position();
  }
}

Gtk::Widget *SearchNotesWidget::make_notebooks_pane()
{
  m_notebooksTree = Gtk::manage(
    new notebooks::NotebooksTreeView(m_manager,
                                     notebooks::NotebookManager::obj()
                                       .get_notebooks_with_special_items()));

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
  sw->set_shadow_type(Gtk::SHADOW_IN);
  sw->add(*m_notebooksTree);
  sw->show();

  return sw;
}

void SearchNotesWidget::restore_position()
{
  Glib::RefPtr<Gio::Settings> settings = Preferences::obj()
    .get_schema_settings(Preferences::SCHEMA_GNOTE);
  int x = settings->get_int(Preferences::SEARCH_WINDOW_X_POS);
  int y = settings->get_int(Preferences::SEARCH_WINDOW_Y_POS);
  int width = settings->get_int(Preferences::SEARCH_WINDOW_WIDTH);
  int height = settings->get_int(Preferences::SEARCH_WINDOW_HEIGHT);
  int pos = settings->get_int(Preferences::SEARCH_WINDOW_SPLITTER_POS);
  bool maximized = settings->get_boolean(Preferences::MAIN_WINDOW_MAXIMIZED);

  if((width == 0) || (height == 0)) {
    return;
  }
  Gtk::Window *window = get_owning_window();
  if(!window) {
    return;
  }

  if(window->get_realized()) {
    //if window is showing, use actual state
    maximized = window->get_window()->get_state() & Gdk::WINDOW_STATE_MAXIMIZED;
  }

  window->set_default_size(width, height);
  if(!m_initial_position_restored) {
    window->move(x, y);
    m_initial_position_restored = true;
  }

  if(!maximized && window->get_visible()) {
    window->get_window()->resize(width, height);
  }
  if(pos) {
    m_hpaned.set_position(pos);
  }
}

void SearchNotesWidget::save_position()
{
  int x;
  int y;
  int width;
  int height;

  utils::EmbeddableWidgetHost *current_host = host();
  if(!current_host || !current_host->running()) {
    return;
  }

  Glib::RefPtr<Gio::Settings> settings = Preferences::obj()
    .get_schema_settings(Preferences::SCHEMA_GNOTE);
  settings->set_int(Preferences::SEARCH_WINDOW_SPLITTER_POS, m_hpaned.get_position());

  Gtk::Window *window = dynamic_cast<Gtk::Window*>(current_host);
  if(!window || (window->get_window()->get_state() & Gdk::WINDOW_STATE_MAXIMIZED) != 0) {
    return;
  }

  window->get_position(x, y);
  window->get_size(width, height);

  settings->set_int(Preferences::SEARCH_WINDOW_X_POS, x);
  settings->set_int(Preferences::SEARCH_WINDOW_Y_POS, y);
  settings->set_int(Preferences::SEARCH_WINDOW_WIDTH, width);
  settings->set_int(Preferences::SEARCH_WINDOW_HEIGHT, height);
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
  crp->property_pixbuf() = notebook->get_icon();
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

  if(std::tr1::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook)) {
    // Bold the "Special" Notebooks
    crt->property_markup() = str(boost::format("<span weight=\"bold\">%1%</span>")
                                 % notebook->get_name());
  }
  else {
    crt->property_text() = notebook->get_name();
  }
}

void SearchNotesWidget::on_notebook_row_edited(const Glib::ustring& /*tree_path*/,
                                               const Glib::ustring& new_text)
{
  if(notebooks::NotebookManager::obj().notebook_exists(new_text) || new_text == "") {
    return;
  }
  notebooks::Notebook::Ptr old_notebook = this->get_selected_notebook();
  if(std::tr1::dynamic_pointer_cast<notebooks::SpecialNotebook>(old_notebook)) {
    return;
  }
  notebooks::Notebook::Ptr new_notebook = notebooks::NotebookManager::obj()
    .get_or_create_notebook(new_text);
  DBG_OUT("Renaming notebook '{%s}' to '{%s}'", old_notebook->get_name().c_str(),
          new_text.c_str());
  std::list<Note *> notes;
  old_notebook->get_tag()->get_notes(notes);
  for(std::list<Note *>::const_iterator note = notes.begin(); note != notes.end(); ++note) {
    notebooks::NotebookManager::obj().move_note_to_notebook(
      (*note)->shared_from_this(), new_notebook);
  }
  notebooks::NotebookManager::obj().delete_notebook(old_notebook);
  Gtk::TreeIter iter;
  if(notebooks::NotebookManager::obj().get_notebook_iter(new_notebook, iter)) {
    m_notebooksTree->get_selection()->select(iter);
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
    m_on_notebook_selection_changed_cid.unblock();
  }
  else {
    m_selected_tags.clear();
    if(notebook->get_tag()) {
      m_selected_tags.insert(notebook->get_tag());
    }
    bool allow_edit = false;
    if(std::tr1::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook)) {
      m_delete_notebook_action->set_sensitive(false);
    }
    else {
      m_delete_notebook_action->set_sensitive(true);
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
  if(ev->button == 3) {
    // third mouse button (right-click)
    Gtk::TreeViewColumn * col = 0; // unused
    Gtk::TreePath p;
    int cell_x, cell_y;            // unused
    const Glib::RefPtr<Gtk::TreeSelection> selection
      = m_notebooksTree->get_selection();

    if(m_notebooksTree->get_path_at_pos(ev->x, ev->y, p, col, cell_x, cell_y)) {
      selection->select(p);
    }

    Gtk::Menu *menu = get_notebook_list_context_menu();
    popup_context_menu_at_location(menu, ev->x, ev->y);
    return true;
  }
  return false;
}

bool SearchNotesWidget::on_notebooks_key_pressed(GdkEventKey *ev)
{
  switch(ev->keyval) {
  case GDK_KEY_Menu:
  {
    Gtk::Menu *menu = get_notebook_list_context_menu();
    popup_context_menu_at_location(menu, 0, 0);
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
  Gtk::TreeIter iter = model->children().begin();
  if(iter) {
    m_notebooksTree->get_selection()->select(iter);
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

  int cnt = 0;

  for(Note::List::const_iterator note_iter = m_manager.get_notes().begin();
      note_iter != m_manager.get_notes().end(); ++note_iter) {
    const Note::Ptr & note(*note_iter);
    std::string nice_date = utils::get_pretty_print_date(note->change_date(), true);

    Gtk::TreeIter iter = m_store->append();
    iter->set_value(0, get_note_icon());  /* icon */
    iter->set_value(1, note->get_title()); /* title */
    iter->set_value(2, nice_date);  /* change date */
    iter->set_value(3, note);      /* note */
    cnt++;
  }

  m_tree->set_model(m_store_sort);

  perform_search();

  if(sort_column >= 0) {
    // Set the sort column after loading data, since we
    // don't want to resort on every append.
    m_store_sort->set_sort_column(sort_column, sort_type);
  }

  // Restore the previous selection
  if(!selected_notes.empty()) {
    select_notes(selected_notes);
  }
}

void SearchNotesWidget::popup_context_menu_at_location(Gtk::Menu *menu, int x, int y)
{
  menu->show_all();

  // Set up the funtion to position the context menu
  // if we were called by the keyboard Gdk.Key.Menu.
  if(x == 0 && y == 0) {
    menu->popup(sigc::mem_fun(*this, &SearchNotesWidget::position_context_menu),
                0, gtk_get_current_event_time());
  }
  else {
    menu->popup(0, gtk_get_current_event_time());
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

  // Don't show the template notes in the list
  Tag::Ptr template_tag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
  if(note->contains_tag(template_tag)) {
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
  std::string title_a = (*a)[m_column_types.title];
  std::string title_b = (*b)[m_column_types.title];

  if(title_a.empty() || title_b.empty()) {
    return -1;
  }

  return title_a.compare(title_b);
}

int SearchNotesWidget::compare_dates(const Gtk::TreeIter & a, const Gtk::TreeIter & b)
{
  Note::Ptr note_a = (*a)[m_column_types.note];
  Note::Ptr note_b = (*b)[m_column_types.note];

  if(!note_a || !note_b) {
    return -1;
  }
  else {
    return sharp::DateTime::compare(note_a->change_date(), note_b->change_date());
  }
}

void SearchNotesWidget::make_recent_tree()
{
  m_targets.push_back(Gtk::TargetEntry("STRING", Gtk::TARGET_SAME_APP, 0));
  m_targets.push_back(Gtk::TargetEntry("text/plain", Gtk::TARGET_SAME_APP, 0));
  m_targets.push_back(Gtk::TargetEntry("text/uri-list", Gtk::TARGET_SAME_APP, 1));

  m_tree = Gtk::manage(new RecentTreeView());
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
  change->set_title(_("Last Changed"));
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

void SearchNotesWidget::position_context_menu(int & x, int & y, bool & push_in)
{
  // Set default "return" values
  push_in = false; // not used
  x = 0;
  y = 0;

  Gtk::Window *parent = get_owning_window();
  if(!parent) {
    return;
  }
  Gtk::Widget * const focus_widget = parent->get_focus();
  if(!focus_widget) {
    return;
  }
  focus_widget->get_window()->get_origin(x, y);

  Gtk::TreeView * const tree = dynamic_cast<Gtk::TreeView*>(focus_widget);
  if(!tree) {
    return;
  }
  const Glib::RefPtr<Gdk::Window> tree_area = tree->get_bin_window();
  if(!tree_area) {
    return;
  }
  tree_area->get_origin(x, y);

  const Glib::RefPtr<Gtk::TreeSelection> selection = tree->get_selection();
  const std::vector<Gtk::TreePath> selected_rows = selection->get_selected_rows();
  if(selected_rows.empty()) {
    return;
  }

  const Gtk::TreePath & dest_path = selected_rows.front();
  const std::vector<Gtk::TreeViewColumn*> columns = tree->get_columns();
  Gdk::Rectangle cell_rect;
  tree->get_cell_area(dest_path, *columns.front(), cell_rect);

  x += cell_rect.get_x();
  y += cell_rect.get_y();
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
  std::list<Tag::Ptr> tags;
  note->get_tags(tags);
  for(std::list<Tag::Ptr>::const_iterator iter = tags.begin();
      iter != tags.end(); ++iter) {
    if(m_selected_tags.find(*iter) != m_selected_tags.end()) {
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
    m_delete_note_action->property_sensitive() = false;
  }
  else {
    m_open_note_action->property_sensitive() = true;
    m_delete_note_action->property_sensitive() = true;
  }
}

bool SearchNotesWidget::on_treeview_button_pressed(GdkEventButton *ev)
{
  if(ev->window != m_tree->get_bin_window()->gobj()) {
    return false;
  }

  Gtk::TreePath dest_path;
  Gtk::TreeViewColumn *column = NULL;
  int unused;

  m_tree->get_path_at_pos(ev->x, ev->y, dest_path, column, unused, unused);

  m_clickX = ev->x;
  m_clickY = ev->y;

  bool retval = false;

  switch(ev->type) {
  case GDK_2BUTTON_PRESS:
    if(ev->button != 1 || (ev->state & (Gdk::CONTROL_MASK | Gdk::SHIFT_MASK)) != 0) {
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
  case GDK_BUTTON_PRESS:
    if(ev->button == 3) {
      const Glib::RefPtr<Gtk::TreeSelection> selection = m_tree->get_selection();

      if(selection->get_selected_rows().size() <= 1) {
        Gtk::TreeViewColumn * col = 0; // unused
        Gtk::TreePath p;
        int cell_x, cell_y;            // unused
        if(m_tree->get_path_at_pos(ev->x, ev->y, p, col, cell_x, cell_y)) {
          selection->unselect_all();
          selection->select(p);
        }
      }
      Gtk::Menu *menu = get_note_list_context_menu();
      popup_context_menu_at_location(menu, ev->x, ev->y);

      // Return true so that the base handler won't
      // run, which causes the selection to change to
      // the row that was right-clicked.
      retval = true;
      break;
    }

    if(m_tree->get_selection()->is_selected(dest_path)
       && (ev->state & (Gdk::CONTROL_MASK | Gdk::SHIFT_MASK)) == 0) {
      if(column && (ev->button == 1)) {
        Gtk::CellRenderer *renderer = column->get_first_cell();
        Gdk::Rectangle background_area;
        m_tree->get_background_area(dest_path, *column, background_area);
        Gdk::Rectangle cell_area;
        m_tree->get_cell_area(dest_path, *column, cell_area);

        renderer->activate((GdkEvent*)ev, *m_tree,
                           dest_path.to_string (),
                           background_area, cell_area,
                           Gtk::CELL_RENDERER_SELECTED);

        Gtk::TreeIter iter = m_tree->get_model()->get_iter (dest_path);
        if(iter) {
          m_tree->get_model()->row_changed(dest_path, iter);
        }
      }

      retval = true;
    }

    break;
  default:
    retval = false;
    break;
  }
  return retval;
}

bool SearchNotesWidget::on_treeview_motion_notify(GdkEventMotion *ev)
{
  if((ev->state & Gdk::BUTTON1_MASK) == 0) {
    return false;
  }
  else if(ev->window != m_tree->get_bin_window()->gobj()) {
    return false;
  }

  bool retval = true;

  if(!m_tree->drag_check_threshold(m_clickX, m_clickY, ev->x, ev->y)) {
    return retval;
  }

  Gtk::TreePath dest_path;
  Gtk::TreeViewColumn * col = NULL; // unused
  int cell_x, cell_y;               // unused
  if(!m_tree->get_path_at_pos(ev->x, ev->y, dest_path, col, cell_x, cell_y)) {
    return retval;
  }

  m_tree->drag_begin(Gtk::TargetList::create (m_targets), Gdk::ACTION_MOVE, 1, (GdkEvent*)ev);
  return retval;
}

bool SearchNotesWidget::on_treeview_button_released(GdkEventButton *ev)
{
  if(!m_tree->drag_check_threshold(m_clickX, m_clickY, ev->x, ev->y)
     && ((ev->state & (Gdk::CONTROL_MASK | Gdk::SHIFT_MASK)) == 0)
     && m_tree->get_selection()->count_selected_rows () > 1) {
    Gtk::TreePath dest_path;
    Gtk::TreeViewColumn * col = NULL; // unused
    int cell_x, cell_y;               // unused
    m_tree->get_path_at_pos(ev->x, ev->y, dest_path, col, cell_x, cell_y);
    m_tree->get_selection()->unselect_all();
    m_tree->get_selection()->select(dest_path);
  }
  return false;
}

bool SearchNotesWidget::on_treeview_key_pressed(GdkEventKey * ev)
{
  switch(ev->keyval) {
  case GDK_KEY_Delete:
    delete_selected_notes();
    break;
  case GDK_KEY_Menu:
  {
    // Pop up the context menu if a note is selected
    Note::List selected_notes = get_selected_notes();
    if(!selected_notes.empty()) {
      Gtk::Menu *menu = get_note_list_context_menu();
      popup_context_menu_at_location(menu, 0, 0);
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
  if(m_matches_column == NULL) {
    return;
  }

  m_tree->remove_column(*m_matches_column);
  m_matches_column = NULL;

  m_store_sort->set_sort_column(2, Gtk::SORT_DESCENDING);
}

// called when no search results are found in the selected notebook
void SearchNotesWidget::no_matches_found_action()
{
  m_hpaned.remove(m_matches_window);
  if(!m_no_matches_box) {
    Glib::ustring message = _("No results found in the selected notebook.\nClick here to search across all notes.");
    Gtk::LinkButton *link_button = manage(new Gtk::LinkButton("", message));
    link_button->signal_activate_link()
      .connect(sigc::mem_fun(*this, &SearchNotesWidget::show_all_search_results));
    link_button->set_tooltip_text(_("Click here to search across all notebooks"));
    link_button->show();
    Gtk::Table *no_matches_found_table = manage(new Gtk::Table(1, 3, false));
    no_matches_found_table->attach(*link_button, 1, 2, 0, 1,
      Gtk::FILL | Gtk::SHRINK,
      Gtk::SHRINK,
      0, 0
    );

    no_matches_found_table->set_col_spacings(4);
    no_matches_found_table->show_all();
    m_no_matches_box = manage(new Gtk::HBox(false, 0));
    m_no_matches_box->pack_start(*no_matches_found_table, true, true, 0);
    m_no_matches_box->show();
  }
  m_hpaned.add2(*m_no_matches_box);
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
}

bool SearchNotesWidget::show_all_search_results()
{
  Gtk::TreeIter iter = m_notebooksTree->get_model()->children().begin();
  m_notebooksTree->get_selection()->select(iter);
  return false;
}

void SearchNotesWidget::matches_column_data_func(Gtk::CellRenderer * cell,
                                                 const Gtk::TreeIter & iter)
{
  Gtk::CellRendererText *crt = dynamic_cast<Gtk::CellRendererText*>(cell);
  if(!crt) {
    return;
  }

  std::string match_str = "";

  Note::Ptr note = (*iter)[m_column_types.note];
  if(note) {
    int match_count;
    std::map<std::string, int>::const_iterator miter
      = m_current_matches.find(note->uri());
    if(miter != m_current_matches.end()) {
      match_count = miter->second;
      if(match_count == INT_MAX) {
        //TRANSLATORS: search found a match in note title
        match_str = _("Title match");
      }
      else if(match_count > 0) {
        const char * fmt;
        fmt = ngettext("%1% match", "%1% matches", match_count);
        match_str = str(boost::format(fmt) % match_count);
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
  std::map<std::string, int>::iterator iter_a = m_current_matches.find(note_a->uri());
  std::map<std::string, int>::iterator iter_b = m_current_matches.find(note_b->uri());
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

void SearchNotesWidget::on_note_deleted(const Note::Ptr & note)
{
  restore_matches_window();
  delete_note(note);
}

void SearchNotesWidget::on_note_added(const Note::Ptr & note)
{
  restore_matches_window();
  add_note(note);
}

void SearchNotesWidget::on_note_renamed(const Note::Ptr & note,
                                        const std::string &)
{
  restore_matches_window();
  rename_note(note);
}

void SearchNotesWidget::on_note_saved(const Note::Ptr&)
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
  std::string nice_date =
    utils::get_pretty_print_date(note->change_date(), true);
  Gtk::TreeIter iter = m_store->append();
  iter->set_value(m_column_types.icon, get_note_icon());
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

    Gtk::MenuItem *item = manage(new Gtk::MenuItem);
    item->set_related_action(m_open_note_action);
    item->add_accelerator("activate", m_accel_group, GDK_KEY_O, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    m_note_list_context_menu->add(*item);

    item = manage(new Gtk::MenuItem);
    item->set_related_action(m_open_note_new_window_action);
    item->add_accelerator("activate", m_accel_group, GDK_KEY_W, Gdk::MOD1_MASK, Gtk::ACCEL_VISIBLE);
    m_note_list_context_menu->add(*item);

    item = manage(new Gtk::MenuItem);
    item->set_related_action(m_delete_note_action);
    m_note_list_context_menu->add(*item);

    m_note_list_context_menu->add(*manage(new Gtk::SeparatorMenuItem));
    item = manage(new Gtk::MenuItem(_("_New Note"), true));
    item->signal_activate().connect(sigc::mem_fun(*this, &SearchNotesWidget::new_note));
    m_note_list_context_menu->add(*item);
  }

  return m_note_list_context_menu;
}

void SearchNotesWidget::new_note()
{
  Note::Ptr note;
  notebooks::Notebook::Ptr notebook = get_selected_notebook();
  if(!notebook || std::tr1::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook)) {
    // Just create a standard note (not in a notebook)
    note = m_manager.create();
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
    item->set_related_action(m_delete_notebook_action);
    m_notebook_list_context_menu->add(*item);
    m_notebook_list_context_menu->add(*manage(new Gtk::SeparatorMenuItem));
    item = manage(new Gtk::MenuItem(_("_New Notebook"), true));
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
  notebooks::NotebookManager::prompt_create_new_notebook(get_owning_window());
}

void SearchNotesWidget::on_delete_notebook()
{
  notebooks::Notebook::Ptr notebook = get_selected_notebook();
  if(!notebook) {
    return;
  }

  notebooks::NotebookManager::prompt_delete_notebook(get_owning_window(), notebook);
}

void SearchNotesWidget::foreground()
{
  utils::EmbeddableWidget::foreground();
  restore_position();
  Gtk::Window *win = dynamic_cast<Gtk::Window*>(host());
  if(win) {
    win->add_accel_group(m_accel_group);
  }
}

void SearchNotesWidget::background()
{
  utils::EmbeddableWidget::background();
  save_position();
  Gtk::Window *win = dynamic_cast<Gtk::Window*>(host());
  if(win) {
    win->remove_accel_group(m_accel_group);
  }
}

}
