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


#include <glibmm/i18n.h>
#include <gtkmm/dragsource.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/linkbutton.h>
#include <gtkmm/liststore.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/shortcutcontroller.h>

#include "debug.hpp"
#include "iactionmanager.hpp"
#include "iconmanager.hpp"
#include "ignote.hpp"
#include "mainwindow.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "search.hpp"
#include "searchnoteswidget.hpp"
#include "itagmanager.hpp"
#include "notebooks/notebookmanager.hpp"
#include "notebooks/specialnotebooks.hpp"
#include "sharp/string.hpp"


namespace gnote {


SearchNotesWidget::SearchNotesWidget(IGnote & g, NoteManagerBase & m)
  : m_gnote(g)
  , m_manager(m)
  , m_clickX(0), m_clickY(0)
  , m_matches_column(NULL)
  , m_initial_position_restored(false)
  , m_sort_column_id(2)
  , m_sort_column_order(Gtk::SortType::DESCENDING)
{
  set_hexpand(true);
  set_vexpand(true);

  // Notebooks Pane
  Gtk::Widget *notebooksPane = Gtk::manage(make_notebooks_pane());

  set_position(150);
  set_start_child(*notebooksPane);
  set_end_child(m_matches_window);

  make_recent_tree();
  m_tree->set_enable_search(false);

  update_results();

  m_matches_window.property_hscrollbar_policy() = Gtk::PolicyType::AUTOMATIC;
  m_matches_window.property_vscrollbar_policy() = Gtk::PolicyType::AUTOMATIC;
  m_matches_window.set_child(*m_tree);

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

  parse_sorting_setting(g.preferences().search_sorting());
  g.preferences().signal_desktop_gnome_clock_format_changed.connect(sigc::mem_fun(*this, &SearchNotesWidget::update_results));

  auto shortcuts = Gtk::ShortcutController::create();
  shortcuts->set_scope(Gtk::ShortcutScope::GLOBAL);
  auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_O, Gdk::ModifierType::CONTROL_MASK);
  auto action = Gtk::NamedAction::create("win.open-note");
  auto shortcut = Gtk::Shortcut::create(trigger, action);
  shortcuts->add_shortcut(shortcut);
  trigger = Gtk::KeyvalTrigger::create(GDK_KEY_W, Gdk::ModifierType::ALT_MASK);
  action = Gtk::NamedAction::create("win.open-note-new-window");
  shortcut = Gtk::Shortcut::create(trigger, action);
  shortcuts->add_shortcut(shortcut);
  add_controller(shortcuts);
}

