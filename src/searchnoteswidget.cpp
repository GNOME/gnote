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


#include <glibmm/i18n.h>
#include <giomm/liststore.h>
#include <gtkmm/boolfilter.h>
#include <gtkmm/columnviewsorter.h>
#include <gtkmm/dragsource.h>
#include <gtkmm/expression.h>
#include <gtkmm/filterlistmodel.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/linkbutton.h>
#include <gtkmm/multiselection.h>
#include <gtkmm/numericsorter.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/shortcutcontroller.h>
#include <gtkmm/sortlistmodel.h>
#include <gtkmm/stringsorter.h>

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
#include "notebooks/notebooknamepopover.hpp"
#include "notebooks/specialnotebooks.hpp"
#include "sharp/string.hpp"


namespace gnote {

namespace {

bool get_column_view_sort(const Gtk::ColumnView & view, Glib::RefPtr<const Gtk::ColumnViewColumn> & sort_column, Gtk::SortType & sort_column_order)
{
  if(auto sorter = std::dynamic_pointer_cast<const Gtk::ColumnViewSorter>(view.get_sorter())) {
    if(auto column = sorter->get_primary_sort_column()) {
      sort_column = column;
      sort_column_order = sorter->get_primary_sort_order();
      return true;
    }
  }

  return false;
}

class NoteBox
  : public Gtk::Box
{
public:
  Gtk::Label title;
  sigc::connection on_note_renamed_cid;
};

class NoteColumnItemFactory
  : public Gtk::SignalListItemFactory
{
public:
  static Glib::RefPtr<NoteColumnItemFactory> create()
    {
      return Glib::make_refptr_for_instance(new NoteColumnItemFactory);
    }
private:
  static const Glib::ustring TITLE_WIDGET;
  NoteColumnItemFactory()
    {
      signal_setup().connect(sigc::mem_fun(*this, &NoteColumnItemFactory::setup));
      signal_bind().connect(sigc::mem_fun(*this, &NoteColumnItemFactory::bind));
      signal_unbind().connect(sigc::mem_fun(*this, &NoteColumnItemFactory::unbind));
    }

  void setup(const Glib::RefPtr<Gtk::ListItem> & list_item)
    {
      auto box = Gtk::make_managed<NoteBox>();
      auto image = Gtk::make_managed<Gtk::Image>();
      image->property_icon_name() = IconManager::NOTE;
      image->set_margin_end(3);
      box->append(*image);
      box->title.set_name(TITLE_WIDGET);
      box->title.set_xalign(0.0f);
      box->title.set_size_request(150);
      box->title.set_ellipsize(Pango::EllipsizeMode::END);
      box->append(box->title);
      list_item->set_child(*box);
    }

  void bind(const Glib::RefPtr<Gtk::ListItem> & list_item)
    {
      if(auto widget = dynamic_cast<NoteBox*>(list_item->get_child())) {
        if(auto note = std::dynamic_pointer_cast<Note>(list_item->get_item())) {
          widget->title.set_label(note->get_title());
          widget->on_note_renamed_cid = note->signal_renamed.connect([widget](const NoteBase &note, const Glib::ustring&) {
            widget->title.set_label(note.get_title());
          });
        }
      }
    }

  void unbind(const Glib::RefPtr<Gtk::ListItem> & list_item)
    {
      if(auto widget = dynamic_cast<NoteBox*>(list_item->get_child())) {
        widget->on_note_renamed_cid.disconnect();
      }
    }
};

const Glib::ustring NoteColumnItemFactory::TITLE_WIDGET = "note-title";


class NoteChangeLabel
  : public Gtk::Label
{
public:
  explicit NoteChangeLabel(Preferences &prefs)
    : m_preferences(prefs)
    {
    }

  void set_text(NoteBase &note)
    {
      Gtk::Label::set_text(utils::get_pretty_print_date(note.change_date(), true, m_preferences));
    }

  void on_note_saved(NoteBase &note)
    {
      set_text(note);
    }

  sigc::connection saved_cid;
private:
  Preferences &m_preferences;
};

class ChangeColumnFactory
  : public Gtk::SignalListItemFactory
{
public:
  static Glib::RefPtr<ChangeColumnFactory> create(Preferences & preferences)
    {
      return Glib::make_refptr_for_instance(new ChangeColumnFactory(preferences));
    }
private:
  ChangeColumnFactory(Preferences & prefs)
    : m_preferences(prefs)
    {
      signal_setup().connect(sigc::mem_fun(*this, &ChangeColumnFactory::setup));
      signal_bind().connect(sigc::mem_fun(*this, &ChangeColumnFactory::bind));
      signal_unbind().connect(sigc::mem_fun(*this, &ChangeColumnFactory::unbind));
    }

  void setup(const Glib::RefPtr<Gtk::ListItem> & list_item)
    {
      auto label = Gtk::make_managed<NoteChangeLabel>(m_preferences);
      label->set_halign(Gtk::Align::START);
      list_item->set_child(*label);
    }

  void bind(const Glib::RefPtr<Gtk::ListItem> & list_item)
    {
      auto &label = get_label(*list_item);
      auto &note = get_note(*list_item);
      label.set_text(note);
      label.saved_cid = note.signal_saved.connect(sigc::mem_fun(label, &NoteChangeLabel::on_note_saved));
    }

  void unbind(const Glib::RefPtr<Gtk::ListItem> & list_item)
    {
      get_label(*list_item).saved_cid.disconnect();
    }

  NoteChangeLabel &get_label(Gtk::ListItem &item)
    {
      if(auto label = dynamic_cast<NoteChangeLabel*>(item.get_child())) {
        return *label;
      }

      throw std::runtime_error("Expected child to be label");
    }

  Note &get_note(Gtk::ListItem &item)
    {
      if(auto note = std::dynamic_pointer_cast<Note>(item.get_item())) {
        return *note;
      }

      throw std::runtime_error("Expected to get Note");
    }

  Preferences & m_preferences;
};

class NoteFilterModel
  : public Gtk::FilterListModel
{
public:
  static Glib::RefPtr<NoteFilterModel> create(const Glib::RefPtr<Gio::ListModel> & model)
    {
      return Glib::make_refptr_for_instance(new NoteFilterModel(model));
    }

  void set_selected_notebook(const notebooks::Notebook::Ptr & notebook)
    {
      m_selected_notebook = notebook;
      gtk_filter_changed(get_filter()->gobj(), GTK_FILTER_CHANGE_DIFFERENT);
    }

  void set_matches(std::map<Glib::ustring, unsigned> && matches)
    {
      m_current_matches = std::move(matches);
      m_searching = true;
      gtk_filter_changed(get_filter()->gobj(), GTK_FILTER_CHANGE_DIFFERENT);
    }

  void clear_matches()
    {
      m_current_matches.clear();
      m_searching = false;
      gtk_filter_changed(get_filter()->gobj(), GTK_FILTER_CHANGE_LESS_STRICT);
    }

  unsigned matches_for(const Glib::ustring & uri) const
    {
      auto iter = m_current_matches.find(uri);
      return (iter == m_current_matches.end()) ? 0 : iter->second;
    }

private:
  explicit NoteFilterModel(const Glib::RefPtr<Gio::ListModel> & model)
    : Gtk::FilterListModel(model, {})
    , m_searching(false)
    {
      auto expression = Gtk::ClosureExpression<bool>::create(sigc::mem_fun(*this, &NoteFilterModel::filter));
      set_filter(Gtk::BoolFilter::create(std::move(expression)));
    }

  bool filter(const Glib::RefPtr<Glib::ObjectBase> & obj) const
    {
      if(!m_selected_notebook) {
        return false;
      }
      auto note = std::dynamic_pointer_cast<Note>(obj);
      if(!note) {
        return false;
      }
      if(!m_selected_notebook->contains_note(*note)) {
        return false;
      }
      if(!m_searching) {
        return true;
      }

      return m_current_matches.find(note->uri()) != m_current_matches.end();
    }

  notebooks::Notebook::Ptr m_selected_notebook;
  std::map<Glib::ustring, unsigned> m_current_matches;
  bool m_searching;
};

class MatchesColumnFactory
  : public utils::LabelFactory
{
public:
  static Glib::RefPtr<MatchesColumnFactory> create(const Glib::RefPtr<NoteFilterModel> & filter)
    {
      return Glib::make_refptr_for_instance(new MatchesColumnFactory(filter));
    }
private:
  explicit MatchesColumnFactory(const Glib::RefPtr<NoteFilterModel> & filter)
    : m_filter(filter)
    {}

  Glib::ustring get_text(Gtk::ListItem & item) override
    {
      if(auto note = std::dynamic_pointer_cast<Note>(item.get_item())) {
        auto matches = m_filter->matches_for(note->uri());
        if(matches == INT_MAX) {
          //TRANSLATORS: search found a match in note title
          return _("Title match");
        }
        else if(matches > 0) {
          const char * fmt;
          fmt = ngettext("%1 match", "%1 matches", matches);
          return Glib::ustring::compose(fmt, matches);
        }
      }
      return Glib::ustring();
    }

  Glib::RefPtr<NoteFilterModel> m_filter;
};

}


SearchNotesWidget::SearchNotesWidget(IGnote & g, NoteManagerBase & m)
  : m_notes_pane(Gtk::Orientation::VERTICAL)
  , m_gnote(g)
  , m_manager(m)
  , m_clickX(0), m_clickY(0)
  , m_matches_column(NULL)
  , m_initial_position_restored(false)
  , m_sort_column_order(Gtk::SortType::DESCENDING)
{
  set_hexpand(true);
  set_vexpand(true);

  // Notebooks Pane
  Gtk::Widget *notebooksPane = make_notebooks_pane();

  set_position(150);
  set_start_child(*notebooksPane);
  m_notes_pane.append(m_matches_window);
  auto actions = Gtk::make_managed<Gtk::Box>();
  auto new_note_button = Gtk::make_managed<Gtk::Button>();
  new_note_button->set_icon_name("list-add-symbolic");
  new_note_button->set_tooltip_text(_("New Note"));
  new_note_button->set_has_frame(false);
  new_note_button->signal_clicked().connect(sigc::mem_fun(*this, &SearchNotesWidget::new_note));
  m_delete_note_button.set_icon_name("edit-delete-symbolic");
  m_delete_note_button.set_tooltip_text(_("Delete Note"));
  m_delete_note_button.set_has_frame(false);
  m_delete_note_button.signal_clicked().connect(sigc::mem_fun(*this, &SearchNotesWidget::delete_selected_notes));
  actions->append(*new_note_button);
  actions->append(m_delete_note_button);
  m_notes_pane.append(*actions);
  set_end_child(m_notes_pane);

  make_recent_notes_view();

  m_matches_window.property_hscrollbar_policy() = Gtk::PolicyType::AUTOMATIC;
  m_matches_window.property_vscrollbar_policy() = Gtk::PolicyType::AUTOMATIC;
  m_matches_window.set_child(*m_notes_view);
  m_matches_window.set_expand(true);


  // Update on changes to notes
  m.signal_note_deleted.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_deleted));
  m.signal_note_added.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_added));
  m.signal_note_renamed.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_renamed));

  // Watch when notes are added to notebooks so the search
  // results will be updated immediately instead of waiting
  // until the note's queue_save () kicks in.
  notebooks::NotebookManager & notebook_manager = g.notebook_manager();
  notebook_manager.signal_note_added_to_notebook
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_added_to_notebook));
  notebook_manager.signal_note_removed_from_notebook
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_removed_from_notebook));
  notebook_manager.signal_note_pin_status_changed
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_note_pin_status_changed));

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
  if(auto selected_notebook = m_notebooks_view->get_selected_notebook()) {
    return selected_notebook.value().get().get_name();
  }

  return "";
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
  NoteFilterModel & store_filter = *std::static_pointer_cast<NoteFilterModel>(m_store_filter);
  auto selected_notebook = m_notebooks_view->get_selected_notebook();
  if(selected_notebook) {
    store_filter.set_selected_notebook(selected_notebook.value().get().shared_from_this());
  }
  else {
    store_filter.set_selected_notebook(notebooks::Notebook::Ptr());
  }

  Glib::ustring text = m_search_text;
  if(text.empty()) {
    store_filter.clear_matches();
    return;
  }
  text = text.lowercase();

  store_filter.clear_matches();

  // Search using the currently selected notebook
  if(dynamic_cast<notebooks::SpecialNotebook*>(&selected_notebook.value().get())) {
    selected_notebook = notebooks::Notebook::ORef();
  }

  auto results = search.search_notes(text, false, selected_notebook);
  // if no results found in current notebook ask user whether
  // to search in all notebooks
  if(results.size() == 0 && selected_notebook) {
    no_matches_found_action();
  }
  else {
    std::map<Glib::ustring, unsigned> current_matches;
    for(auto iter = results.rbegin(); iter != results.rend(); ++iter) {
      current_matches[iter->second.get().uri()] = iter->first;
    }

    store_filter.set_matches(std::move(current_matches));
    add_matches_column();
  }
}

