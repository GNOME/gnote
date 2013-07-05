/*
 * gnote
 *
 * Copyright (C) 2011-2013 Aurimas Cernius
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
#include <gtkmm/stock.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/separatormenuitem.h>

#include "debug.hpp"
#include "iconmanager.hpp"
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


namespace gnote {

  Glib::RefPtr<Gio::Icon> NoteWindow::get_icon_pin_active()
  {
    return IconManager::obj().get_icon(IconManager::PIN_ACTIVE, 22);
  }

  Glib::RefPtr<Gio::Icon> NoteWindow::get_icon_pin_down()
  {
    return IconManager::obj().get_icon(IconManager::PIN_DOWN, 22);
  }



  NoteWindow::NoteWindow(Note & note)
    : m_note(note)
    , m_name(note.get_title())
    , m_height(360)
    , m_width(450)
    , m_find_handler(note)
    , m_global_keys(NULL)
  {
    m_template_tag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
    m_template_save_size_tag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SAVE_SIZE_SYSTEM_TAG);
    m_template_save_selection_tag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SAVE_SELECTION_SYSTEM_TAG);
    m_template_save_title_tag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SAVE_TITLE_SYSTEM_TAG);

    set_hexpand(true);
    set_vexpand(true);

    m_text_menu = Gtk::manage(new NoteTextMenu(note.get_buffer(), note.get_buffer()->undoer()));

    m_embeddable_toolbar = manage(make_toolbar());

    m_template_widget = make_template_bar();

    // The main editor widget
    m_editor = manage(new NoteEditor(note.get_buffer()));
    m_editor->signal_populate_popup().connect(sigc::mem_fun(*this, &NoteWindow::on_populate_popup));
    m_editor->show();

    // Sensitize the Link toolbar button on text selection
    m_mark_set_timeout = new utils::InterruptableTimeout();
    m_mark_set_timeout->signal_timeout.connect(
      sigc::mem_fun(*this, &NoteWindow::update_link_button_sensitivity));
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


  std::string NoteWindow::get_name() const
  {
    return m_name;
  }

  void NoteWindow::set_name(const std::string & name)
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
  }

  void NoteWindow::background()
  {
    EmbeddableWidget::background();
    Gtk::Window *parent = dynamic_cast<Gtk::Window*>(host());
    if(!parent) {
      return;
    }
    remove_accel_group(*parent);
    if(parent->get_window() != 0
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
  }

  void NoteWindow::hint_size(int & width, int & height)
  {
    width = m_width;
    height = m_height;
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
      m_text_menu->set_accel_group(m_accel_group);
      m_link_button->add_accelerator("clicked", m_accel_group,
                                     GDK_KEY_L, Gdk::CONTROL_MASK,
                                     Gtk::ACCEL_VISIBLE);

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
      }
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

  void NoteWindow::perform_search(const std::string & text)
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

  std::vector<Glib::RefPtr<Gtk::Action> > NoteWindow::get_widget_actions()
  {
    std::vector<Glib::RefPtr<Gtk::Action> > res;
    for(std::map<int, Glib::RefPtr<Gtk::Action> >::iterator iter = m_widget_actions.begin();
        iter != m_widget_actions.end(); ++iter) {
      res.push_back(iter->second);
    }
    return res;
  }

  sigc::signal<void> & NoteWindow::signal_actions_changed()
  {
    return m_signal_actions_changed;
  }

  void NoteWindow::add_widget_action(const Glib::RefPtr<Gtk::Action> & action, int order)
  {
    std::map<int, Glib::RefPtr<Gtk::Action> >::iterator iter = m_widget_actions.find(order);
    while(iter != m_widget_actions.end()) {
      iter = m_widget_actions.find(++order);
    }
    m_widget_actions[order] = action;
    m_signal_actions_changed();
  }

  void NoteWindow::remove_widget_action(const std::string & name)
  {
    for(std::map<int, Glib::RefPtr<Gtk::Action> >::iterator iter = m_widget_actions.begin();
        iter != m_widget_actions.end(); ++iter) {
      if(iter->second->get_name() == name) {
        m_widget_actions.erase(iter);
        break;
      }
    }
    m_signal_actions_changed();
  }


    // Delete this Note.
    //

  void NoteWindow::on_delete_button_clicked()
  {
    // Prompt for note deletion
    std::list<Note::Ptr> single_note_list;
    single_note_list.push_back(m_note.shared_from_this());
    noteutils::show_deletion_dialog(single_note_list, dynamic_cast<Gtk::Window*>(host()));
  }

  void NoteWindow::on_selection_mark_set(const Gtk::TextIter&, const Glib::RefPtr<Gtk::TextMark>&)
  {
    // FIXME: Process in a timeout due to GTK+ bug #172050.
    if(m_mark_set_timeout) {
      m_mark_set_timeout->reset(0);
    }
  }

  void NoteWindow::update_link_button_sensitivity()
  {
    m_link_button->set_sensitive(!m_note.get_buffer()->get_selection().empty());
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
      
    Gtk::ImageMenuItem *text_item = manage(new Gtk::ImageMenuItem(_("Te_xt"), true));
    text_item->set_image(*manage(new Gtk::Image(
                                   Gtk::Stock::SELECT_FONT, 
                                   Gtk::ICON_SIZE_MENU)));
    text_item->set_submenu(*manage(new NoteTextMenu(m_note.get_buffer(),
                                                    m_note.get_buffer()->undoer())));
    text_item->show();

    Gtk::MenuItem *spacer2 = manage(new Gtk::SeparatorMenuItem());
    spacer2->show();

    menu->prepend(*spacer1);
    menu->prepend(*text_item);
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
    Gtk::IconSize icon_size = Gtk::IconSize::from_name(gtk_icon_size_get_name(GTK_ICON_SIZE_SMALL_TOOLBAR));

    Gtk::Grid *grid = manage(new Gtk::Grid);
    int grid_col = 0;

    m_pin_image = manage(new Gtk::Image);
    if(m_note.is_pinned()) {
      m_pin_image->property_gicon() = get_icon_pin_down();
    }
    else {
      m_pin_image->property_gicon() = get_icon_pin_active();
    }

    m_pin_button = manage(new Gtk::ToolButton(*m_pin_image, _("Pin")));
    m_pin_button->set_tooltip_text(_("Mark note as important"));
    m_pin_button->signal_clicked().connect(sigc::mem_fun(*this, &NoteWindow::on_pin_button_clicked));
    grid->attach(*m_pin_button, grid_col++, 0, 1, 1);
    notebooks::NotebookManager::obj().signal_note_pin_status_changed
      .connect(sigc::mem_fun(*this, &NoteWindow::on_pin_status_changed));

    m_link_button = manage(new Gtk::ToolButton(
                             *manage(new Gtk::Image(Gtk::Stock::JUMP_TO, icon_size)),
                             _("Link")));
    m_link_button->set_use_underline(true);
    m_link_button->set_is_important(true);
    m_link_button->set_sensitive(!m_note.get_buffer()->get_selection().empty());
    m_link_button->signal_clicked().connect(
      sigc::mem_fun(*this, &NoteWindow::link_button_clicked));
    m_link_button->set_tooltip_text(_("Link selected text to a new note (Ctrl-L)"));
    m_link_button->show_all();
    grid->attach(*m_link_button, grid_col++, 0, 1, 1);

    utils::ToolMenuButton *text_button = manage(new utils::ToolMenuButton(
        *manage(new Gtk::Image(Gtk::Stock::SELECT_FONT, icon_size)), _("_Text"), m_text_menu));
    text_button->set_use_underline(true);
    text_button->set_is_important(true);
    text_button->show_all();
    grid->attach(*text_button, grid_col++, 0, 1, 1);
    text_button->set_tooltip_text(_("Set properties of text"));

    grid->attach(*manage(new Gtk::SeparatorToolItem()), grid_col++, 0, 1, 1);

    m_delete_button = manage(new Gtk::ToolButton(Gtk::Stock::DELETE));
    m_delete_button->set_use_underline(true);
    m_delete_button->signal_clicked().connect(
      sigc::mem_fun(*this, &NoteWindow::on_delete_button_clicked));
    m_delete_button->show_all();
    grid->attach(*m_delete_button, grid_col++, 0, 1, 1);
    m_delete_button->set_tooltip_text(_("Delete this note"));

      // Don't allow deleting the "Start Here" note...
    if (m_note.is_special()) {
      m_delete_button->set_sensitive(false);
    }
    grid->attach(*manage(new Gtk::SeparatorToolItem()), grid_col++, 0, 1, 1);

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

    m_note.signal_tag_added().connect(sigc::mem_fun(*this, &NoteWindow::on_note_tag_added));
    m_note.signal_tag_removed().connect(sigc::mem_fun(*this, &NoteWindow::on_note_tag_removed));

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


  void NoteWindow::on_note_tag_added(const Note&, const Tag::Ptr & tag)
  {
    if(tag == m_template_tag) {
      m_template_widget->show_all();
    }
  }


  void NoteWindow::on_note_tag_removed(const Note::Ptr&, const std::string & tag)
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
    std::string select = m_note.get_buffer()->get_selection();
    if (select.empty())
      return;
    
    std::string body_unused;
    std::string title = NoteManager::split_title_from_content(select, body_unused);
    if (title.empty())
      return;

    Note::Ptr match = m_note.manager().find(title);
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

    host()->embed_widget(*match->get_window());
  }

  void NoteWindow::open_help_activate()
  {
    utils::show_help("gnote", "editing-notes", get_screen()->gobj(),
                     dynamic_cast<Gtk::Window*>(host()));
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
    if(pinned) {
      m_pin_image->property_gicon() = get_icon_pin_down();
    }
    else {
      m_pin_image->property_gicon() = get_icon_pin_active();
    }
  }

  void NoteWindow::on_pin_button_clicked()
  {
    m_note.set_pinned(!m_note.is_pinned());
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


  void NoteFindHandler::perform_search(const std::string & txt)
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
  NoteTextMenu::NoteTextMenu(const Glib::RefPtr<NoteBuffer> & buffer, UndoManager & undo_manager)
    : Gtk::Menu()
    , m_buffer(buffer)
    , m_undo_manager(undo_manager)
    , m_bold(_("<b>_Bold</b>"), true)
    , m_italic(_("<i>_Italic</i>"), true)
    , m_strikeout(_("<s>_Strikeout</s>"), true)
    , m_highlight(Glib::ustring("<span background=\"yellow\">")
                  + _("_Highlight") + "</span>", true)
    , m_fontsize_group()
    , m_normal(m_fontsize_group, _("_Normal"), true)
    , m_huge(m_fontsize_group, Glib::ustring("<span size=\"x-large\">")
             + _("Hu_ge") + "</span>", true)
    , m_large(m_fontsize_group, Glib::ustring("<span size=\"large\">")
            + _("_Large") + "</span>", true)
       ,  m_small(m_fontsize_group, Glib::ustring("<span size=\"small\">")
                  + _("S_mall") + "</span>", true)
    , m_hidden_no_size(m_fontsize_group, "", true)
    , m_bullets(_("Bullets"))
    , m_increase_indent(Gtk::Stock::INDENT)
    , m_decrease_indent(Gtk::Stock::UNINDENT)
    , m_increase_font(_("Increase Font Size"), true)
    , m_decrease_font(_("Decrease Font Size"), true)
    {
      m_undo = manage(new Gtk::ImageMenuItem (Gtk::Stock::UNDO));
      m_undo->signal_activate().connect(sigc::mem_fun(*this, &NoteTextMenu::undo_clicked));
      m_undo->show();
      append(*m_undo);

      m_redo = manage(new Gtk::ImageMenuItem (Gtk::Stock::REDO));
      m_redo->signal_activate().connect(sigc::mem_fun(*this, &NoteTextMenu::redo_clicked));
      m_redo->show();
      append(*m_redo);

      Gtk::SeparatorMenuItem *undo_spacer = manage(new Gtk::SeparatorMenuItem());
      append(*undo_spacer);

      // Listen to events so we can sensitize and
      // enable keybinding
      undo_manager.signal_undo_changed().connect(sigc::mem_fun(*this, &NoteTextMenu::undo_changed));

      Glib::Quark tag_quark("Tag");
      markup_label(m_bold);
      m_bold.set_data(tag_quark, const_cast<char*>("bold"));
      m_bold.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_style_clicked), &m_bold));

      markup_label (m_italic);
      m_italic.set_data(tag_quark, const_cast<char*>("italic"));
      m_italic.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_style_clicked), &m_italic));

      markup_label (m_strikeout);
      m_strikeout.set_data(tag_quark, const_cast<char*>("strikethrough"));
      m_strikeout.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_style_clicked), &m_strikeout));

      markup_label (m_highlight);
      m_highlight.set_data(tag_quark, const_cast<char*>("highlight"));
      m_highlight.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_style_clicked), &m_highlight));

      Gtk::SeparatorMenuItem *spacer1 = manage(new Gtk::SeparatorMenuItem());

      markup_label(m_normal);
      m_normal.set_active(true);
      m_normal.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_size_activated), &m_normal));

      markup_label(m_huge);
      m_huge.set_data(tag_quark, const_cast<char*>("size:huge"));
      m_huge.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_size_activated), &m_huge));

      markup_label(m_large);
      m_large.set_data(tag_quark, const_cast<char*>("size:large"));
      m_large.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_size_activated), &m_large));

      markup_label(m_small);
      m_small.set_data(tag_quark, const_cast<char*>("size:small"));
      m_small.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_size_activated), &m_small));

      m_hidden_no_size.hide();

      m_increase_font.signal_activate()
        .connect(sigc::mem_fun(*this, &NoteTextMenu::increase_font_clicked));

      m_decrease_font.signal_activate()
        .connect(sigc::mem_fun(*this, &NoteTextMenu::decrease_font_clicked));

      Gtk::SeparatorMenuItem *spacer2 = manage(new Gtk::SeparatorMenuItem());

      m_bullets_clicked_cid = m_bullets.signal_activate()
        .connect(sigc::mem_fun(*this, &NoteTextMenu::toggle_bullets_clicked));

      m_increase_indent.signal_activate()
        .connect(sigc::mem_fun(*this, &NoteTextMenu::increase_indent_clicked));
      m_increase_indent.show();

      m_decrease_indent.signal_activate()
        .connect(sigc::mem_fun(*this, &NoteTextMenu::decrease_indent_clicked));
      m_decrease_indent.show();

      refresh_state();

      append(m_bold);
      append(m_italic);
      append(m_strikeout);
      append(m_highlight);
      append(*spacer1);
      append(m_small);
      append(m_normal);
      append(m_large);
      append(m_huge);
      append(m_increase_font);
      append(m_decrease_font);
      append(*spacer2);
      append(m_bullets);
      append(m_increase_indent);
      append(m_decrease_indent);
      show_all ();
    }

  void NoteTextMenu::set_accel_group(const Glib::RefPtr<Gtk::AccelGroup> & accel_group)
  {
    m_undo->add_accelerator("activate", accel_group,
                            GDK_KEY_Z,
                            Gdk::CONTROL_MASK,
                            Gtk::ACCEL_VISIBLE);
    m_redo->add_accelerator("activate", accel_group,
                            GDK_KEY_Z,
                            Gdk::CONTROL_MASK | Gdk::SHIFT_MASK,
                            Gtk::ACCEL_VISIBLE);
    m_bold.add_accelerator("activate", accel_group,
                           GDK_KEY_B,
                           Gdk::CONTROL_MASK,
                           Gtk::ACCEL_VISIBLE);
    m_italic.add_accelerator("activate", accel_group,
                             GDK_KEY_I,
                             Gdk::CONTROL_MASK,
                             Gtk::ACCEL_VISIBLE);
    m_strikeout.add_accelerator("activate", accel_group,
                                GDK_KEY_S,
                                Gdk::CONTROL_MASK,
                                Gtk::ACCEL_VISIBLE);
    m_highlight.add_accelerator("activate", accel_group,
                                GDK_KEY_H,
                                Gdk::CONTROL_MASK,
                                Gtk::ACCEL_VISIBLE);
    m_increase_font.add_accelerator("activate", accel_group,
                                    GDK_KEY_plus,
                                    Gdk::CONTROL_MASK,
                                    Gtk::ACCEL_VISIBLE);
    m_decrease_font.add_accelerator("activate", accel_group,
                                    GDK_KEY_minus,
                                    Gdk::CONTROL_MASK,
                                    Gtk::ACCEL_VISIBLE);
    m_increase_indent.add_accelerator("activate", accel_group,
                                      GDK_KEY_Right,
                                      Gdk::MOD1_MASK,
                                      Gtk::ACCEL_VISIBLE);
    m_decrease_indent.add_accelerator("activate", accel_group,
                                      GDK_KEY_Left,
                                      Gdk::MOD1_MASK,
                                      Gtk::ACCEL_VISIBLE);
  }

  void NoteTextMenu::on_show()
  {
    refresh_state();
    Gtk::Menu::on_show();
  }

  void NoteTextMenu::markup_label (Gtk::MenuItem & item)
  {
    Gtk::Label *label = dynamic_cast<Gtk::Label*>(item.get_child());
    if(label) {
      label->set_use_markup(true);
      label->set_use_underline(true);
    }
  }


  void NoteTextMenu::refresh_sizing_state()
  {
    Gtk::TextIter cursor = m_buffer->get_iter_at_mark(m_buffer->get_insert());
    Gtk::TextIter selection = m_buffer->get_iter_at_mark(m_buffer->get_selection_bound());

    // When on title line, activate the hidden menu item
    if ((cursor.get_line() == 0) || (selection.get_line() == 0)) {
      m_hidden_no_size.set_active(true);
      return;
    }

    bool has_size = false;
    bool active;
    active = m_buffer->is_active_tag ("size:huge");
    has_size |= active;
    m_huge.set_active(active);
    active = m_buffer->is_active_tag ("size:large");
    has_size |= active;
    m_large.set_active(active);
    active = m_buffer->is_active_tag ("size:small");
    has_size |= active;
    m_small.set_active(active);

    m_normal.set_active(!has_size);

  }

  void NoteTextMenu::refresh_state ()
  {
    m_event_freeze = true;

    m_bold.set_active(m_buffer->is_active_tag("bold"));
    m_italic.set_active(m_buffer->is_active_tag("italic"));
    m_strikeout.set_active(m_buffer->is_active_tag("strikethrough"));
    m_highlight.set_active(m_buffer->is_active_tag("highlight"));

    bool inside_bullets = m_buffer->is_bulleted_list_active();
    bool can_make_bulleted_list = m_buffer->can_make_bulleted_list();
    m_bullets_clicked_cid.block();
    m_bullets.set_active(inside_bullets);
    m_bullets_clicked_cid.unblock();
    m_bullets.set_sensitive(can_make_bulleted_list);
    m_increase_indent.set_sensitive(inside_bullets);
    m_decrease_indent.set_sensitive(inside_bullets);

    refresh_sizing_state ();

    m_undo->set_sensitive(m_undo_manager.get_can_undo());
    m_redo->set_sensitive(m_undo_manager.get_can_redo());

    m_event_freeze = false;
  }

  //
  // Font-style menu item activate
  //
  // Toggle the style tag for the current text.  Style tags are
  // stored in a "Tag" member of the menuitem's Data.
  //
  void NoteTextMenu::font_style_clicked(Gtk::CheckMenuItem * item)
  {
    if (m_event_freeze)
      return;

    const char * tag = (const char *)item->get_data(Glib::Quark("Tag"));

    if (tag)
      m_buffer->toggle_active_tag(tag);
  }

  //
  // Font-style menu item activate
  //
  // Set the font size tag for the current text.  Style tags are
  // stored in a "Tag" member of the menuitem's Data.
  //

  // FIXME: Change this back to use FontSizeToggled instead of using the
  // Activated signal.  Fix the Text menu so it doesn't show a specific
  // font size already selected if multiple sizes are highlighted. The
  // Activated event is used here to fix
  // http://bugzilla.gnome.org/show_bug.cgi?id=412404.
  void NoteTextMenu::font_size_activated(Gtk::RadioMenuItem *item)
  {
    if (m_event_freeze)
      return;

    if (!item->get_active())
      return;

    m_buffer->remove_active_tag ("size:huge");
    m_buffer->remove_active_tag ("size:large");
    m_buffer->remove_active_tag ("size:small");

    const char * tag = (const char *)item->get_data(Glib::Quark("Tag"));
    if (tag)
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

  void NoteTextMenu::undo_clicked ()
  {
    if (m_undo_manager.get_can_undo()) {
      DBG_OUT("Running undo...");
      m_undo_manager.undo();
    }
  }

  void NoteTextMenu::redo_clicked ()
  {
    if (m_undo_manager.get_can_redo()) {
      DBG_OUT("Running redo...");
      m_undo_manager.redo();
    }
  }

  void NoteTextMenu::undo_changed ()
  {
    m_undo->set_sensitive(m_undo_manager.get_can_undo());
    m_redo->set_sensitive(m_undo_manager.get_can_redo());
  }


    //
    // Bulleted list handlers
    //
  void  NoteTextMenu::toggle_bullets_clicked()
  {
    m_buffer->toggle_selection_bullets();
  }

  void  NoteTextMenu::increase_indent_clicked()
  {
    m_buffer->increase_cursor_depth();
  }

  void  NoteTextMenu::decrease_indent_clicked()
  {
    m_buffer->decrease_cursor_depth();
  }

}