Glib::ustring SearchNotesWidget::get_name() const
{
  auto selected_notebook = m_notebooks_view->get_selected_notebook();
  if(!selected_notebook) {
    return "";
  }
  return selected_notebook->get_name();
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
  auto selected_notebook = m_notebooks_view->get_selected_notebook();
  if(std::dynamic_pointer_cast<notebooks::SpecialNotebook>(selected_notebook)) {
    selected_notebook = notebooks::Notebook::Ptr();
  }

  auto results = search.search_notes(text, false, selected_notebook);
  // if no results found in current notebook ask user whether
  // to search in all notebooks
  if(results.size() == 0 && selected_notebook != NULL) {
    no_matches_found_action();
  }
  else {
    for(auto iter = results.rbegin(); iter != results.rend(); ++iter) {
      m_current_matches[iter->second.get().uri()] = iter->first;
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
  if(m_no_matches_box && get_end_child() == m_no_matches_box.get()) {
    set_end_child(m_matches_window);
  }
}

Gtk::Widget *SearchNotesWidget::make_notebooks_pane()
{
  m_notebooks_view = Gtk::make_managed<notebooks::NotebooksView>(m_manager, m_gnote.notebook_manager().get_notebooks_with_special_items());

  m_notebooks_view->get_selection()->set_mode(Gtk::SelectionMode::SINGLE);
  m_notebooks_view->set_headers_visible(true);

  Gtk::CellRenderer *renderer;

  Gtk::TreeViewColumn *column = manage(new Gtk::TreeViewColumn());
  column->set_title(_("Notebooks"));
  column->set_sizing(Gtk::TreeViewColumn::Sizing::AUTOSIZE);
  column->set_resizable(false);

  renderer = manage(new Gtk::CellRendererPixbuf());
  column->pack_start(*renderer, false);
  column->set_cell_data_func(*renderer, sigc::mem_fun(*this, &SearchNotesWidget::notebook_pixbuf_cell_data_func));

  Gtk::CellRendererText *text_renderer = manage(new Gtk::CellRendererText());
  text_renderer->property_editable() = true;
  column->pack_start(*text_renderer, true);
  column->set_cell_data_func(*text_renderer, sigc::mem_fun(*this, &SearchNotesWidget::notebook_text_cell_data_func));
  text_renderer->signal_edited().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_notebook_row_edited));

  m_notebooks_view->append_column(*column);

  m_notebooks_view->signal_selected_notebook_changed
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_notebook_selection_changed));

  auto button_ctrl = Gtk::GestureClick::create();
  button_ctrl->set_button(3);
  button_ctrl->signal_pressed()
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_notebooks_view_right_click), false);
  m_notebooks_view->add_controller(button_ctrl);

  auto key_ctrl = Gtk::EventControllerKey::create();
  key_ctrl->signal_key_pressed().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_notebooks_key_pressed), false);
  m_notebooks_view->add_controller(key_ctrl);

  Gtk::ScrolledWindow *sw = new Gtk::ScrolledWindow();
  sw->property_hscrollbar_policy() = Gtk::PolicyType::AUTOMATIC;
  sw->property_vscrollbar_policy() = Gtk::PolicyType::AUTOMATIC;
  sw->set_child(*m_notebooks_view);

  return sw;
}

void SearchNotesWidget::save_position()
{
  EmbeddableWidgetHost *current_host = host();
  if(!current_host || !current_host->running()) {
    return;
  }

  m_gnote.preferences().search_window_splitter_pos(get_position());

  Gtk::Window *window = dynamic_cast<Gtk::Window*>(current_host);
  if(!window || window->is_maximized()) {
    return;
  }

  int width = window->get_width();
  int height = window->get_height();

  m_gnote.preferences().search_window_width(width);
  m_gnote.preferences().search_window_height(height);
}

void SearchNotesWidget::notebook_pixbuf_cell_data_func(Gtk::CellRenderer * renderer, const Gtk::TreeIter<Gtk::TreeConstRow> & iter)
{
  notebooks::Notebook::Ptr notebook;
  iter->get_value(0, notebook);
  if(!notebook) {
    return;
  }

  Gtk::CellRendererPixbuf *crp = dynamic_cast<Gtk::CellRendererPixbuf*>(renderer);
  notebooks::SpecialNotebook::Ptr special_nb = std::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook);
  crp->property_icon_name() = special_nb ?  special_nb->get_icon_name() : IconManager::NOTEBOOK;
}

void SearchNotesWidget::notebook_text_cell_data_func(Gtk::CellRenderer *renderer, const Gtk::TreeIter<Gtk::TreeConstRow> & iter)
{
  Gtk::CellRendererText *crt = dynamic_cast<Gtk::CellRendererText*>(renderer);
  crt->property_ellipsize() = Pango::EllipsizeMode::END;
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
  auto old_notebook = m_notebooks_view->get_selected_notebook();
  if(std::dynamic_pointer_cast<notebooks::SpecialNotebook>(old_notebook)) {
    return;
  }
  notebooks::Notebook::Ptr new_notebook = notebook_manager.get_or_create_notebook(new_text);
  DBG_OUT("Renaming notebook '{%s}' to '{%s}'", old_notebook->get_name().c_str(),
          new_text.c_str());
  auto notes = old_notebook->get_tag()->get_notes();
  for(NoteBase *note : notes) {
    notebook_manager.move_note_to_notebook(static_cast<Note&>(*note), new_notebook);
  }
  notebook_manager.delete_notebook(old_notebook);
  Gtk::TreeIter<Gtk::TreeRow> iter;
  if(notebook_manager.get_notebook_iter(new_notebook, iter)) {
    m_notebooks_view->get_selection()->select(iter);
    m_notebooks_view->set_cursor(m_notebooks_view->get_model()->get_path(iter));
  }
}