void SearchNotesWidget::restore_matches_window()
{
  if(m_no_matches_box && get_end_child() == m_no_matches_box.get()) {
    set_end_child(m_notes_pane);
  }
}

Gtk::Widget *SearchNotesWidget::make_notebooks_pane()
{
  auto& notebook_manager = m_gnote.notebook_manager();
  auto model = Gio::ListStore<notebooks::Notebook>::create();
  notebook_manager.get_notebooks([&model](const notebooks::Notebook::Ptr& nb) { model->append(nb); }, true);
  m_notebooks_view = Gtk::make_managed<notebooks::NotebooksView>(m_manager, model);

  m_notebooks_view->signal_selected_notebook_changed
    .connect(sigc::mem_fun(*this, &SearchNotesWidget::on_notebook_selection_changed));
  m_notebooks_view->signal_open_template_note.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_open_notebook_template_note));
  notebook_manager.signal_notebook_list_changed.connect(sigc::mem_fun(*this, &SearchNotesWidget::on_notebook_list_changed));

  return m_notebooks_view;
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

void SearchNotesWidget::on_notebook_selection_changed(const notebooks::Notebook & notebook)
{
  restore_matches_window();
  update_results();
  signal_name_changed(get_name());
}

void SearchNotesWidget::on_notebook_list_changed()
{
  auto model = Gio::ListStore<notebooks::Notebook>::create();
  m_gnote.notebook_manager().get_notebooks([&model](const notebooks::Notebook::Ptr& nb) { model->append(nb); }, true);
  m_notebooks_view->set_notebooks(model);
}

