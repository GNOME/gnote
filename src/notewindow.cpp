/*
 * gnote
 *
 * Copyright (C) 2011-2017 Aurimas Cernius
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/i18n.h>
#include <gtkmm/grid.h>
#include <gtkmm/image.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/stock.h>
#include <gtkmm/separator.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/separatormenuitem.h>

#include "debug.hpp"
#include "addinmanager.hpp"
#include "iconmanager.hpp"
#include "mainwindow.hpp"
#include "note.hpp"
#include "notewindow.hpp"
#include "notemanager.hpp"
#include "noteeditor.hpp"
#include "preferences.hpp"
#include "utils.hpp"
#include "undo.hpp"
#include "search.hpp"
#include "itagmanager.hpp"
#include "notebooks/notebookmanager.hpp"
#include "sharp/exception.hpp"
#include "mainwindowaction.hpp"


namespace gnote {

  Glib::RefPtr<Gio::Icon> NoteWindow::get_icon_pin_active()
  {
    return IconManager::obj().get_icon(IconManager::PIN_ACTIVE, 22);
  }

  Glib::RefPtr<Gio::Icon> NoteWindow::get_icon_pin_down()
  {
    return IconManager::obj().get_icon(IconManager::PIN_DOWN, 22);
  }


  NoteWindow::NonModifyingAction::NonModifyingAction()
  {}

  NoteWindow::NonModifyingAction::NonModifyingAction(const Glib::ustring & name,
                                                     const Gtk::StockID & stock_id,
                                                     const Glib::ustring & label,
                                                     const Glib::ustring & tooltip)
    : Gtk::Action(name, stock_id, label, tooltip)
  {}

  NoteWindow::NonModifyingAction::NonModifyingAction(const Glib::ustring & name,
                                                     const Glib::ustring & icon_name,
                                                     const Glib::ustring & label,
                                                     const Glib::ustring & tooltip)
    : Gtk::Action(name, icon_name, label, tooltip)
  {}

  void NoteWindow::NonModifyingAction::reference() const
  {
    Gtk::Action::reference();
  }

  void NoteWindow::NonModifyingAction::unreference() const
  {
    Gtk::Action::unreference();
  }



  NoteWindow::NoteWindow(Note & note)
    : m_note(note)
    , m_name(note.get_title())
    , m_height(450)
    , m_width(600)
    , m_find_handler(note)
    , m_global_keys(NULL)
    , m_enabled(true)
  {
    m_template_tag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
    m_template_save_size_tag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SAVE_SIZE_SYSTEM_TAG);
    m_template_save_selection_tag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SAVE_SELECTION_SYSTEM_TAG);
    m_template_save_title_tag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SAVE_TITLE_SYSTEM_TAG);

    set_hexpand(true);
    set_vexpand(true);

    m_text_menu = Gtk::manage(new NoteTextMenu(*this, note.get_buffer(), note.get_buffer()->undoer()));

    m_embeddable_toolbar = manage(make_toolbar());

    m_template_widget = make_template_bar();

    // The main editor widget
    m_editor = manage(new NoteEditor(note.get_buffer()));
    m_editor->signal_populate_popup().connect(sigc::mem_fun(*this, &NoteWindow::on_populate_popup));
    m_editor->show();

    // Sensitize the Link toolbar button on text selection
    m_mark_set_timeout = new utils::InterruptableTimeout();
    m_mark_set_timeout->signal_timeout.connect(sigc::mem_fun(*m_text_menu, &NoteTextMenu::refresh_state));
    note.get_buffer()->signal_mark_set().connect(
      sigc::mem_fun(*this, &NoteWindow::on_selection_mark_set));

    // FIXME: I think it would be really nice to let the
    //        window get bigger up till it grows more than
    //        60% of the screen, and then show scrollbars.
    m_editor_window = manage(new Gtk::ScrolledWindow());
    m_editor_window->property_hscrollbar_policy().set_value(Gtk::POLICY_AUTOMATIC);
    m_editor_window->property_vscrollbar_policy().set_value(Gtk::POLICY_AUTOMATIC);
    m_editor_window->add(*m_editor);
    m_editor_window->set_hexpand(true);
    m_editor_window->set_vexpand(true);
    m_editor_window->show();

    set_focus_child(*m_editor);

    attach(*m_template_widget, 0, 0, 1, 1);
    attach(*m_editor_window, 0, 1, 1, 1);
  }


  NoteWindow::~NoteWindow()
  {
    delete m_global_keys;
    m_global_keys = NULL;
    delete m_mark_set_timeout;
    m_mark_set_timeout = NULL;
    // make sure editor is NULL. See bug 586084
    m_editor = NULL;
  }


  Glib::ustring NoteWindow::get_name() const
  {
    return m_name;
  }

  void NoteWindow::set_name(const Glib::ustring & name)
  {
    m_name = name;
    signal_name_changed(m_name);
  }

  void NoteWindow::foreground()
  {
    //addins may add accelarators, so accel group must be there
    EmbeddableWidgetHost *current_host = host();
    Gtk::Window *parent = dynamic_cast<Gtk::Window*>(current_host);
    if(parent) {
      add_accel_group(*parent);
    }

    EmbeddableWidget::foreground();
    if(parent) {
      parent->set_focus(*m_editor);
    }

    // Don't allow deleting the "Start Here" note...
    if(!m_note.is_special()) {
      m_delete_note_slot = current_host->find_action("delete-note")->signal_activate()
        .connect(sigc::mem_fun(*this, &NoteWindow::on_delete_button_clicked));
    }

    MainWindowAction::Ptr important_action = current_host->find_action("important-note");
    important_action->set_state(Glib::Variant<bool>::create(m_note.is_pinned()));
    m_important_note_slot = important_action->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteWindow::on_pin_button_clicked));
    notebooks::NotebookManager::obj().signal_note_pin_status_changed
      .connect(sigc::mem_fun(*this, &NoteWindow::on_pin_status_changed));

  }

  void NoteWindow::background()
  {
    EmbeddableWidget::background();
    Gtk::Window *parent = dynamic_cast<Gtk::Window*>(host());
    if(!parent) {
      return;
    }
    remove_accel_group(*parent);
    if(parent->get_window()
       && (parent->get_window()->get_state() & Gdk::WINDOW_STATE_MAXIMIZED) == 0) {
      int cur_width, cur_height;
      parent->get_size(cur_width, cur_height);

      if(!(m_note.data().width() == cur_width && m_note.data().height() == cur_height)) {
        m_note.data().set_extent(cur_width, cur_height);
        m_width = cur_width;
        m_height = cur_height;

        DBG_OUT("WindowConfigureEvent queueing save");
        m_note.queue_save(NO_CHANGE);
      }
    }

    m_note.save();  // to update not title immediately in notes list
    m_delete_note_slot.disconnect();
    m_important_note_slot.disconnect();
  }

  void NoteWindow::hint_size(int & width, int & height)
  {
    if (Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE)->get_boolean(
          Preferences::AUTOSIZE_NOTE_WINDOW)) {
      width = m_width;
      height = m_height;
    }
  }

  void NoteWindow::size_internals()
  {
    m_editor->scroll_to(m_editor->get_buffer()->get_insert());
  }

  void NoteWindow::add_accel_group(Gtk::Window & window)
  {
    if(!m_accel_group) {
      m_accel_group = Gtk::AccelGroup::create();
      window.add_accel_group(m_accel_group);

      if(!m_global_keys) {
        // NOTE: Since some of our keybindings are only
        // available in the context menu, and the context menu
        // is created on demand, register them with the
        // global keybinder
        m_global_keys = new utils::GlobalKeybinder(m_accel_group);

        // Open Help (F1)
        m_global_keys->add_accelerator(sigc::mem_fun(*this, &NoteWindow::open_help_activate),
                                       GDK_KEY_F1, (Gdk::ModifierType)0, (Gtk::AccelFlags)0);

        // Increase Indent
        m_global_keys->add_accelerator(sigc::mem_fun(*this, &NoteWindow::change_depth_right_handler),
                                       GDK_KEY_Right, Gdk::MOD1_MASK,
                                       Gtk::ACCEL_VISIBLE);

        // Decrease Indent
        m_global_keys->add_accelerator(sigc::mem_fun(*this, &NoteWindow::change_depth_left_handler),
                                      GDK_KEY_Left, Gdk::MOD1_MASK,
                                      Gtk::ACCEL_VISIBLE);
        m_global_keys->enabled(m_enabled);
      }

      m_text_menu->set_accels(*m_global_keys);
    }
    else {
      window.add_accel_group(m_accel_group);
    }
  }

  void NoteWindow::remove_accel_group(Gtk::Window & window)
  {
    if(m_accel_group) {
      window.remove_accel_group(m_accel_group);
    }
  }

  void NoteWindow::perform_search(const Glib::ustring & text)
  {
    get_find_handler().perform_search(text);
  }

  bool NoteWindow::supports_goto_result()
  {
    return true;
  }

  bool NoteWindow::goto_next_result()
  {
    return get_find_handler().goto_next_result();
  }

  bool NoteWindow::goto_previous_result()
  {
    return get_find_handler().goto_previous_result();
  }

  Gtk::Grid *NoteWindow::embeddable_toolbar()
  {
    return m_embeddable_toolbar;
  }

  std::vector<Gtk::Widget*> NoteWindow::get_popover_widgets()
  {
    std::vector<Gtk::Widget*> widgets;
    std::map<int, Gtk::Widget*> widget_map;

    Gtk::Widget *undo = manage(utils::create_popover_button("win.undo", _("_Undo")));
    widgets.push_back(undo);
    Gtk::Widget *redo = manage(utils::create_popover_button("win.redo", _("_Redo")));
    widgets.push_back(redo);
    widgets.push_back(NULL);

    Gtk::Widget *link = manage(utils::create_popover_button("win.link", _("_Link to New Note")));
    widget_map[2000] = link; // place under "note actions", see iactionmanger.hpp

    NoteManager & manager = static_cast<NoteManager&>(m_note.manager());
    Note::Ptr note = std::dynamic_pointer_cast<Note>(m_note.shared_from_this());
    FOREACH(NoteAddin *addin, manager.get_addin_manager().get_note_addins(note)) {
      utils::merge_ordered_maps(widget_map, addin->get_actions_popover_widgets());
    }

    int last_order = 0;
    for(std::map<int, Gtk::Widget*>::iterator iter = widget_map.begin();
        iter != widget_map.end(); ++iter) {
      // put separator between groups
      if(iter->first < 10000 && (iter->first / 1000) > last_order && widgets.size() > 0 && widgets.back() != NULL) {
        widgets.push_back(NULL);
      }
      last_order = iter->first / 1000;
      widgets.push_back(iter->second);
    }

    widgets.push_back(utils::create_popover_button("win.important-note", _("_Important")));
    widgets.push_back(utils::create_popover_button("win.delete-note", _("_Delete…")));

    return widgets;
  }

  std::vector<MainWindowAction::Ptr> NoteWindow::get_widget_actions()
  {
    std::vector<MainWindowAction::Ptr> res;
    EmbeddableWidgetHost *h = host();
    if(h != NULL) {
      h->find_action("important-note");
      h->find_action("delete-note");
    }
    return res;
  }

    // Delete this Note.
    //

  void NoteWindow::on_delete_button_clicked(const Glib::VariantBase&)
  {
    // Prompt for note deletion
    std::list<Note::Ptr> single_note_list;
    single_note_list.push_back(static_pointer_cast<Note>(m_note.shared_from_this()));
    noteutils::show_deletion_dialog(single_note_list, dynamic_cast<Gtk::Window*>(host()));
  }

  void NoteWindow::on_selection_mark_set(const Gtk::TextIter&, const Glib::RefPtr<Gtk::TextMark>&)
  {
    // FIXME: Process in a timeout due to GTK+ bug #172050.
    if(m_mark_set_timeout) {
      m_mark_set_timeout->reset(0);
    }
  }

  void NoteWindow::on_populate_popup(Gtk::Menu* menu)
  {
    menu->set_accel_group(m_accel_group);

    DBG_OUT("Populating context menu...");

    // Remove the lame-o gigantic Insert Unicode Control
    // Characters menu item.
    Gtk::Widget *lame_unicode;
    std::vector<Gtk::Widget*> children(menu->get_children());
      
    lame_unicode = *children.rbegin();
    menu->remove(*lame_unicode);

    Gtk::MenuItem *spacer1 = manage(new Gtk::SeparatorMenuItem());
    spacer1->show ();

    Gtk::ImageMenuItem *link = manage(new Gtk::ImageMenuItem(_("_Link to New Note"), true));
    link->set_image(*manage(new Gtk::Image (Gtk::Stock::JUMP_TO, Gtk::ICON_SIZE_MENU)));
    link->set_sensitive(!m_note.get_buffer()->get_selection().empty());
    link->signal_activate().connect(sigc::mem_fun(*this, &NoteWindow::link_button_clicked));
    link->add_accelerator("activate", m_accel_group, GDK_KEY_L,
                          Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    link->show();
      
    Gtk::MenuItem *spacer2 = manage(new Gtk::SeparatorMenuItem());
    spacer2->show();

    menu->prepend(*spacer1);
    menu->prepend(*link);
  }
  
  //
  // Toolbar
  //
  // Add Link button, Font menu, Delete button to the window's
  // toolbar.
  //
  Gtk::Grid *NoteWindow::make_toolbar()
  {
    Gtk::Grid *grid = manage(new Gtk::Grid);
    int grid_col = 0;

    Gtk::Button *text_button = manage(new Gtk::Button);
    Gtk::Image *image = manage(new Gtk::Image);
    image->property_icon_name() = "insert-text-symbolic";
    image->property_icon_size() = GTK_ICON_SIZE_MENU;
    text_button->set_image(*image);
    text_button->signal_clicked()
      .connect(sigc::mem_fun(*this, &NoteWindow::on_text_button_clicked));
    text_button->property_margin_left() = 12;
    text_button->show_all();
    grid->attach(*text_button, grid_col++, 0, 1, 1);
    text_button->set_tooltip_text(_("Set properties of text"));
    m_text_menu->set_relative_to(*text_button);

    grid->property_margin_left() = 12;
    grid->show_all();
    return grid;
  }


  Gtk::Grid * NoteWindow::make_template_bar()
  {
    Gtk::Grid * bar = manage(new Gtk::Grid);

    Gtk::Label * infoLabel = manage(new Gtk::Label(
      _("This note is a template note. It determines the default content of regular notes, and will not show up in the note menu or search window.")));
    infoLabel->set_line_wrap(true);

    Gtk::Button * untemplateButton = manage(new Gtk::Button(_("Convert to regular note")));
    untemplateButton->signal_clicked().connect(sigc::mem_fun(*this, &NoteWindow::on_untemplate_button_click));

    m_save_size_check_button = manage(new Gtk::CheckButton(_("Save Si_ze"), true));
    m_save_size_check_button->set_active(m_note.contains_tag(m_template_save_size_tag));
    m_save_size_check_button->signal_toggled().connect(sigc::mem_fun(*this, &NoteWindow::on_save_size_check_button_toggled));

    m_save_selection_check_button = manage(new Gtk::CheckButton(_("Save Se_lection"), true));
    m_save_selection_check_button->set_active(m_note.contains_tag(m_template_save_selection_tag));
    m_save_selection_check_button->signal_toggled().connect(sigc::mem_fun(*this, &NoteWindow::on_save_selection_check_button_toggled));

    m_save_title_check_button = manage(new Gtk::CheckButton(_("Save _Title"), true));
    m_save_title_check_button->set_active(m_note.contains_tag(m_template_save_title_tag));
    m_save_title_check_button->signal_toggled().connect(sigc::mem_fun(*this, &NoteWindow::on_save_title_check_button_toggled));

    bar->attach(*infoLabel, 0, 0, 1, 1);
    bar->attach(*untemplateButton, 0, 1, 1, 1);
    bar->attach(*m_save_size_check_button, 0, 2, 1, 1);
    bar->attach(*m_save_selection_check_button, 0, 3, 1, 1);
    bar->attach(*m_save_title_check_button, 0, 4, 1, 1);

    if(m_note.contains_tag(m_template_tag)) {
      bar->show_all();
    }

    m_note.signal_tag_added.connect(sigc::mem_fun(*this, &NoteWindow::on_note_tag_added));
    m_note.signal_tag_removed.connect(sigc::mem_fun(*this, &NoteWindow::on_note_tag_removed));

    return bar;
  }


  void NoteWindow::on_untemplate_button_click()
  {
    m_note.remove_tag(m_template_tag);
  }


  void NoteWindow::on_save_size_check_button_toggled()
  {
    if(m_save_size_check_button->get_active()) {
      m_note.add_tag(m_template_save_size_tag);
    }
    else {
      m_note.remove_tag(m_template_save_size_tag);
    }
  }


  void NoteWindow::on_save_selection_check_button_toggled()
  {
    if(m_save_selection_check_button->get_active()) {
      m_note.add_tag(m_template_save_selection_tag);
    }
    else {
      m_note.remove_tag(m_template_save_selection_tag);
    }
  }


  void NoteWindow::on_save_title_check_button_toggled()
  {
    if(m_save_title_check_button->get_active()) {
      m_note.add_tag(m_template_save_title_tag);
    }
    else {
      m_note.remove_tag(m_template_save_title_tag);
    }
  }


  void NoteWindow::on_note_tag_added(const NoteBase&, const Tag::Ptr & tag)
  {
    if(tag == m_template_tag) {
      m_template_widget->show_all();
    }
  }


  void NoteWindow::on_note_tag_removed(const NoteBase::Ptr&, const Glib::ustring & tag)
  {
    if(tag == m_template_tag->normalized_name()) {
      m_template_widget->hide();
    }
  }


  //
  // Link menu item activate
  //
  // Create a new note, names according to the buffer's selected
  // text.  Does nothing if there is no active selection.
  //
  void NoteWindow::link_button_clicked()
  {
    Glib::ustring select = m_note.get_buffer()->get_selection();
    if (select.empty())
      return;
    
    Glib::ustring body_unused;
    Glib::ustring title = NoteManagerBase::split_title_from_content(select, body_unused);
    if (title.empty())
      return;

    NoteBase::Ptr match = m_note.manager().find(title);
    if (!match) {
      try {
        match = m_note.manager().create(select);
      } 
      catch (const sharp::Exception & e) {
        utils::HIGMessageDialog dialog(dynamic_cast<Gtk::Window*>(host()),
          GTK_DIALOG_DESTROY_WITH_PARENT,
          Gtk::MESSAGE_ERROR,  Gtk::BUTTONS_OK,
          _("Cannot create note"), e.what());
        dialog.run ();
        return;
      }
    }
    else {
      Gtk::TextIter start, end;
      m_note.get_buffer()->get_selection_bounds(start, end);
      m_note.get_buffer()->remove_tag(m_note.get_tag_table()->get_broken_link_tag(), start, end);
      m_note.get_buffer()->apply_tag(m_note.get_tag_table()->get_link_tag(), start, end);
    }

    MainWindow::present_in(*dynamic_cast<MainWindow*>(host()), static_pointer_cast<Note>(match));
  }

  void NoteWindow::open_help_activate()
  {
    utils::show_help("gnote", "editing-notes", *dynamic_cast<Gtk::Window*>(host()));
  }

  void NoteWindow::change_depth_right_handler()
  {
    Glib::RefPtr<NoteBuffer>::cast_static(m_editor->get_buffer())->change_cursor_depth_directional(true);
  }

  void NoteWindow::change_depth_left_handler()
  {
    Glib::RefPtr<NoteBuffer>::cast_static(m_editor->get_buffer())->change_cursor_depth_directional(false);
  }

  void NoteWindow::on_pin_status_changed(const Note & note, bool pinned)
  {
    if(&m_note != &note) {
      return;
    }
    EmbeddableWidgetHost *h = host();
    if(h != NULL) {
      h->find_action("important-note")->change_state(Glib::Variant<bool>::create(pinned));
    }
  }

  void NoteWindow::on_pin_button_clicked(const Glib::VariantBase & state)
  {
    EmbeddableWidgetHost *h = host();
    if(h != NULL) {
      Glib::Variant<bool> new_state = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(state);
      m_note.set_pinned(new_state.get());
      h->find_action("important-note")->set_state(state);
    }
  }

  void NoteWindow::on_text_button_clicked()
  {
    m_text_menu->show_all();
  }

  void NoteWindow::enabled(bool enable)
  {
    m_enabled = enable;
    m_editor->set_editable(m_enabled);
    embeddable_toolbar()->set_sensitive(m_enabled);
    if(m_global_keys)
      m_global_keys->enabled(m_enabled);
    FOREACH(const MainWindowAction::Ptr & action, get_widget_actions()) {
      // A list includes empty actions to mark separators, non-modifying actions are always enabled
      if(action && !Glib::RefPtr<NonModifyingNoteAction>::cast_dynamic(action)) {
        action->set_enabled(enable);
      }
    }
  }


  NoteFindHandler::NoteFindHandler(Note & note)
    : m_note(note)
  {
  }

  bool NoteFindHandler::goto_previous_result()
  {
    if (m_current_matches.empty() || m_current_matches.size() == 0)
      return false;

    std::list<Match>::reverse_iterator iter(m_current_matches.rbegin());
    for ( ; iter != m_current_matches.rend() ; ++iter) {
      Match & match(*iter);
      
      Glib::RefPtr<NoteBuffer> buffer = match.buffer;
      Gtk::TextIter selection_start, selection_end;
      buffer->get_selection_bounds(selection_start, selection_end);
      Gtk::TextIter end = buffer->get_iter_at_mark(match.start_mark);

      if (end.get_offset() < selection_start.get_offset()) {
        jump_to_match(match);
        return true;
      }
    }

    return false;
  }

  bool NoteFindHandler::goto_next_result()
  {
    if (m_current_matches.empty() || m_current_matches.size() == 0)
      return false;

    std::list<Match>::iterator iter(m_current_matches.begin());
    for ( ; iter != m_current_matches.end() ; ++iter) {
      Match & match(*iter);

      Glib::RefPtr<NoteBuffer> buffer = match.buffer;
      Gtk::TextIter selection_start, selection_end;
      buffer->get_selection_bounds(selection_start, selection_end);
      Gtk::TextIter start = buffer->get_iter_at_mark(match.start_mark);

      if (start.get_offset() >= selection_end.get_offset()) {
        jump_to_match(match);
        return true;
      }
    }

    return false;
  }

  void NoteFindHandler::jump_to_match(const Match & match)
  {
    Glib::RefPtr<NoteBuffer> buffer(match.buffer);

    Gtk::TextIter start = buffer->get_iter_at_mark(match.start_mark);
    Gtk::TextIter end = buffer->get_iter_at_mark(match.end_mark);

    // Move cursor to end of match, and select match text
    buffer->place_cursor(end);
    buffer->move_mark(buffer->get_selection_bound(), start);

    Gtk::TextView *editor = m_note.get_window()->editor();
    editor->scroll_to(buffer->get_insert());
  }


  void NoteFindHandler::perform_search(const Glib::ustring & txt)
  {
    cleanup_matches();
    if(txt.empty()) {
      return;
    }

    Glib::ustring text(txt);
    text = text.lowercase();

    std::vector<Glib::ustring> words;
    Search::split_watching_quotes(words, text);

    find_matches_in_buffer(m_note.get_buffer(), words, m_current_matches);

    if(!m_current_matches.empty()) {
      highlight_matches(true);
      jump_to_match(m_current_matches.front());
    }
  }

  void NoteFindHandler::highlight_matches(bool highlight)
  {
    if(m_current_matches.empty()) {
      return;
    }

    for(std::list<Match>::iterator iter = m_current_matches.begin();
        iter != m_current_matches.end(); ++iter) {
      Match &match(*iter);
      Glib::RefPtr<NoteBuffer> buffer = match.buffer;

      if (match.highlighting != highlight) {
        Gtk::TextIter start = buffer->get_iter_at_mark(match.start_mark);
        Gtk::TextIter end = buffer->get_iter_at_mark(match.end_mark);

        match.highlighting = highlight;

        if (match.highlighting) {
          buffer->apply_tag_by_name("find-match", start, end);
        }
        else {
          buffer->remove_tag_by_name("find-match", start, end);
        }
      }
    }
  }


  void NoteFindHandler::cleanup_matches()
  {
    if (!m_current_matches.empty()) {
      highlight_matches (false /* unhighlight */);

      for(std::list<Match>::const_iterator iter = m_current_matches.begin();
          iter != m_current_matches.end(); ++iter) {
        const Match &match(*iter);
        match.buffer->delete_mark(match.start_mark);
        match.buffer->delete_mark(match.end_mark);
      }

      m_current_matches.clear();
    }
  }



  void NoteFindHandler::find_matches_in_buffer(const Glib::RefPtr<NoteBuffer> & buffer, 
                                               const std::vector<Glib::ustring> & words,
                                               std::list<NoteFindHandler::Match> & matches)
  {
    matches.clear();
    Glib::ustring note_text = buffer->get_slice (buffer->begin(),
                                               buffer->end(),
                                               false /* hidden_chars */);
    note_text = note_text.lowercase();

    for(std::vector<Glib::ustring>::const_iterator iter = words.begin();
        iter != words.end(); ++iter) {
      const Glib::ustring & word(*iter);
      Glib::ustring::size_type idx = 0;
      bool this_word_found = false;

      if (word.empty())
        continue;

      while(true) {
        idx = note_text.find(word, idx);
        if (idx == Glib::ustring::npos) {
          if (this_word_found) {
            break;
          }
          else {
            matches.clear();
            return;
          }
        }

        this_word_found = true;

        Gtk::TextIter start = buffer->get_iter_at_offset(idx);
        Gtk::TextIter end = start;
        end.forward_chars(word.length());

        Match match;
        match.buffer = buffer;
        match.start_mark = buffer->create_mark(start, false);
        match.end_mark = buffer->create_mark(end, true);
        match.highlighting = false;

        matches.push_back(match);

        idx += word.length();
      }
    }
  }


  // FIXME: Tags applied to a word should hold over the space
  // between the next word, as thats where you'll start typeing.
  // Tags are only active -after- a character with that tag.  This
  // is different from the way gtk-textbuffer applies tags.

  //
  // Text menu
  //
  // Menu for font style and size, and set the active radio
  // menuitem depending on the cursor poition.
  //
  NoteTextMenu::NoteTextMenu(EmbeddableWidget & widget, const Glib::RefPtr<NoteBuffer> & buffer, UndoManager & undo_manager)
    : Gtk::PopoverMenu()
    , m_widget(widget)
    , m_buffer(buffer)
    , m_undo_manager(undo_manager)
    {
      m_widget.signal_foregrounded.connect(sigc::mem_fun(*this, &NoteTextMenu::on_widget_foregrounded));
      m_widget.signal_backgrounded.connect(sigc::mem_fun(*this, &NoteTextMenu::on_widget_backgrounded));

      set_position(Gtk::POS_BOTTOM);
      Gtk::Box *menu_box = manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));

      // Listen to events so we can sensitize and
      // enable keybinding
      undo_manager.signal_undo_changed().connect(sigc::mem_fun(*this, &NoteTextMenu::undo_changed));

      Glib::Quark tag_quark("Tag");
      Gtk::Widget *bold = create_font_item("win.change-font-bold", _("_Bold"), "b");
      Gtk::Widget *italic = create_font_item("win.change-font-italic", _("_Italic"), "i");
      Gtk::Widget *strikeout = create_font_item("win.change-font-strikeout", _("_Strikeout"), "s");

      Gtk::Widget *highlight = manage(utils::create_popover_button("win.change-font-highlight", ""));
      auto label = static_cast<Gtk::Label*>(static_cast<Gtk::Bin*>(highlight)->get_child());
      Glib::ustring markup = Glib::ustring::compose("<span background=\"yellow\">%1</span>", _("_Highlight"));
      label->set_markup_with_mnemonic(markup);

      Gtk::Widget *normal = create_font_size_item(_("_Normal"), NULL, "");
      Gtk::Widget *small = create_font_size_item(_("S_mall"), "small", "size:small");
      Gtk::Widget *large = create_font_size_item(_("_Large"), "large", "size:large");
      Gtk::Widget *huge = create_font_size_item(_("Hu_ge"), "x-large", "size:huge");

      auto box = manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
      utils::set_common_popover_widget_props(*box);
      box->set_name("formatting");
      box->add(*bold);
      box->add(*italic);
      box->add(*strikeout);
      box->add(*highlight);
      menu_box->add(*box);
      menu_box->add(*manage(new Gtk::Separator));

      box = manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
      utils::set_common_popover_widget_props(*box);
      box->set_name("font-size");
      box->add(*small);
      box->add(*normal);
      box->add(*large);
      box->add(*huge);
      menu_box->add(*box);
      menu_box->add(*manage(new Gtk::Separator));

      Gtk::Widget *bullets = manage(utils::create_popover_button("win.enable-bullets", _("⦁ Bullets")));
      menu_box->add(*bullets);
      Gtk::Widget *increase_indent = manage(utils::create_popover_button("win.increase-indent", _("→ Increase indent")));
      menu_box->add(*increase_indent);
      Gtk::Widget *decrease_indent = manage(utils::create_popover_button("win.decrease-indent", _("← Decrease indent")));
      menu_box->add(*decrease_indent);

      add(*menu_box);

      refresh_state();
    }

  Gtk::Widget *NoteTextMenu::create_font_item(const char *action, const char *label, const char *markup)
  {
    Gtk::Widget *widget = manage(utils::create_popover_button(action, ""));
    auto lbl = static_cast<Gtk::Label*>(static_cast<Gtk::Bin*>(widget)->get_child());
    Glib::ustring m = Glib::ustring::compose("<%1>%2</%1>", markup, label);
    lbl->set_markup_with_mnemonic(m);
    return widget;
  }

  Gtk::Widget *NoteTextMenu::create_font_size_item(const char *label, const char *markup, const char *size)
  {
    Gtk::Widget *item = manage(utils::create_popover_button("win.change-font-size", ""));
    auto lbl = static_cast<Gtk::Label*>(static_cast<Gtk::Bin*>(item)->get_child());
    Glib::ustring mrkp;
    if(markup != NULL) {
      mrkp = Glib::ustring::compose("<span size=\"%1\">%2</span>", markup, label);
    }
    else {
      mrkp = label;
    }
    lbl->set_markup_with_mnemonic(mrkp);
    gtk_actionable_set_action_target_value(GTK_ACTIONABLE(item->gobj()), g_variant_new_string(size));
    return item;
  }

  void NoteTextMenu::set_accels(utils::GlobalKeybinder & keybinder)
  {
    keybinder.add_accelerator(sigc::mem_fun(*this, &NoteTextMenu::undo_clicked),
                              GDK_KEY_Z, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    keybinder.add_accelerator(sigc::mem_fun(*this, &NoteTextMenu::redo_clicked),
                              GDK_KEY_Z, Gdk::CONTROL_MASK | Gdk::SHIFT_MASK, Gtk::ACCEL_VISIBLE);
    keybinder.add_accelerator(sigc::mem_fun(*this, &NoteTextMenu::link_clicked),
                              GDK_KEY_L, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    keybinder.add_accelerator(sigc::mem_fun(*this, &NoteTextMenu::bold_pressed),
                              GDK_KEY_B, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    keybinder.add_accelerator(sigc::mem_fun(*this, &NoteTextMenu::italic_pressed),
                              GDK_KEY_I, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    keybinder.add_accelerator(sigc::mem_fun(*this, &NoteTextMenu::strikeout_pressed),
                              GDK_KEY_S, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    keybinder.add_accelerator(sigc::mem_fun(*this, &NoteTextMenu::highlight_pressed),
                              GDK_KEY_H, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    keybinder.add_accelerator(sigc::mem_fun(*this, &NoteTextMenu::increase_font_clicked),
                              GDK_KEY_plus, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    keybinder.add_accelerator(sigc::mem_fun(*this, &NoteTextMenu::decrease_font_clicked),
                              GDK_KEY_minus, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    keybinder.add_accelerator(sigc::mem_fun(*this, &NoteTextMenu::increase_indent_pressed),
                              GDK_KEY_Right, Gdk::MOD1_MASK, Gtk::ACCEL_VISIBLE);
    keybinder.add_accelerator(sigc::mem_fun(*this, &NoteTextMenu::decrease_indent_pressed),
                              GDK_KEY_Left, Gdk::MOD1_MASK, Gtk::ACCEL_VISIBLE);

    signal_set_accels(keybinder);
  }

  void NoteTextMenu::on_show()
  {
    refresh_state();
    Gtk::PopoverMenu::on_show();
  }

  void NoteTextMenu::refresh_sizing_state()
  {
    EmbeddableWidgetHost *host = m_widget.host();
    if(host == NULL) {
      return;
    }
    auto action = host->find_action("change-font-size");
    Gtk::TextIter cursor = m_buffer->get_iter_at_mark(m_buffer->get_insert());
    Gtk::TextIter selection = m_buffer->get_iter_at_mark(m_buffer->get_selection_bound());

    // When on title line, activate the hidden menu item
    if ((cursor.get_line() == 0) || (selection.get_line() == 0)) {
      action->set_enabled(false);
      return;
    }

    action->set_enabled(true);
    if(m_buffer->is_active_tag ("size:huge")) {
      action->set_state(Glib::Variant<Glib::ustring>::create("size:huge"));
    }
    else if(m_buffer->is_active_tag ("size:large")) {
      action->set_state(Glib::Variant<Glib::ustring>::create("size:large"));
    }
    else if(m_buffer->is_active_tag ("size:small")) {
      action->set_state(Glib::Variant<Glib::ustring>::create("size:small"));
    }
    else {
      action->set_state(Glib::Variant<Glib::ustring>::create(""));
    }
  }

  void NoteTextMenu::refresh_state ()
  {
    EmbeddableWidgetHost *host = m_widget.host();
    if(host == NULL) {
      return;
    }

    m_event_freeze = true;

    Gtk::TextIter start, end;
    host->find_action("link")->property_enabled() = m_buffer->get_selection_bounds(start, end);
    host->find_action("change-font-bold")->set_state(Glib::Variant<bool>::create(m_buffer->is_active_tag("bold")));
    host->find_action("change-font-italic")->set_state(Glib::Variant<bool>::create(m_buffer->is_active_tag("italic")));
    host->find_action("change-font-strikeout")->set_state(Glib::Variant<bool>::create(m_buffer->is_active_tag("strikethrough")));
    host->find_action("change-font-highlight")->set_state(Glib::Variant<bool>::create(m_buffer->is_active_tag("highlight")));

    bool inside_bullets = m_buffer->is_bulleted_list_active();
    bool can_make_bulleted_list = m_buffer->can_make_bulleted_list();
    auto enable_bullets = host->find_action("enable-bullets");
    enable_bullets->set_state(Glib::Variant<bool>::create(inside_bullets));
    enable_bullets->property_enabled() = can_make_bulleted_list;
    host->find_action("increase-indent")->property_enabled() = inside_bullets;
    host->find_action("decrease-indent")->property_enabled() = inside_bullets;

    refresh_sizing_state ();

    undo_changed();

    m_event_freeze = false;
  }

  void NoteTextMenu::on_widget_foregrounded()
  {
    auto host = m_widget.host();

    m_signal_cids.push_back(host->find_action("undo")->signal_activate()
      .connect([this](const Glib::VariantBase&) { undo_clicked(); } ));
    m_signal_cids.push_back(host->find_action("redo")->signal_activate()
      .connect([this](const Glib::VariantBase&) { redo_clicked(); } ));
    m_signal_cids.push_back(host->find_action("link")->signal_activate()
      .connect([this](const Glib::VariantBase&) { link_clicked(); } ));
    m_signal_cids.push_back(host->find_action("change-font-bold")->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteTextMenu::bold_clicked)));
    m_signal_cids.push_back(host->find_action("change-font-italic")->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteTextMenu::italic_clicked)));
    m_signal_cids.push_back(host->find_action("change-font-strikeout")->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteTextMenu::strikeout_clicked)));
    m_signal_cids.push_back(host->find_action("change-font-highlight")->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteTextMenu::highlight_clicked)));
    m_signal_cids.push_back(host->find_action("change-font-size")->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteTextMenu::font_size_activated)));
    m_signal_cids.push_back(host->find_action("enable-bullets")->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteTextMenu::toggle_bullets_clicked)));
    m_signal_cids.push_back(host->find_action("increase-indent")->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteTextMenu::increase_indent_clicked)));
    m_signal_cids.push_back(host->find_action("decrease-indent")->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteTextMenu::decrease_indent_clicked)));
  }

  void NoteTextMenu::on_widget_backgrounded()
  {
    for(auto & cid : m_signal_cids) {
      cid.disconnect();
    }
    m_signal_cids.clear();
  }

  void NoteTextMenu::link_clicked()
  {
    if(m_event_freeze) {
      return;
    }

    Glib::ustring select = m_buffer->get_selection();
    if(select.empty()) {
      return;
    }

    Glib::ustring body_unused;
    Glib::ustring title = NoteManagerBase::split_title_from_content(select, body_unused);
    if(title.empty()) {
      return;
    }

    NoteManagerBase & manager(m_buffer->note().manager());
    NoteBase::Ptr match = manager.find(title);
    if(!match) {
      try {
        match = manager.create(select);
      }
      catch(const sharp::Exception & e) {
        utils::HIGMessageDialog dialog(dynamic_cast<Gtk::Window*>(m_buffer->note().get_window()->host()),
          GTK_DIALOG_DESTROY_WITH_PARENT,
          Gtk::MESSAGE_ERROR,  Gtk::BUTTONS_OK,
          _("Cannot create note"), e.what());
        dialog.run();
        return;
      }
    }
    else {
      Gtk::TextIter start, end;
      m_buffer->get_selection_bounds(start, end);
      m_buffer->remove_tag(m_buffer->note().get_tag_table()->get_broken_link_tag(), start, end);
      m_buffer->apply_tag(m_buffer->note().get_tag_table()->get_link_tag(), start, end);
    }

    MainWindow::present_in(*dynamic_cast<MainWindow*>(m_buffer->note().get_window()->host()),
                           static_pointer_cast<Note>(match));
  }

  //
  // Font-style menu item activate
  //
  // Toggle the style tag for the current text.  Style tags are
  // stored in a "Tag" member of the menuitem's Data.
  //
  void NoteTextMenu::font_style_clicked(const char * tag)
  {
    if (m_event_freeze)
      return;

    if(tag) {
      m_buffer->toggle_active_tag(tag);
    }
  }

  void NoteTextMenu::font_clicked(const char *action, const Glib::VariantBase & state, void (NoteTextMenu::*func)())
  {
    auto host = m_widget.host();
    if(host == NULL) {
      return;
    }
    host->find_action(action)->set_state(state);
    (this->*func)();
  }

  void NoteTextMenu::bold_clicked(const Glib::VariantBase & state)
  {
    font_clicked("change-font-bold", state, &NoteTextMenu::bold_pressed);
  }

  void NoteTextMenu::bold_pressed()
  {
    font_style_clicked("bold");
  }

  void NoteTextMenu::italic_clicked(const Glib::VariantBase & state)
  {
    font_clicked("change-font-italic", state, &NoteTextMenu::italic_pressed);
  }

  void NoteTextMenu::italic_pressed()
  {
    font_style_clicked("italic");
  }

  void NoteTextMenu::strikeout_clicked(const Glib::VariantBase & state)
  {
    font_clicked("change-font-strikeout", state, &NoteTextMenu::strikeout_pressed);
  }

  void NoteTextMenu::strikeout_pressed()
  {
    font_style_clicked("strikethrough");
  }

  void NoteTextMenu::highlight_clicked(const Glib::VariantBase & state)
  {
    font_clicked("change-font-highlight", state, &NoteTextMenu::highlight_pressed);
  }

  void NoteTextMenu::highlight_pressed()
  {
    font_style_clicked("highlight");
  }

  // Font-style menu item activate
  void NoteTextMenu::font_size_activated(const Glib::VariantBase & state)
  {
    if (m_event_freeze)
      return;

    auto host = m_widget.host();
    if(host == NULL) {
      return;
    }
    host->find_action("change-font-size")->set_state(state);

    m_buffer->remove_active_tag ("size:huge");
    m_buffer->remove_active_tag ("size:large");
    m_buffer->remove_active_tag ("size:small");

    auto tag = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(state).get();
    if(!tag.empty())
      m_buffer->set_active_tag(tag);
  }

  void NoteTextMenu::increase_font_clicked ()
  {
    if (m_event_freeze)
      return;

    if (m_buffer->is_active_tag ("size:small")) {
      m_buffer->remove_active_tag ("size:small");
    } 
    else if (m_buffer->is_active_tag ("size:large")) {
      m_buffer->remove_active_tag ("size:large");
      m_buffer->set_active_tag ("size:huge");
    } 
    else if (m_buffer->is_active_tag ("size:huge")) {
      // Maximum font size, do nothing
    } 
    else {
      // Current font size is normal
      m_buffer->set_active_tag ("size:large");
    }
 }

  void NoteTextMenu::decrease_font_clicked ()
  {
    if (m_event_freeze)
      return;

    if (m_buffer->is_active_tag ("size:small")) {
// Minimum font size, do nothing
    } 
    else if (m_buffer->is_active_tag ("size:large")) {
      m_buffer->remove_active_tag ("size:large");
    } 
    else if (m_buffer->is_active_tag ("size:huge")) {
      m_buffer->remove_active_tag ("size:huge");
      m_buffer->set_active_tag ("size:large");
    } 
    else {
// Current font size is normal
      m_buffer->set_active_tag ("size:small");
    }
  }

  void NoteTextMenu::undo_clicked()
  {
    if (m_undo_manager.get_can_undo()) {
      DBG_OUT("Running undo...");
      m_undo_manager.undo();
    }
  }

  void NoteTextMenu::redo_clicked()
  {
    if (m_undo_manager.get_can_redo()) {
      DBG_OUT("Running redo...");
      m_undo_manager.redo();
    }
  }

  void NoteTextMenu::undo_changed ()
  {
    EmbeddableWidgetHost *host = m_widget.host();
    if(host == NULL) {
      return;
    }
    host->find_action("undo")->property_enabled() = m_undo_manager.get_can_undo();
    host->find_action("redo")->property_enabled() = m_undo_manager.get_can_redo();
  }


    //
    // Bulleted list handlers
    //
  void NoteTextMenu::toggle_bullets_clicked(const Glib::VariantBase&)
  {
    m_buffer->toggle_selection_bullets();
  }

  void NoteTextMenu::increase_indent_clicked(const Glib::VariantBase&)
  {
    m_buffer->increase_cursor_depth();
  }

  void NoteTextMenu::increase_indent_pressed()
  {
    m_buffer->toggle_selection_bullets();
  }

  void NoteTextMenu::decrease_indent_clicked(const Glib::VariantBase&)
  {
    m_buffer->decrease_cursor_depth();
  }

  void NoteTextMenu::decrease_indent_pressed()
  {
    m_buffer->decrease_cursor_depth();
  }

}