void SearchNotesWidget::on_notebook_selection_changed(const notebooks::Notebook::Ptr & notebook)
{
  restore_matches_window();

  bool allow_edit = false;
  if(!std::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook)) {
    allow_edit = true;
  }

  std::vector<Gtk::CellRenderer*> renderers = m_notebooks_view->get_column(0)->get_cells();
  for(std::vector<Gtk::CellRenderer*>::iterator renderer = renderers.begin();
      renderer != renderers.end(); ++renderer) {
    Gtk::CellRendererText *text_rederer = dynamic_cast<Gtk::CellRendererText*>(*renderer);
    if(text_rederer) {
      text_rederer->property_editable() = allow_edit;
    }
  }

  if(auto win = dynamic_cast<MainWindow*>(host())) {
    if(auto action = win->find_action("rename-notebook")) {
      action->set_enabled(allow_edit);
    }
    if(auto action = win->find_action("delete-notebook")) {
      action->set_enabled(allow_edit);
    }
  }
  update_results();
  signal_name_changed(get_name());
}

void SearchNotesWidget::on_notebooks_view_right_click(int n_press, double x, double y)
{
  auto popover = get_notebook_list_context_menu();
  Gdk::Rectangle pos;
  pos.set_x(x);
  pos.set_y(y);
  popover->set_pointing_to(pos);
  popover->popup();
}

bool SearchNotesWidget::on_notebooks_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state)
{
  switch(keyval) {
  case GDK_KEY_F2:
    on_rename_notebook();
    return true;
  case GDK_KEY_Menu:
  {
    Gtk::Popover *menu = get_notebook_list_context_menu();
    popup_context_menu_at_location(menu, m_notebooks_view);
    return true;
  }
  default:
    break;
  }

  return false; // Let Escape be handled by the window.
}

void SearchNotesWidget::select_all_notes_notebook()
{
  m_notebooks_view->select_all_notes_notebook();
}

void SearchNotesWidget::update_results()
{
  // Save the currently selected notes
  Note::List selected_notes = get_selected_notes();

  int sort_column = RecentNotesColumnTypes::CHANGE_DATE;
  Gtk::SortType sort_type = Gtk::SortType::DESCENDING;
  if(m_store_sort) {
    m_store_sort->get_sort_column_id(sort_column, sort_type);
  }

  m_store = Gtk::ListStore::create(m_column_types);

  m_store_filter = Gtk::TreeModelFilter::create(m_store);
  m_store_filter->set_visible_func(sigc::mem_fun(*this, &SearchNotesWidget::filter_notes));
  m_store_sort = Gtk::TreeModelSort::create(m_store_filter);
  m_store_sort->set_sort_func(RecentNotesColumnTypes::TITLE, sigc::mem_fun(*this, &SearchNotesWidget::compare_titles));
  m_store_sort->set_sort_func(RecentNotesColumnTypes::CHANGE_DATE, sigc::mem_fun(*this, &SearchNotesWidget::compare_dates));
  m_store_sort->set_sort_column(m_sort_column_id, m_sort_column_order);
  m_store_sort->signal_sort_column_changed()
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_sorting_changed));

  m_manager.copy_to(m_store, [this](const Glib::RefPtr<Gtk::ListStore> & store, const NoteBase::Ptr & note_base) {
    Note::Ptr note(std::static_pointer_cast<Note>(note_base));
    Glib::ustring nice_date = utils::get_pretty_print_date(note->change_date(), true, m_gnote.preferences());

    Gtk::TreeIter iter = store->append();
    iter->set_value(RecentNotesColumnTypes::TITLE, note->get_title());
    iter->set_value(RecentNotesColumnTypes::CHANGE_DATE, nice_date);
    iter->set_value(RecentNotesColumnTypes::NOTE, note);
  });

  m_tree->set_model(m_store_sort);

  perform_search();

  if(sort_column >= 0 && m_no_matches_box && get_end_child() != m_no_matches_box.get()) {
    // Set the sort column after loading data, since we
    // don't want to resort on every append.
    m_store_sort->set_sort_column(sort_column, sort_type);
  }

  // Restore the previous selection
  if(!selected_notes.empty()) {
    select_notes(selected_notes);
  }
}