void SearchNotesWidget::select_all_notes_notebook()
{
  m_notebooks_view->select_all_notes_notebook();
}

void SearchNotesWidget::update_results()
{
  // Save the currently selected notes
  std::vector<Note::Ref> selected_notes;
  if(m_store) {
    selected_notes = get_selected_notes();
  }

  auto selection_model = std::static_pointer_cast<Gtk::MultiSelection>(m_notes_view->get_model());
  selection_model->unselect_all();

  perform_search();

  // Restore the previous selection
  if(!selected_notes.empty()) {
    select_notes(selected_notes);
  }
}

unsigned SearchNotesWidget::selected_note_count() const
{
  return m_notes_view->get_model()->get_selection()->get_size();
}

std::vector<Note::Ref> SearchNotesWidget::get_selected_notes()
{
  std::vector<Note::Ref> selected_notes;

  auto selection = std::static_pointer_cast<Gtk::MultiSelection>(m_notes_view->get_model());
  const unsigned num_items = selection->get_n_items();
  for(unsigned i = 0; i < num_items; ++i) {
    if(selection->is_selected(i)) {
      if(auto note = std::dynamic_pointer_cast<Note>(selection->get_object(i))) {
        selected_notes.push_back(*note);
      }
    }
  }

  return selected_notes;
}

