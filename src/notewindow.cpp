/*
 * gnote
 *
 * Copyright (C) 2011-2024 Aurimas Cernius
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
#include <gtkmm/label.h>
#include <gtkmm/separator.h>
#include <gtkmm/togglebutton.h>

#include "debug.hpp"
#include "addinmanager.hpp"
#include "ignote.hpp"
#include "mainwindow.hpp"
#include "note.hpp"
#include "notewindow.hpp"
#include "notemanager.hpp"
#include "noteeditor.hpp"
#include "preferences.hpp"
#include "utils.hpp"
#include "undo.hpp"
#include "search.hpp"
#include "notebooks/notebookmanager.hpp"
#include "sharp/exception.hpp"
#include "mainwindowaction.hpp"


namespace gnote {

  NoteWindow::NoteWindow(Note & note, IGnote & g)
    : m_note(note)
    , m_gnote(g)
    , m_name(note.get_title())
    , m_height(450)
    , m_width(600)
    , m_find_handler(note)
    , m_enabled(true)
  {
    ITagManager & tag_manager = note.manager().tag_manager();
    m_template_tag = tag_manager.get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG).normalized_name();
    m_template_save_selection_tag = tag_manager.get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SAVE_SELECTION_SYSTEM_TAG).normalized_name();
    m_template_save_title_tag = tag_manager.get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SAVE_TITLE_SYSTEM_TAG).normalized_name();

    set_hexpand(true);
    set_vexpand(true);

    m_template_widget = make_template_bar();

    // The main editor widget
    m_editor = manage(new NoteEditor(note.get_buffer(), g.preferences()));
    m_editor->set_extra_menu(editor_extra_menu());

    // FIXME: I think it would be really nice to let the
    //        window get bigger up till it grows more than
    //        60% of the screen, and then show scrollbars.
    m_editor_window = manage(new Gtk::ScrolledWindow());
    m_editor_window->property_hscrollbar_policy().set_value(Gtk::PolicyType::AUTOMATIC);
    m_editor_window->property_vscrollbar_policy().set_value(Gtk::PolicyType::AUTOMATIC);
    m_editor_window->set_child(*m_editor);
    m_editor_window->set_hexpand(true);
    m_editor_window->set_vexpand(true);

    attach(*m_template_widget, 0, 0, 1, 1);
    attach(*m_editor_window, 0, 1, 1, 1);
    add_shortcuts();
  }


  NoteWindow::~NoteWindow()
  {
    // make sure editor is NULL. See bug 586084
    m_editor = NULL;
  }


  Glib::ustring NoteWindow::get_name() const
  {
    return m_name;
  }

  void NoteWindow::set_name(Glib::ustring && name)
  {
    m_name = std::move(name);
    signal_name_changed(m_name);
  }

  void NoteWindow::foreground()
  {
    //addins may add accelarators, so accel group must be there
    EmbeddableWidgetHost *current_host = host();
    Gtk::Window *parent = dynamic_cast<Gtk::Window*>(current_host);

    EmbeddableWidget::foreground();
    if(parent) {
      parent->set_focus(*m_editor);
    }

    connect_actions(host());
  }

  void NoteWindow::background()
  {
    EmbeddableWidget::background();
    Gtk::Window *parent = dynamic_cast<Gtk::Window*>(host());
    if(!parent) {
      return;
    }
    if(!parent->is_maximized()) {
      int cur_width = parent->get_width();
      int cur_height = parent->get_height();;

      if(!(m_note.data().width() == cur_width && m_note.data().height() == cur_height)) {
        m_note.data().set_extent(cur_width, cur_height);
        m_width = cur_width;
        m_height = cur_height;

        DBG_OUT("WindowConfigureEvent queueing save");
        m_note.queue_save(NO_CHANGE);
      }
    }

    m_note.save();  // to update not title immediately in notes list
    disconnect_actions();
  }

  void NoteWindow::connect_actions(EmbeddableWidgetHost *host)
  {
    // Don't allow deleting the "Start Here" note...
    if(!m_note.is_special()) {
      m_signal_cids.push_back(host->find_action("delete-note")->signal_activate()
        .connect(sigc::mem_fun(*this, &NoteWindow::on_delete_button_clicked)));
    }

    MainWindowAction::Ptr important_action = host->find_action("important-note");
    important_action->set_state(Glib::Variant<bool>::create(m_note.is_pinned()));
    m_signal_cids.push_back(important_action->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteWindow::on_pin_button_clicked)));
    m_signal_cids.push_back(m_gnote.notebook_manager().signal_note_pin_status_changed
      .connect(sigc::mem_fun(*this, &NoteWindow::on_pin_status_changed)));

    m_signal_cids.push_back(host->find_action("undo")->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteWindow::undo_clicked)));
    m_signal_cids.push_back(host->find_action("redo")->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteWindow::redo_clicked)));
    m_signal_cids.push_back(host->find_action("link")->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteWindow::link_clicked)));
    m_signal_cids.push_back(host->find_action("change-font-bold")->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteWindow::bold_clicked)));
    m_signal_cids.push_back(host->find_action("change-font-italic")->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteWindow::italic_clicked)));
    m_signal_cids.push_back(host->find_action("change-font-strikeout")->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteWindow::strikeout_clicked)));
    m_signal_cids.push_back(host->find_action("change-font-highlight")->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteWindow::highlight_clicked)));
    m_signal_cids.push_back(host->find_action("change-font-size")->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteWindow::font_size_activated)));
    m_signal_cids.push_back(host->find_action("increase-indent")->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteWindow::increase_indent_clicked)));
    m_signal_cids.push_back(host->find_action("decrease-indent")->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteWindow::decrease_indent_clicked)));
  }

  void NoteWindow::disconnect_actions()
  {
    for(auto & cid : m_signal_cids) {
      cid.disconnect();
    }
    m_signal_cids.clear();
  }

  void NoteWindow::size_internals()
  {
    m_editor->scroll_to(m_editor->get_buffer()->get_insert());
  }

  void NoteWindow::set_initial_focus()
  {
    m_editor->grab_focus();
  }

  void NoteWindow::add_shortcuts()
  {
    auto controller = Gtk::ShortcutController::create();
    controller->set_scope(Gtk::ShortcutScope::GLOBAL);
    add_controller(controller);
    m_shortcut_controller = controller;

    // Open Help (F1)
    {
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_F1);
      auto action = Gtk::CallbackAction::create(sigc::mem_fun(*this, &NoteWindow::open_help_activate));
      auto shortcut = Gtk::Shortcut::create(trigger, action);
      controller->add_shortcut(shortcut);
    }

    {
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_Z, Gdk::ModifierType::CONTROL_MASK);
      auto action = Gtk::NamedAction::create("win.undo");
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
    }

    {
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_Z, Gdk::ModifierType::CONTROL_MASK | Gdk::ModifierType::SHIFT_MASK);
      auto action = Gtk::NamedAction::create("win.redo");
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
    }

    {
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_L, Gdk::ModifierType::CONTROL_MASK);
      auto action = Gtk::NamedAction::create("win.link");
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
    }

    {
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_B, Gdk::ModifierType::CONTROL_MASK);
      auto action = Gtk::NamedAction::create("win.change-font-bold");
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
    }

    {
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_I, Gdk::ModifierType::CONTROL_MASK);
      auto action = Gtk::NamedAction::create("win.change-font-italic");
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
    }

    {
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_S, Gdk::ModifierType::CONTROL_MASK);
      auto action = Gtk::NamedAction::create("win.change-font-strikeout");
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
    }

    {
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_H, Gdk::ModifierType::CONTROL_MASK);
      auto action = Gtk::NamedAction::create("win.change-font-highlight");
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
    }

    {
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_plus, Gdk::ModifierType::CONTROL_MASK);
      auto action = Gtk::CallbackAction::create(sigc::mem_fun(*this, &NoteWindow::increase_font_clicked));
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
      trigger = Gtk::KeyvalTrigger::create(GDK_KEY_KP_Add, Gdk::ModifierType::CONTROL_MASK);
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
    }

    {
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_minus, Gdk::ModifierType::CONTROL_MASK);
      auto action = Gtk::CallbackAction::create(sigc::mem_fun(*this, &NoteWindow::decrease_font_clicked));
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
      trigger = Gtk::KeyvalTrigger::create(GDK_KEY_KP_Subtract, Gdk::ModifierType::CONTROL_MASK);
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
    }

    {
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_Right, Gdk::ModifierType::ALT_MASK);
      auto action = Gtk::NamedAction::create("win.increase-indent");
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
    }

    {
      auto trigger = Gtk::KeyvalTrigger::create(GDK_KEY_Left, Gdk::ModifierType::ALT_MASK);
      auto action = Gtk::NamedAction::create("win.decrease-indent");
      controller->add_shortcut(Gtk::Shortcut::create(trigger, action));
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
    return manage(make_toolbar());
  }

  std::vector<PopoverWidget> NoteWindow::get_popover_widgets()
  {
    undo_changed();
    std::vector<PopoverWidget> popover_widgets;
    popover_widgets.reserve(20);

    auto undo = Gio::MenuItem::create(_("_Undo"), "win.undo");
    popover_widgets.push_back(PopoverWidget(NOTE_SECTION_UNDO, 1, undo));
    auto redo = Gio::MenuItem::create(_("_Redo"), "win.redo");
    popover_widgets.push_back(PopoverWidget(NOTE_SECTION_UNDO, 2, redo));
    auto link = Gio::MenuItem::create(_("_Link to New Note"), "win.link");
    popover_widgets.push_back(PopoverWidget::create_for_note(LINK_ORDER, link));
    auto important = Gio::MenuItem::create(C_("NoteActions", "_Important"), "win.important-note");
    popover_widgets.push_back(PopoverWidget(NOTE_SECTION_FLAGS, IMPORTANT_ORDER, important));

    NoteManager & manager = static_cast<NoteManager&>(m_note.manager());
    for(NoteAddin *addin : manager.get_addin_manager().get_note_addins(m_note)) {
      auto addin_widgets = addin->get_actions_popover_widgets();
      popover_widgets.insert(popover_widgets.end(), addin_widgets.begin(), addin_widgets.end());
    }

    auto delete_button = Gio::MenuItem::create(_("_Delete…"), "win.delete-note");
    popover_widgets.push_back(PopoverWidget(NOTE_SECTION_ACTIONS, 1000, delete_button));

    return popover_widgets;
  }

    // Delete this Note.
    //

  void NoteWindow::on_delete_button_clicked(const Glib::VariantBase&)
  {
    // Prompt for note deletion
    if(auto host_window = dynamic_cast<Gtk::Window*>(host())) {
      std::vector<NoteBase::Ref> single_note_list;
      single_note_list.push_back(m_note);
      noteutils::show_deletion_dialog(single_note_list, *host_window);
    }
  }

  Glib::RefPtr<Gio::MenuModel> NoteWindow::editor_extra_menu()
  {
    auto menu = Gio::Menu::create();
    menu->append(_("_Link to New Note"), "win.link");
    return menu;
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
    text_button->property_icon_name() = "insert-text-symbolic";
    text_button->signal_clicked().connect([this, text_button] { on_text_button_clicked(text_button); });
    text_button->property_margin_start() = 12;
    grid->attach(*text_button, grid_col++, 0, 1, 1);
    text_button->set_tooltip_text(_("Set properties of text"));

    grid->property_margin_start() = 12;
    return grid;
  }


  Gtk::Grid * NoteWindow::make_template_bar()
  {
    Gtk::Grid * bar = manage(new Gtk::Grid);

    Gtk::Label * infoLabel = manage(new Gtk::Label(
      _("This note is a template note. It determines the default content of regular notes, and will not show up in the note menu or search window.")));
    infoLabel->set_wrap(true);

    Gtk::Button * untemplateButton = manage(new Gtk::Button(_("Convert to regular note")));
    untemplateButton->signal_clicked().connect(sigc::mem_fun(*this, &NoteWindow::on_untemplate_button_click));

    m_save_selection_check_button = manage(new Gtk::CheckButton(_("Save Se_lection"), true));
    m_save_selection_check_button->set_active(m_note.contains_tag(template_save_selection_tag()));
    m_save_selection_check_button->signal_toggled().connect(sigc::mem_fun(*this, &NoteWindow::on_save_selection_check_button_toggled));

    m_save_title_check_button = manage(new Gtk::CheckButton(_("Save _Title"), true));
    m_save_title_check_button->set_active(m_note.contains_tag(template_save_title_tag()));
    m_save_title_check_button->signal_toggled().connect(sigc::mem_fun(*this, &NoteWindow::on_save_title_check_button_toggled));

    bar->attach(*infoLabel, 0, 0, 1, 1);
    bar->attach(*untemplateButton, 0, 1, 1, 1);
    bar->attach(*m_save_selection_check_button, 0, 2, 1, 1);
    bar->attach(*m_save_title_check_button, 0, 3, 1, 1);

    if(auto template_tag = m_note.manager().tag_manager().get_tag(m_template_tag)) {
      if(!m_note.contains_tag(*template_tag)) {
        bar->hide();
      }
    }

    m_note.signal_tag_added.connect(sigc::mem_fun(*this, &NoteWindow::on_note_tag_added));
    m_note.signal_tag_removed.connect(sigc::mem_fun(*this, &NoteWindow::on_note_tag_removed));

    return bar;
  }


  void NoteWindow::on_untemplate_button_click()
  {
    if(auto template_tag = m_note.manager().tag_manager().get_tag(m_template_tag)) {
      m_note.remove_tag(*template_tag);
    }
  }


  void NoteWindow::on_save_selection_check_button_toggled()
  {
    if(m_save_selection_check_button->get_active()) {
      m_note.add_tag(template_save_selection_tag());
    }
    else {
      m_note.remove_tag(template_save_selection_tag());
    }
  }


  void NoteWindow::on_save_title_check_button_toggled()
  {
    if(m_save_title_check_button->get_active()) {
      m_note.add_tag(template_save_title_tag());
    }
    else {
      m_note.remove_tag(template_save_title_tag());
    }
  }


  void NoteWindow::on_note_tag_added(const NoteBase&, const Tag &tag)
  {
    if(tag.normalized_name() == m_template_tag) {
      m_template_widget->show();
    }
  }


  void NoteWindow::on_note_tag_removed(const NoteBase&, const Glib::ustring & tag)
  {
    if(tag == m_template_tag) {
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

    auto match = m_note.manager().find(title);
    if(!match) {
      try {
        match = NoteBase::Ref(std::ref(m_note.manager().create(std::move(select))));
      } 
      catch (const sharp::Exception & e) {
        auto dialog = Gtk::make_managed<utils::HIGMessageDialog>(dynamic_cast<Gtk::Window*>(host()),
          GTK_DIALOG_DESTROY_WITH_PARENT,
          Gtk::MessageType::ERROR, Gtk::ButtonsType::OK,
          _("Cannot create note"), e.what());
        dialog->show();
        return;
      }
    }
    else {
      Gtk::TextIter start, end;
      m_note.get_buffer()->get_selection_bounds(start, end);
      m_note.get_buffer()->remove_tag(m_note.get_tag_table()->get_broken_link_tag(), start, end);
      m_note.get_buffer()->apply_tag(m_note.get_tag_table()->get_link_tag(), start, end);
    }

    MainWindow::present_in(*dynamic_cast<MainWindow*>(host()), static_cast<Note&>(match.value().get()));
  }

  bool NoteWindow::open_help_activate(Gtk::Widget&, const Glib::VariantBase&)
  {
    utils::show_help("gnote", "editing-notes", *dynamic_cast<Gtk::Window*>(host()));
    return true;
  }

  void NoteWindow::change_depth_right_handler()
  {
    std::static_pointer_cast<NoteBuffer>(m_editor->get_buffer())->change_cursor_depth_directional(true);
  }

  void NoteWindow::change_depth_left_handler()
  {
    std::static_pointer_cast<NoteBuffer>(m_editor->get_buffer())->change_cursor_depth_directional(false);
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

  void NoteWindow::on_text_button_clicked(Gtk::Widget *parent)
  {
    auto text_menu = Gtk::make_managed<NoteTextMenu>(*this, m_note.get_buffer(), m_gnote.preferences());
    text_menu->set_parent(*parent);
    utils::unparent_popover_on_close(text_menu);
    signal_build_text_menu(*text_menu);
    text_menu->popup();
  }

  void NoteWindow::enabled(bool enable)
  {
    m_enabled = enable;
    m_editor->set_editable(m_enabled);
    embeddable_toolbar()->set_sensitive(m_enabled);
  }

  void NoteWindow::undo_clicked(const Glib::VariantBase&)
  {
    auto & undo_manager = m_note.get_buffer()->undoer();
    if(undo_manager.get_can_undo()) {
      DBG_OUT("Running undo...");
      undo_manager.undo();
    }
  }

  void NoteWindow::redo_clicked(const Glib::VariantBase&)
  {
    auto & undo_manager = m_note.get_buffer()->undoer();
    if(undo_manager.get_can_redo()) {
      DBG_OUT("Running redo...");
      undo_manager.redo();
    }
  }

  void NoteWindow::link_clicked(const Glib::VariantBase&)
  {
    auto & buffer = m_note.get_buffer();
    Glib::ustring select = buffer->get_selection();
    if(select.empty()) {
      return;
    }

    Glib::ustring body_unused;
    Glib::ustring title = NoteManagerBase::split_title_from_content(select, body_unused);
    if(title.empty()) {
      return;
    }

    NoteManagerBase & manager(m_note.manager());
    auto match = manager.find(title);
    if(!match) {
      try {
        match = NoteBase::Ref(std::ref(manager.create(std::move(select))));
      }
      catch(const sharp::Exception & e) {
        auto dialog = Gtk::make_managed<utils::HIGMessageDialog>(dynamic_cast<Gtk::Window*>(m_note.get_window()->host()),
          GTK_DIALOG_DESTROY_WITH_PARENT,
          Gtk::MessageType::ERROR,  Gtk::ButtonsType::OK,
          _("Cannot create note"), e.what());
        dialog->show();
        return;
      }
    }
    else {
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      buffer->remove_tag(m_note.get_tag_table()->get_broken_link_tag(), start, end);
      buffer->apply_tag(m_note.get_tag_table()->get_link_tag(), start, end);
    }

    MainWindow::present_in(*dynamic_cast<MainWindow*>(m_note.get_window()->host()), static_cast<Note&>(match.value().get()));
  }

  //
  // Font-style menu item activate
  //
  // Toggle the style tag for the current text.  Style tags are
  // stored in a "Tag" member of the menuitem's Data.
  //
  void NoteWindow::font_style_clicked(const char * tag)
  {
    if(tag) {
      m_note.get_buffer()->toggle_active_tag(tag);
    }
  }

  void NoteWindow::bold_clicked(const Glib::VariantBase & state)
  {
    host()->find_action("change-font-bold")->set_state(state);
    font_style_clicked("bold");
  }

  void NoteWindow::italic_clicked(const Glib::VariantBase & state)
  {
    host()->find_action("change-font-italic")->set_state(state);
    font_style_clicked("italic");
  }

  void NoteWindow::strikeout_clicked(const Glib::VariantBase & state)
  {
    host()->find_action("change-font-strikeout")->set_state(state);
    font_style_clicked("strikethrough");
  }

  void NoteWindow::highlight_clicked(const Glib::VariantBase & state)
  {
    host()->find_action("change-font-highlight")->set_state(state);
    font_style_clicked("highlight");
  }

  void NoteWindow::font_size_activated(const Glib::VariantBase & state)
  {
    auto host = this->host();
    if(host == NULL) {
      return;
    }
    host->find_action("change-font-size")->set_state(state);

    auto & buffer = m_note.get_buffer();
    buffer->remove_active_tag ("size:huge");
    buffer->remove_active_tag ("size:large");
    buffer->remove_active_tag ("size:small");

    auto tag = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(state).get();
    if(!tag.empty())
      buffer->set_active_tag(tag);
  }

  void NoteWindow::toggle_bullets_clicked(const Glib::VariantBase&)
  {
    m_note.get_buffer()->toggle_selection_bullets();
  }

  void NoteWindow::increase_indent_clicked(const Glib::VariantBase&)
  {
    m_note.get_buffer()->increase_cursor_depth();
    if(auto h = host()) {
      h->find_action("decrease-indent")->property_enabled() = true;
    }
  }

  void NoteWindow::decrease_indent_clicked(const Glib::VariantBase&)
  {
    auto & buffer = m_note.get_buffer();
    buffer->decrease_cursor_depth();
    if(auto h = host()) {
      h->find_action("decrease-indent")->property_enabled() = buffer->is_bulleted_list_active();
    }
  }

  bool NoteWindow::increase_font_clicked(Gtk::Widget&, const Glib::VariantBase&)
  {
    auto & buffer = m_note.get_buffer();
    if(buffer->is_active_tag("size:small")) {
      buffer->remove_active_tag("size:small");
    }
    else if(buffer->is_active_tag("size:large")) {
      buffer->remove_active_tag("size:large");
      buffer->set_active_tag("size:huge");
    }
    else if(buffer->is_active_tag("size:huge")) {
      // Maximum font size, do nothing
    }
    else {
      // Current font size is normal
      buffer->set_active_tag("size:large");
    }

   return true;
 }

  bool NoteWindow::decrease_font_clicked(Gtk::Widget&, const Glib::VariantBase&)
  {
    auto & buffer = m_note.get_buffer();
    if(buffer->is_active_tag("size:small")) {
// Minimum font size, do nothing
    }
    else if(buffer->is_active_tag("size:large")) {
      buffer->remove_active_tag("size:large");
    }
    else if(buffer->is_active_tag("size:huge")) {
      buffer->remove_active_tag("size:huge");
      buffer->set_active_tag("size:large");
    }
    else {
// Current font size is normal
      buffer->set_active_tag("size:small");
    }

    return true;
  }

  void NoteWindow::undo_changed()
  {
    EmbeddableWidgetHost *host = this->host();
    if(host == NULL) {
      return;
    }

    auto & undo_manager = m_note.get_buffer()->undoer();
    host->find_action("undo")->property_enabled() = undo_manager.get_can_undo();
    host->find_action("redo")->property_enabled() = undo_manager.get_can_redo();
  }

  Tag &NoteWindow::template_save_selection_tag()
  {
    if(auto tag = m_note.manager().tag_manager().get_tag(m_template_save_selection_tag)) {
      return *tag;
    }

    throw std::runtime_error("No save selection tag found");
  }

  Tag &NoteWindow::template_save_title_tag()
  {
    if(auto tag = m_note.manager().tag_manager().get_tag(m_template_save_title_tag)) {
      return *tag;
    }

    throw std::runtime_error("No save title tag found");
  }


  NoteFindHandler::NoteFindHandler(Note & note)
    : m_note(note)
  {
  }

  bool NoteFindHandler::goto_previous_result()
  {
    if (m_current_matches.empty() || m_current_matches.size() == 0)
      return false;

    Match *previous_match = nullptr;
    for (auto & match : m_current_matches) {
      Glib::RefPtr<NoteBuffer> buffer = match.buffer;
      Gtk::TextIter selection_start, selection_end;
      buffer->get_selection_bounds(selection_start, selection_end);
      Gtk::TextIter end = buffer->get_iter_at_mark(match.start_mark);

      if (end.get_offset() < selection_start.get_offset()) {
        previous_match = &match;
      }
      else {
        break;
      }
    }
    if(previous_match) {
      jump_to_match(*previous_match);
      return true;
    }

    return false;
  }

  bool NoteFindHandler::goto_next_result()
  {
    if (m_current_matches.empty() || m_current_matches.size() == 0)
      return false;

    for (auto & match : m_current_matches) {
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

    for(auto & match : m_current_matches) {
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

      for(auto & match : m_current_matches) {
        match.buffer->delete_mark(match.start_mark);
        match.buffer->delete_mark(match.end_mark);
      }

      m_current_matches.clear();
    }
  }



  void NoteFindHandler::find_matches_in_buffer(const Glib::RefPtr<NoteBuffer> & buffer, 
                                               const std::vector<Glib::ustring> & words,
                                               std::vector<NoteFindHandler::Match> & matches)
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
  NoteTextMenu::NoteTextMenu(EmbeddableWidget & widget, const Glib::RefPtr<NoteBuffer> & buffer, Preferences &prefs)
    : Gtk::Popover()
    {
      set_position(Gtk::PositionType::BOTTOM);
      auto menu_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);

      auto font_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
      font_box->set_name("font-box");
      Gtk::Widget *bold = create_font_item("win.change-font-bold", "format-text-bold-symbolic");
      Gtk::Widget *italic = create_font_item("win.change-font-italic", "format-text-italic-symbolic");
      Gtk::Widget *strikeout = create_font_item("win.change-font-strikeout", "format-text-strikethrough-symbolic");
      font_box->append(*bold);
      font_box->append(*italic);
      font_box->append(*strikeout);

      auto highlight = Gtk::make_managed<Gtk::ToggleButton>();
      highlight->set_action_name("win.change-font-highlight");
      highlight->set_has_frame(false);
      auto label = Gtk::make_managed<Gtk::Label>();
      Glib::ustring markup = Glib::ustring::compose("<span color=\"%1\" background=\"%2\">%3</span>",
        prefs.highlight_foreground_color(), prefs.highlight_background_color(), _("_Highlight"));
      label->set_markup_with_mnemonic(markup);
      highlight->set_child(*label);

      Gtk::Widget *normal = create_font_size_item(_("_Normal"), NULL, "");
      Gtk::Widget *small = create_font_size_item(_("S_mall"), "small", "size:small");
      Gtk::Widget *large = create_font_size_item(_("_Large"), "large", "size:large");
      Gtk::Widget *huge = create_font_size_item(_("Hu_ge"), "x-large", "size:huge");

      auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
      box->set_name("formatting");
      box->append(*font_box);
      box->append(*highlight);
      menu_box->append(*box);
      menu_box->append(*Gtk::make_managed<Gtk::Separator>());

      box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
      box->set_name("font-size");
      box->append(*small);
      box->append(*normal);
      box->append(*large);
      box->append(*huge);
      menu_box->append(*box);
      menu_box->append(*Gtk::make_managed<Gtk::Separator>());

      box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
      box->set_name("indentation");
      auto increase_indent = Gtk::make_managed<Gtk::Button>();
      increase_indent->set_icon_name("format-indent-more-symbolic");
      increase_indent->set_action_name("win.increase-indent");
      increase_indent->set_has_frame(false);
      box->append(*increase_indent);
      auto decrease_indent = Gtk::make_managed<Gtk::Button>();
      decrease_indent->set_icon_name("format-indent-less-symbolic");
      decrease_indent->set_action_name("win.decrease-indent");
      decrease_indent->set_has_frame(false);
      box->append(*decrease_indent);
      menu_box->append(*box);

      set_child(*menu_box);

      refresh_state(widget, buffer);
    }

  Gtk::Widget *NoteTextMenu::create_font_item(const char *action, const char *icon_name)
  {
    auto widget = Gtk::make_managed<Gtk::ToggleButton>();
    widget->set_action_name(action);
    widget->set_icon_name(icon_name);
    widget->set_has_frame(false);
    return widget;
  }

  Gtk::Widget *NoteTextMenu::create_font_size_item(const char *label, const char *markup, const char *size)
  {
    auto item = Gtk::make_managed<Gtk::ToggleButton>();
    item->set_action_name("win.change-font-size");
    item->set_action_target_value(Glib::Variant<Glib::ustring>::create(size));
    item->set_has_frame(false);
    auto lbl = Gtk::make_managed<Gtk::Label>();
    Glib::ustring mrkp;
    if(markup != NULL) {
      mrkp = Glib::ustring::compose("<span size=\"%1\">%2</span>", markup, label);
    }
    else {
      mrkp = label;
    }
    lbl->set_markup_with_mnemonic(mrkp);
    item->set_child(*lbl);
    return item;
  }

  void NoteTextMenu::refresh_sizing_state(EmbeddableWidget & widget, const Glib::RefPtr<NoteBuffer> & buffer)
  {
    EmbeddableWidgetHost *host = widget.host();
    if(host == NULL) {
      return;
    }
    auto action = host->find_action("change-font-size");
    Gtk::TextIter cursor = buffer->get_iter_at_mark(buffer->get_insert());
    Gtk::TextIter selection = buffer->get_iter_at_mark(buffer->get_selection_bound());

    // When on title line, activate the hidden menu item
    if ((cursor.get_line() == 0) || (selection.get_line() == 0)) {
      action->set_enabled(false);
      return;
    }

    action->set_enabled(true);
    if(buffer->is_active_tag ("size:huge")) {
      action->set_state(Glib::Variant<Glib::ustring>::create("size:huge"));
    }
    else if(buffer->is_active_tag ("size:large")) {
      action->set_state(Glib::Variant<Glib::ustring>::create("size:large"));
    }
    else if(buffer->is_active_tag ("size:small")) {
      action->set_state(Glib::Variant<Glib::ustring>::create("size:small"));
    }
    else {
      action->set_state(Glib::Variant<Glib::ustring>::create(""));
    }
  }

  void NoteTextMenu::refresh_state(EmbeddableWidget & widget, const Glib::RefPtr<NoteBuffer> & buffer)
  {
    EmbeddableWidgetHost *host = widget.host();
    if(host == NULL) {
      return;
    }

    Gtk::TextIter start, end;
    host->find_action("link")->property_enabled() = buffer->get_selection_bounds(start, end);
    host->find_action("change-font-bold")->set_state(Glib::Variant<bool>::create(buffer->is_active_tag("bold")));
    host->find_action("change-font-italic")->set_state(Glib::Variant<bool>::create(buffer->is_active_tag("italic")));
    host->find_action("change-font-strikeout")->set_state(Glib::Variant<bool>::create(buffer->is_active_tag("strikethrough")));
    host->find_action("change-font-highlight")->set_state(Glib::Variant<bool>::create(buffer->is_active_tag("highlight")));

    host->find_action("decrease-indent")->property_enabled() = buffer->is_bulleted_list_active();

    refresh_sizing_state(widget, buffer);
  }
}