void SearchNotesWidget::popup_context_menu_at_location(Gtk::Popover *popover, Gtk::TreeView *tree)
{
  const auto selection = tree->get_selection();
  const auto selected_rows = selection->get_selected_rows();
  if(!selected_rows.empty()) {
    const Gtk::TreePath & dest_path = selected_rows.front();
    const std::vector<Gtk::TreeViewColumn*> columns = tree->get_columns();
    Gdk::Rectangle cell_rect;
    tree->get_cell_area(dest_path, *columns.front(), cell_rect);
    popover->set_pointing_to(cell_rect);
  }
  popover->popup();
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

bool SearchNotesWidget::filter_notes(const Gtk::TreeIter<Gtk::TreeConstRow> & iter)
{
  Note::Ptr note = (*iter)[m_column_types.note];
  if(!note) {
    return false;
  }

  auto selected_notebook = m_notebooks_view->get_selected_notebook();
  if(!selected_notebook || !selected_notebook->contains_note(*note)) {
    return false;
  }

  bool passes_search_filter = filter_by_search(*note);
  if(passes_search_filter == false) {
    return false; // don't waste time checking tags if it's already false
  }

  bool passes_tag_filter = true; // no selected notebook
  if(auto notebook = m_notebooks_view->get_selected_notebook()) {
    if(auto tag = notebook->get_tag()) {
      passes_tag_filter = filter_by_tag(*note, tag);
    }
  }

  // Must pass both filters to appear in the list
  return passes_tag_filter && passes_search_filter;
}

int SearchNotesWidget::compare_titles(const Gtk::TreeIter<Gtk::TreeConstRow> & a, const Gtk::TreeIter<Gtk::TreeConstRow> & b)
{
  Glib::ustring title_a = (*a)[m_column_types.title];
  Glib::ustring title_b = (*b)[m_column_types.title];

  if(title_a.empty() || title_b.empty()) {
    return -1;
  }

  return title_a.lowercase().compare(title_b.lowercase());
}

int SearchNotesWidget::compare_dates(const Gtk::TreeIter<Gtk::TreeConstRow> & a, const Gtk::TreeIter<Gtk::TreeConstRow> & b)
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
  m_tree = Gtk::make_managed<Gtk::TreeView>();
  m_tree->set_headers_visible(true);
  m_tree->signal_row_activated().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_row_activated));
  m_tree->get_selection()->set_mode(Gtk::SelectionMode::MULTIPLE);
  m_tree->get_selection()->signal_changed().connect(
    sigc::mem_fun(*this, &SearchNotesWidget::on_selection_changed));

  auto button_ctrl = Gtk::GestureClick::create();
  button_ctrl->set_button(3);
  button_ctrl->signal_pressed().connect(
    sigc::mem_fun(*this, &SearchNotesWidget::on_treeview_right_button_pressed));
  m_tree->add_controller(button_ctrl);

  m_notes_widget_key_ctrl = Gtk::EventControllerKey::create();
  m_notes_widget_key_ctrl->signal_key_pressed().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_treeview_key_pressed), false);
  m_tree->add_controller(m_notes_widget_key_ctrl);

  auto drag_source = Gtk::DragSource::create();
  auto paintable = Gtk::IconTheme::get_for_display(Gdk::Display::get_default())->lookup_icon(IconManager::NOTE, 24);
  drag_source->set_icon(paintable, 0, 0);
  drag_source->signal_prepare().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_treeview_drag_data_get), false);
  m_tree->add_controller(drag_source);

  Gtk::CellRenderer *renderer;

  Gtk::TreeViewColumn *title = manage(new Gtk::TreeViewColumn());
  title->set_title(_("Note"));
  title->set_min_width(150);
  title->set_sizing(Gtk::TreeViewColumn::Sizing::AUTOSIZE);
  title->set_expand(true);
  title->set_resizable(true);

  {
    auto renderer = Gtk::make_managed<Gtk::CellRendererPixbuf>();
    renderer->property_icon_name() = IconManager::NOTE;
    title->pack_start(*renderer, false);
  }

  renderer = manage(new Gtk::CellRendererText());
  static_cast<Gtk::CellRendererText*>(renderer)->property_ellipsize() = Pango::EllipsizeMode::END;
  title->pack_start(*renderer, true);
  title->add_attribute(*renderer, "text", RecentNotesColumnTypes::TITLE);
  title->set_sort_column(RecentNotesColumnTypes::TITLE);
  title->set_sort_indicator(false);
  title->set_reorderable(false);
  title->set_sort_order(Gtk::SortType::ASCENDING);

  m_tree->append_column(*title);

  Gtk::TreeViewColumn *change = manage(new Gtk::TreeViewColumn());
  change->set_title(_("Modified"));
  change->set_sizing(Gtk::TreeViewColumn::Sizing::AUTOSIZE);
  change->set_resizable(false);

  renderer = manage(new Gtk::CellRendererText());
  renderer->property_xalign() = 1.0;
  change->pack_start(*renderer, false);
  change->add_attribute(*renderer, "text", RecentNotesColumnTypes::CHANGE_DATE);
  change->set_sort_column(RecentNotesColumnTypes::CHANGE_DATE);
  change->set_sort_indicator(false);
  change->set_reorderable(false);
  change->set_sort_order(Gtk::SortType::DESCENDING);

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