void SearchNotesWidget::make_recent_notes_view()
{
  m_notes_view = Gtk::make_managed<Gtk::ColumnView>();
  m_notes_view->set_reorderable(false);
  m_notes_view->signal_activate().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_row_activated));

  m_notes_widget_key_ctrl = Gtk::EventControllerKey::create();
  m_notes_widget_key_ctrl->signal_key_pressed().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_treeview_key_pressed), false);
  m_notes_view->add_controller(m_notes_widget_key_ctrl);

  auto drag_source = Gtk::DragSource::create();
  auto paintable = Gtk::IconTheme::get_for_display(Gdk::Display::get_default())->lookup_icon(IconManager::NOTE, 24);
  drag_source->set_icon(paintable, 0, 0);
  drag_source->signal_prepare().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_treeview_drag_data_get), false);
  m_notes_view->add_controller(drag_source);

  m_title_column = Gtk::ColumnViewColumn::create(_("Note"), NoteColumnItemFactory::create());
  m_title_column->set_expand(true);
  m_title_column->set_resizable(true);
  m_title_column->set_sorter(Gtk::StringSorter::create(Gtk::ClosureExpression<Glib::ustring>::create([](const Glib::RefPtr<Glib::ObjectBase> & item) {
    if(auto note = std::dynamic_pointer_cast<Note>(item)) {
      return note->get_title();
    }
    return Glib::ustring();
  })));

  m_notes_view->append_column(m_title_column);

  m_change_column = Gtk::ColumnViewColumn::create(_("Modified"), ChangeColumnFactory::create(m_gnote.preferences()));
  m_change_column->set_resizable(false);
  m_change_column->set_sorter(Gtk::NumericSorter<guint64>::create(Gtk::ClosureExpression<guint64>::create([](const Glib::RefPtr<Glib::ObjectBase> & item) -> guint64 {
    if(auto note = std::dynamic_pointer_cast<Note>(item)) {
      return note->change_date().to_unix();
    }
    return 0;
  })));

  m_notes_view->append_column(m_change_column);

  parse_sorting_setting(m_gnote.preferences().search_sorting());
  if(!m_sort_column) {
    m_sort_column = m_change_column;
    m_sort_column_order = Gtk::SortType::DESCENDING;
  }

  auto store = Gio::ListStore<Note>::create();
  m_manager.copy_to(store, [this](const Glib::RefPtr<Gio::ListStore<Note>> & store, const NoteBase::Ptr & note_base) {
    store->append(std::static_pointer_cast<Note>(note_base));
  });
  m_store = store;
  m_store_filter = NoteFilterModel::create(m_store);
  m_store_sort = Gtk::SortListModel::create(m_store_filter, m_notes_view->get_sorter());
  auto selection = Gtk::MultiSelection::create(m_store_sort);
  m_notes_view->set_model(selection);
  m_notes_view->sort_by_column(std::const_pointer_cast<Gtk::ColumnViewColumn>(m_sort_column), m_sort_column_order);
  m_notes_view->get_sorter()->signal_changed().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_sorting_changed));
  selection->signal_selection_changed().connect(sigc::mem_fun(*this, &SearchNotesWidget::on_selection_changed));
  utils::timeout_add_once(1, [store, selection]() {
    if(store->get_n_items() > 0) {
      selection->select_item(0, false);
    }
  });
}