bool SearchNotesWidget::filter_by_search(const Note & note)
{
  if(m_search_text.empty()) {
    return true;
  }

  if(m_current_matches.empty()) {
    return false;
  }

  return m_current_matches.find(note.uri()) != m_current_matches.end();
}

bool SearchNotesWidget::filter_by_tag(const Note & note, const Tag::Ptr & tag)
{
  for(auto & t : note.get_tags()) {
    if(tag == t) {
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
  signal_open_note(*note);
}

void SearchNotesWidget::on_selection_changed()
{
  if(auto win = dynamic_cast<MainWindow*>(host())) {
    bool enabled = get_selected_notes().empty() == false;
    if(auto action = win->find_action("open-note")) {
      action->set_enabled(enabled);
    }
    if(auto action = win->find_action("open-note-new-window")) {
      action->set_enabled(enabled);
    }
    if(auto action = win->find_action("delete-selected-notes")) {
      action->set_enabled(enabled);
    }
  }
}

void SearchNotesWidget::on_treeview_right_button_pressed(int n_press, double x, double y)
{
  auto popover = get_note_list_context_menu();
  Gdk::Rectangle pos;
  pos.set_x(x);
  pos.set_y(y);
  popover->set_pointing_to(pos);
  popover->popup();
}

bool SearchNotesWidget::on_treeview_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state)
{
  switch(keyval) {
  case GDK_KEY_Delete:
    delete_selected_notes();
    break;
  case GDK_KEY_Menu:
  {
    // Pop up the context menu if a note is selected
    Note::List selected_notes = get_selected_notes();
    if(!selected_notes.empty()) {
      auto menu = get_note_list_context_menu();
      popup_context_menu_at_location(menu, m_tree);
    }
    return true;
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

Glib::RefPtr<Gdk::ContentProvider> SearchNotesWidget::on_treeview_drag_data_get(double, double)
{
  Note::List selected_notes = get_selected_notes();
  if(selected_notes.empty()) {
    // TODO grab note under cursor
    return Glib::RefPtr<Gdk::ContentProvider>();
  }

  if(selected_notes.size() == 1) {
    Glib::Value<Glib::ustring> value;
    value.init(Glib::Value<Glib::ustring>::value_type());
    value.set(selected_notes.front()->uri());
    return Gdk::ContentProvider::create(value);
  }
  else {
    std::vector<Glib::ustring> uris;
    for(const auto & note : selected_notes) {
      uris.emplace_back(note->uri());
    }

    Glib::Value<std::vector<Glib::ustring>> value;
    value.init(Glib::Value<std::vector<Glib::ustring>>::value_type());
    value.set(uris);
    return Gdk::ContentProvider::create(value);
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
  if(!m_no_matches_box) {
    Glib::ustring message = _("No results found in the selected notebook.\nClick here to search across all notes.");
    Gtk::LinkButton *link_button = manage(new Gtk::LinkButton("", message));
    link_button->signal_activate_link()
      .connect(sigc::mem_fun(*this, &SearchNotesWidget::show_all_search_results), false);
    link_button->set_tooltip_text(_("Click here to search across all notebooks"));
    link_button->set_hexpand(true);
    link_button->set_vexpand(true);
    link_button->set_halign(Gtk::Align::CENTER);
    link_button->set_valign(Gtk::Align::CENTER);

    m_no_matches_box = std::make_shared<Gtk::Grid>();
    m_no_matches_box->attach(*link_button, 0, 0, 1, 1);
  }
  set_end_child(*m_no_matches_box);
}

void SearchNotesWidget::add_matches_column()
{
  if(!m_matches_column) {
    Gtk::CellRenderer *renderer;

    m_matches_column = manage(new Gtk::TreeViewColumn());
    m_matches_column->set_title(_("Matches"));
    m_matches_column->set_sizing(Gtk::TreeViewColumn::Sizing::AUTOSIZE);
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
    m_matches_column->set_sort_order(Gtk::SortType::DESCENDING);
    m_matches_column->set_clickable(true);
    m_store_sort->set_sort_func(4 /* matches */,
                                sigc::mem_fun(*this, &SearchNotesWidget::compare_search_hits));

    m_tree->append_column(*m_matches_column);
    m_store_sort->set_sort_column(4, Gtk::SortType::DESCENDING);
  }
  else {
    m_matches_column->set_visible(true);
    m_store_sort->set_sort_column(4, Gtk::SortType::DESCENDING);
  }
}

bool SearchNotesWidget::show_all_search_results()
{
  select_all_notes_notebook();
  return true;
}

void SearchNotesWidget::matches_column_data_func(Gtk::CellRenderer * cell, const Gtk::TreeIter<Gtk::TreeConstRow> & iter)
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

int SearchNotesWidget::compare_search_hits(const Gtk::TreeIter<Gtk::TreeConstRow> & a, const Gtk::TreeIter<Gtk::TreeConstRow> & b)
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
        if(sort_type == Gtk::SortType::DESCENDING) {
          result = -result; // reverse sign
        }
      }
    }

    return result;
  }

  return result;
}

void SearchNotesWidget::on_note_deleted(NoteBase & note)
{
  restore_matches_window();
  delete_note(note);
}

void SearchNotesWidget::on_note_added(NoteBase & note)
{
  restore_matches_window();
  add_note(note);
}

void SearchNotesWidget::on_note_renamed(const NoteBase & note,
                                        const Glib::ustring &)
{
  restore_matches_window();
  rename_note(note);
}

void SearchNotesWidget::on_note_saved(NoteBase&)
{
  restore_matches_window();
  update_results();
}

void SearchNotesWidget::delete_note(const NoteBase & note)
{
  Gtk::TreeModel::Children rows = m_store->children();

  for(Gtk::TreeModel::iterator iter = rows.begin();
      rows.end() != iter; iter++) {
    if(&note == iter->get_value(m_column_types.note).get()) {
      m_store->erase(iter);
      break;
    }
  }
}

void SearchNotesWidget::add_note(NoteBase & note)
{
  Glib::ustring nice_date = utils::get_pretty_print_date(note.change_date(), true, m_gnote.preferences());
  Gtk::TreeIter iter = m_store->append();
  iter->set_value(m_column_types.title, note.get_title());
  iter->set_value(m_column_types.change_date, nice_date);
  iter->set_value(m_column_types.note, std::static_pointer_cast<Note>(note.shared_from_this()));
}

void SearchNotesWidget::rename_note(const NoteBase & note)
{
  Gtk::TreeModel::Children rows = m_store->children();

  for(Gtk::TreeModel::iterator iter = rows.begin();
      rows.end() != iter; iter++) {
    if(&note == iter->get_value(m_column_types.note).get()) {
      iter->set_value(m_column_types.title, note.get_title());
      break;
    }
  }
}

void SearchNotesWidget::on_open_note()
{
  Note::List selected_notes = get_selected_notes ();
  Note::List::iterator iter = selected_notes.begin();
  if(iter != selected_notes.end()) {
    signal_open_note(**iter);
    ++iter;
  }
  for(; iter != selected_notes.end(); ++iter) {
    signal_open_note_new_window(**iter);
  }
}

void SearchNotesWidget::on_open_note_new_window()
{
  Note::List selected_notes = get_selected_notes();
  for(Note::List::iterator iter = selected_notes.begin();
      iter != selected_notes.end(); ++iter) {
    signal_open_note_new_window(**iter);
  }
}

void SearchNotesWidget::delete_selected_notes()
{
  auto owning = get_owning_window();
  if(!owning) {
    return;
  }
  auto & owning_window = *owning;

  std::vector<NoteBase::Ref> selected_notes;
  for(const auto & note : get_selected_notes()) {
    selected_notes.push_back(*note);
  }
  if(selected_notes.empty()) {
    return;
  }

  noteutils::show_deletion_dialog(selected_notes, owning_window);
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

Gtk::Popover *SearchNotesWidget::get_note_list_context_menu()
{
  if(!m_note_list_context_menu) {
    auto menu = Gio::Menu::create();

    menu->append(_("_New"), "app.new-note");
    menu->append(_("_Open"), "win.open-note");
    menu->append(_("Open In New _Window"), "win.open-note-new-window");
    menu->append(_("_Delete"), "win.delete-selected-notes");

    m_note_list_context_menu = utils::make_owned_popover<Gtk::PopoverMenu>(*m_tree, menu);
  }
  else {
    m_note_list_context_menu->set_parent(*m_tree);
  }

  on_selection_changed();
  return m_note_list_context_menu.get();
}

void SearchNotesWidget::new_note()
{
  auto notebook = m_notebooks_view->get_selected_notebook();
  auto & note = (!notebook || std::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook))
    // Just create a standard note (not in a notebook)
    ? static_cast<Note&>(m_manager.create())
    // Look for the template note and create a new note
    : notebook->create_notebook_note();

  signal_open_note(note);
}

Gtk::Popover *SearchNotesWidget::get_notebook_list_context_menu()
{
  if(!m_notebook_list_context_menu) {
    auto menu = Gio::Menu::create();
    menu->append(_("_New..."), "win.new-notebook");
    menu->append(_("_Open Template Note"), "win.open-template-note");
    menu->append(_("Re_name..."), "win.rename-notebook");
    menu->append(_("_Delete"), "win.delete-notebook");
    m_notebook_list_context_menu = utils::make_owned_popover<Gtk::PopoverMenu>(*m_notebooks_view, menu);
  }
  else {
    m_notebook_list_context_menu->set_parent(*m_notebooks_view);
  }

  on_notebook_selection_changed(m_notebooks_view->get_selected_notebook());
  return m_notebook_list_context_menu.get();
}

void SearchNotesWidget::on_open_notebook_template_note(const Glib::VariantBase&)
{
  auto notebook = m_notebooks_view->get_selected_notebook();
  if(!notebook) {
    return;
  }

  auto & template_note = notebook->get_template_note();
  signal_open_note(template_note);
}

void SearchNotesWidget::on_new_notebook(const Glib::VariantBase&)
{
  notebooks::NotebookManager::prompt_create_new_notebook(m_gnote, *get_owning_window());
}

void SearchNotesWidget::on_delete_notebook(const Glib::VariantBase&)
{
  auto notebook = m_notebooks_view->get_selected_notebook();
  if(!notebook) {
    return;
  }

  notebooks::NotebookManager::prompt_delete_notebook(m_gnote, get_owning_window(), notebook);
}

void SearchNotesWidget::embed(EmbeddableWidgetHost *h)
{
  EmbeddableWidget::embed(h);
  if(auto win = dynamic_cast<MainWindow*>(host())) {
    if(auto action = win->find_action("open-note")) {
      action->signal_activate().connect([this](const Glib::VariantBase&) { on_open_note(); });
    }
    if(auto action = win->find_action("open-note-new-window")) {
      action->signal_activate().connect([this](const Glib::VariantBase&) { on_open_note_new_window(); });
    }
    if(auto action = win->find_action("delete-selected-notes")) {
      action->signal_activate().connect([this](const Glib::VariantBase&) { delete_selected_notes(); });
    }
    if(auto action = win->find_action("new-notebook")) {
      action->signal_activate().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_new_notebook));
    }
    if(auto action = win->find_action("rename-notebook")) {
      action->signal_activate().connect([this](const Glib::VariantBase&) { on_rename_notebook(); });
    }
    if(auto action = win->find_action("delete-notebook")) {
      action->signal_activate().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_delete_notebook));
    }
    if(auto action = win->find_action("open-template-note")) {
      action->signal_activate().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_open_notebook_template_note));
    }
  }
}

void SearchNotesWidget::background()
{
  EmbeddableWidget::background();
  save_position();
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
    case RecentNotesColumnTypes::TITLE:
      value = "note:";
      break;
    case RecentNotesColumnTypes::CHANGE_DATE:
      value = "change:";
      break;
    default:
      return;
    }
    if(m_sort_column_order == Gtk::SortType::ASCENDING) {
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
    column_id = RecentNotesColumnTypes::TITLE;
  }
  else if(tokens[0] == "change") {
    column_id = RecentNotesColumnTypes::CHANGE_DATE;
  }
  else {
    ERR_OUT(_("Failed to parse setting search-sorting (Value: %s):"), sorting.c_str());
    ERR_OUT(_("Unrecognized column %s"), tokens[0].c_str());
    return;
  }
  if(tokens[1] == "asc") {
    order = Gtk::SortType::ASCENDING;
  }
  else if(tokens[1] == "desc") {
    order = Gtk::SortType::DESCENDING;
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
  Glib::RefPtr<Gtk::TreeSelection> selection = m_notebooks_view->get_selection();
  if(!selection) {
    return;
  }
  std::vector<Gtk::TreeModel::Path> selected_row = selection->get_selected_rows();
  if(selected_row.size() != 1) {
    return;
  }
  m_notebooks_view->set_cursor(selected_row[0], *m_notebooks_view->get_column(0), true);
}

}