void SearchNotesWidget::select_notes(const std::vector<Note::Ref> & notes)
{
  auto selection = std::static_pointer_cast<Gtk::MultiSelection>(m_notes_view->get_model());
  auto model = selection->get_model();
  auto n_items = model->get_n_items();

  for(unsigned i = 0; i < n_items; ++i) {
    Note::Ptr iter_note = std::dynamic_pointer_cast<Note>(model->get_object(i));
    if(find_if(notes.begin(), notes.end(), [iter_note](const Note::Ref & note) { return &note.get() == iter_note.get(); }) != notes.end()) {
      // Found one
      selection->select_item(i, false);
    }
  }
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

void SearchNotesWidget::on_row_activated(guint idx)
{
  auto selected_notes = get_selected_notes();
  if(selected_notes.size() > 0) {
    on_open_note(OpenNoteMode::CURRENT_WINDOW);
  }
  else if(auto note = std::dynamic_pointer_cast<Note>(m_store_sort->get_object(idx))) {
    signal_open_note(*note);
  }
}

void SearchNotesWidget::on_selection_changed(guint, guint)
{
  if(auto win = dynamic_cast<MainWindow*>(host())) {
    bool enabled = selected_note_count();
    if(auto action = win->find_action("open-note")) {
      action->set_enabled(enabled);
    }
    if(auto action = win->find_action("open-note-new-window")) {
      action->set_enabled(enabled);
    }
    if(auto action = win->find_action("delete-selected-notes")) {
      action->set_enabled(enabled);
      m_delete_note_button.set_sensitive(enabled);
    }
  }
}

bool SearchNotesWidget::on_treeview_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state)
{
  switch(keyval) {
  case GDK_KEY_Delete:
    delete_selected_notes();
    break;
  case GDK_KEY_Return:
  case GDK_KEY_KP_Enter:
    // Open all selected notes
    {
      OpenNoteMode mode = OpenNoteMode::CURRENT_WINDOW;
      if(Gdk::ModifierType::CONTROL_MASK == (state & Gdk::ModifierType::CONTROL_MASK)) {
        if(Gdk::ModifierType::SHIFT_MASK == (state & Gdk::ModifierType::SHIFT_MASK)) {
          mode = OpenNoteMode::NEW_WINDOW ;
        }
        else {
          mode = OpenNoteMode::SINGLE_NEW_WINDOW;
        }
      }
      on_open_note(mode);
      return true;
    }
  default:
    break;
  }

  return false; // Let Escape be handled by the window.
}

Glib::RefPtr<Gdk::ContentProvider> SearchNotesWidget::on_treeview_drag_data_get(double, double)
{
  auto selected_notes = get_selected_notes();
  if(selected_notes.empty()) {
    // TODO grab note under cursor
    return Glib::RefPtr<Gdk::ContentProvider>();
  }

  if(selected_notes.size() == 1) {
    Glib::Value<Glib::ustring> value;
    value.init(Glib::Value<Glib::ustring>::value_type());
    value.set(selected_notes.front().get().uri());
    return Gdk::ContentProvider::create(value);
  }
  else {
    std::vector<Glib::ustring> uris;
    for(const auto & note : selected_notes) {
      uris.emplace_back(note.get().uri());
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
  m_notes_view->sort_by_column(std::const_pointer_cast<Gtk::ColumnViewColumn>(m_sort_column), m_sort_column_order);
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
    m_matches_column = Gtk::ColumnViewColumn::create(_("Matches"), MatchesColumnFactory::create(std::static_pointer_cast<NoteFilterModel>(m_store_filter)));
    m_matches_column->set_resizable(false);

    m_matches_column->set_sorter(Gtk::NumericSorter<unsigned>::create(
      Gtk::ClosureExpression<unsigned>::create([this](const Glib::RefPtr<Glib::ObjectBase> & item) -> unsigned {
        if(auto note = std::dynamic_pointer_cast<NoteBase>(item)) {
          if(auto store_filter = std::dynamic_pointer_cast<NoteFilterModel>(m_store_filter)) {
            return store_filter->matches_for(note->uri());
          }
        }
        return 0;
      })
    ));

    m_notes_view->append_column(m_matches_column);
  }
  else {
    m_matches_column->set_visible(true);
  }

  m_notes_view->sort_by_column(m_matches_column, Gtk::SortType::DESCENDING);
}

bool SearchNotesWidget::show_all_search_results()
{
  select_all_notes_notebook();
  return true;
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
}

void SearchNotesWidget::delete_note(NoteBase & note)
{
  auto store = std::static_pointer_cast<Gio::ListStore<NoteBase>>(m_store);
  auto item = store->find(note.shared_from_this());

  if(item.first) {
    store->remove(item.second);
  }
}

void SearchNotesWidget::add_note(NoteBase & note)
{
  auto store = std::static_pointer_cast<Gio::ListStore<NoteBase>>(m_store);
  store->append(note.shared_from_this());
}

void SearchNotesWidget::on_open_note(OpenNoteMode mode)
{
  auto selected_notes = get_selected_notes();
  if(selected_notes.size() == 0) {
    return;
  }

  switch(mode) {
  case OpenNoteMode::CURRENT_WINDOW:
    for(auto &note : selected_notes) {
      signal_open_note(note);
    }
    break;
  case OpenNoteMode::NEW_WINDOW:
    for(auto &note : selected_notes) {
      signal_open_note_new_window(note);
    }
    break;
  case OpenNoteMode::SINGLE_NEW_WINDOW:
    {
      auto iter = selected_notes.begin();
      signal_open_note_new_window(*iter);
      if(selected_notes.size() > 1) {
        if(auto host = dynamic_cast<MainWindow*>(iter->get().get_window()->host())) {
          for(++iter; iter != selected_notes.end(); ++iter) {
            MainWindow::present_in(*host, *iter);
          }
        }
      }
    }
  }
}

void SearchNotesWidget::on_open_note_new_window()
{
  auto selected_notes = get_selected_notes();
  for(auto note : selected_notes) {
    signal_open_note_new_window(note);
  }
}

void SearchNotesWidget::delete_selected_notes()
{
  auto owning = get_owning_window();
  if(!owning) {
    return;
  }
  auto & owning_window = *owning;

  auto selected_notes = get_selected_notes();
  if(selected_notes.empty()) {
    return;
  }

  noteutils::show_deletion_dialog(std::vector<NoteBase::Ref>(selected_notes.begin(), selected_notes.end()), owning_window);
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

void SearchNotesWidget::on_note_added_to_notebook(const Note &, const notebooks::Notebook &)
{
  restore_matches_window();
  update_results();
}

void SearchNotesWidget::on_note_removed_from_notebook(const Note &, const notebooks::Notebook &)
{
  restore_matches_window();
  update_results();
}

void SearchNotesWidget::on_note_pin_status_changed(const Note &, bool)
{
  restore_matches_window();
  update_results();
}

void SearchNotesWidget::new_note()
{
  auto notebook = m_notebooks_view->get_selected_notebook();
  auto & note = (!notebook || dynamic_cast<notebooks::SpecialNotebook*>(&notebook.value().get()))
    // Just create a standard note (not in a notebook)
    ? static_cast<Note&>(m_manager.create())
    // Look for the template note and create a new note
    : notebook.value().get().create_notebook_note();

  signal_open_note(note);
}

void SearchNotesWidget::on_open_notebook_template_note(Note& note)
{
  signal_open_note(note);
}

void SearchNotesWidget::embed(EmbeddableWidgetHost *h)
{
  EmbeddableWidget::embed(h);
  if(auto win = dynamic_cast<MainWindow*>(host())) {
    if(auto action = win->find_action("open-note")) {
      action->signal_activate().connect([this](const Glib::VariantBase&) { on_open_note(OpenNoteMode::CURRENT_WINDOW); });
    }
    if(auto action = win->find_action("open-note-new-window")) {
      action->signal_activate().connect([this](const Glib::VariantBase&) { on_open_note_new_window(); });
    }
    if(auto action = win->find_action("delete-selected-notes")) {
      action->signal_activate().connect([this](const Glib::VariantBase&) { delete_selected_notes(); });
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
    win->set_focus(*m_notes_view);
  }
}

void SearchNotesWidget::on_sorting_changed(Gtk::Sorter::Change)
{
  // don't do anything if in search mode
  if(m_matches_column && m_matches_column->get_visible()) {
    return;
  }

  if(!get_column_view_sort(*m_notes_view, m_sort_column, m_sort_column_order)) {
    return;
  }

  Glib::ustring value;
  if(m_sort_column == m_title_column) {
    value = "note:";
  }
  else if(m_sort_column == m_change_column) {
    value = "change:";
  }
  else {
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

void SearchNotesWidget::parse_sorting_setting(const Glib::ustring & sorting)
{
  std::vector<Glib::ustring> tokens;
  sharp::string_split(tokens, sorting.lowercase(), ":");
  if(tokens.size() != 2) {
    ERR_OUT(_("Failed to parse setting search-sorting (Value: %s):"), sorting.c_str());
    ERR_OUT(_("Expected format 'column:order'"));
    return;
  }
  Glib::RefPtr<Gtk::ColumnViewColumn> column;
  Gtk::SortType order;
  if(tokens[0] == "note") {
    column = m_title_column;
  }
  else if(tokens[0] == "change") {
    column = m_change_column;
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

  m_sort_column = column;
  m_sort_column_order = order;
}

}
