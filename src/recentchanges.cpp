/*
 * gnote
 *
 * Copyright (C) 2010-2012 Aurimas Cernius
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/bind.hpp>
#include <glibmm/i18n.h>
#include <gtkmm/image.h>

#include "actionmanager.hpp"
#include "debug.hpp"
#include "gnote.hpp"
#include "iconmanager.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "recentchanges.hpp"


namespace gnote {

  NoteRecentChanges::NoteRecentChanges(NoteManager& m)
    : NoteRecentChangesParent(_("Notes"))
    , m_note_manager(m)
    , m_search_notes_widget(m)
    , m_content_vbox(false, 0)
    , m_mapped(false)
  {
    set_default_size(450,400);
    set_resizable(true);
    if(Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE)->get_boolean(
         Preferences::MAIN_WINDOW_MAXIMIZED)) {
      maximize();
    }

    add_accel_group(ActionManager::obj().get_ui()->get_accel_group());

    set_has_resize_grip(true);

    m_search_notes_widget.signal_open_note
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note));
    m_search_notes_widget.signal_open_note_new_window
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note_new_window));

    Gtk::Toolbar *toolbar = manage(new Gtk::Toolbar);
    m_menu = manage(new Gtk::Menu);
    m_menu->signal_show().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_menu_show));
    utils::ToolMenuButton *tool_button = manage(new utils::ToolMenuButton(
      *manage(new Gtk::Image(IconManager::obj().get_icon(IconManager::NOTE, 24))), _("_Show"), m_menu));
    tool_button->set_use_underline(true);
    tool_button->set_is_important(true);
    tool_button->show_all();
    toolbar->append(*tool_button);
    toolbar->show();

    m_content_vbox.pack_start(*toolbar, false, false, 0);
    m_content_vbox.pack_start(m_embed_box, true, true, 0);
    m_embed_box.show();
    m_content_vbox.show ();

    add (m_content_vbox);
    signal_delete_event().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_delete));
    signal_key_press_event()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_key_pressed)); // For Escape

    embed_widget(m_search_notes_widget);
  }


  NoteRecentChanges::~NoteRecentChanges()
  {
    while(m_embeded_widgets.size()) {
      unembed_widget(**m_embeded_widgets.begin());
    }
  }


  void NoteRecentChanges::present_note(const Note::Ptr & note)
  {
    embed_widget(*note->get_window());
  }


  void NoteRecentChanges::new_note()
  {
    std::vector<Gtk::Widget*> current = m_embed_box.get_children();
    SearchNotesWidget *search_wgt = dynamic_cast<SearchNotesWidget*>(current.size() > 0 ? current[0] : NULL);
    if(search_wgt) {
      search_wgt->new_note();
    }
    else {
      present_note(m_note_manager.create());
    }
  }


  NoteRecentChanges::Ptr NoteRecentChanges::get_owning(Gtk::Widget & widget)
  {
    Ptr owner;
    Gtk::Container *container = widget.get_parent();
    if(!container) {
      try {
        return dynamic_cast<NoteRecentChanges &>(widget).shared_from_this();
      }
      catch(std::bad_cast &) {
        return owner;
      }
    }

    Gtk::Container *cntr = container->get_parent();
    while(cntr) {
      container = cntr;
      cntr = container->get_parent();
    }

    NoteRecentChanges *recent_changes = dynamic_cast<NoteRecentChanges*>(container);
    if(recent_changes) {
      owner = recent_changes->shared_from_this();
    }

    return owner;
  }


  void NoteRecentChanges::on_open_note(const Note::Ptr & note)
  {
    present_note(note);
  }

  void NoteRecentChanges::on_open_note_new_window(const Note::Ptr & note)
  {
    NoteRecentChanges::Ptr window = Gnote::obj().new_main_window();
    window->present();
    window->present_note(note);
  }

  void NoteRecentChanges::on_delete_note()
  {
    m_search_notes_widget.delete_selected_notes();
  }



  void NoteRecentChanges::on_close_window()
  {
    Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE)->set_boolean(
        Preferences::MAIN_WINDOW_MAXIMIZED,
        get_window()->get_state() & Gdk::WINDOW_STATE_MAXIMIZED);
    std::vector<Gtk::Widget*> current = m_embed_box.get_children();
    for(std::vector<Gtk::Widget*>::iterator iter = current.begin();
        iter != current.end(); ++iter) {
      utils::EmbedableWidget *widget = dynamic_cast<utils::EmbedableWidget*>(*iter);
      if(widget) {
        background_embeded(*widget);
      }
    }

    hide();
  }


  bool NoteRecentChanges::on_delete(GdkEventAny *)
  {
    on_close_window();
    return true;
  }

  bool NoteRecentChanges::on_key_pressed(GdkEventKey * ev)
  {
    switch (ev->keyval) {
    case GDK_KEY_Escape:
      // Allow Escape to close the window
      if(&m_search_notes_widget == dynamic_cast<SearchNotesWidget*>(currently_embeded())) {
        on_close_window();
      }
      else {
        utils::EmbedableWidget *current_item = currently_embeded();
        if(current_item) {
          background_embeded(*current_item);
        }
        foreground_embeded(m_search_notes_widget);
      }
      break;
    default:
      break;
    }
    return true;
  }


  void NoteRecentChanges::on_show()
  {
    // Select "All Notes" in the notebooks list
    m_search_notes_widget.select_all_notes_notebook();

    if(m_embed_box.get_children().size() == 0 && m_embeded_widgets.size() > 0) {
      foreground_embeded(**m_embeded_widgets.rbegin());
    }
    std::vector<Gtk::Widget*> embeded = m_embed_box.get_children();
    if(embeded.size() == 1 && embeded.front() == &m_search_notes_widget) {
      m_search_notes_widget.focus_search_entry();
    }
    NoteRecentChangesParent::on_show();
  }

  void NoteRecentChanges::set_search_text(const std::string & value)
  {
    //TODO: handle non-search embeded widgets
    m_search_notes_widget.set_search_text(value);
  }

  void NoteRecentChanges::embed_widget(utils::EmbedableWidget & widget)
  {
    if(std::find(m_embeded_widgets.begin(), m_embeded_widgets.end(), &widget) == m_embeded_widgets.end()) {
      widget.embed(this);
      m_embeded_widgets.push_back(&widget);
    }
    utils::EmbedableWidget *current = currently_embeded();
    if(current && current != &widget) {
      background_embeded(*current);
    }
    foreground_embeded(widget);
  }

  void NoteRecentChanges::unembed_widget(utils::EmbedableWidget & widget)
  {
    bool show_other = false;
    std::list<utils::EmbedableWidget*>::iterator iter = std::find(
      m_embeded_widgets.begin(), m_embeded_widgets.end(), &widget);
    if(iter != m_embeded_widgets.end()) {
      if(is_foreground(**iter)) {
        background_embeded(widget);
        show_other = true;
      }
      m_embeded_widgets.erase(iter);
      widget.unembed();
    }
    if(show_other) {
      if(m_embeded_widgets.size()) {
	foreground_embeded(**m_embeded_widgets.rbegin());
      }
      else if(get_visible()) {
        on_close_window();
      }
    }
  }

  void NoteRecentChanges::foreground_embeded(utils::EmbedableWidget & widget)
  {
    try {
      if(currently_embeded() == &widget) {
        return;
      }
      Gtk::Widget &wid = dynamic_cast<Gtk::Widget&>(widget);
      m_embed_box.pack_start(wid, true, true, 0);
      widget.foreground();
      wid.show();
    }
    catch(std::bad_cast&) {
    }
  }

  void NoteRecentChanges::background_embeded(utils::EmbedableWidget & widget)
  {
    try {
      if(currently_embeded() != &widget) {
        return;
      }
      Gtk::Widget &wid = dynamic_cast<Gtk::Widget&>(widget);
      widget.background();
      m_embed_box.remove(wid);
    }
    catch(std::bad_cast&) {
    }
  }

  bool NoteRecentChanges::is_foreground(utils::EmbedableWidget & widget)
  {
    std::vector<Gtk::Widget*> current = m_embed_box.get_children();
    for(std::vector<Gtk::Widget*>::iterator iter = current.begin();
        iter != current.end(); ++iter) {
      if(dynamic_cast<utils::EmbedableWidget*>(*iter) == &widget) {
        return true;
      }
    }

    return false;
  }

  void NoteRecentChanges::on_embeded_widget_menu_item_toggled(Gtk::RadioMenuItem *item,
                                                              utils::EmbedableWidget * widget)
  {
    if(std::find(m_embeded_widgets.begin(), m_embeded_widgets.end(), widget)
       == m_embeded_widgets.end()) {
      return;
    }
    if(item->get_active()) {
      foreground_embeded(*widget);
    }
    else {
      background_embeded(*widget);
    }
  }

  void NoteRecentChanges::on_menu_show()
  {
    std::vector<Gtk::Widget*> items = m_menu->get_children();
    for(std::vector<Gtk::Widget*>::iterator iter = items.begin(); iter != items.end(); ++iter) {
      m_menu->remove(**iter);
    }

    utils::EmbedableWidget *current = currently_embeded();
    Gtk::RadioMenuItem *active_item = NULL;
    for(std::list<utils::EmbedableWidget*>::iterator iter = m_embeded_widgets.begin();
        iter != m_embeded_widgets.end(); ++iter) {
      Gtk::RadioMenuItem *item = manage(new Gtk::RadioMenuItem(m_tool_menu_group, (*iter)->get_name()));
      item->signal_toggled().connect(
        boost::bind(sigc::mem_fun(*this, &NoteRecentChanges::on_embeded_widget_menu_item_toggled),
                    item, *iter));
      item->show();
      m_menu->append(*item);
      if(*iter == current) {
        active_item = item;
      }
    }

    if(active_item) {
      active_item->set_active(true);
    }
  }

  utils::EmbedableWidget *NoteRecentChanges::currently_embeded()
  {
    std::vector<Gtk::Widget*> children = m_embed_box.get_children();
    return children.size() ? dynamic_cast<utils::EmbedableWidget*>(children[0]) : NULL;
  }

  bool NoteRecentChanges::on_map_event(GdkEventAny *evt)
  {
    bool res = NoteRecentChangesParent::on_map_event(evt);
    m_mapped = true;
    return res;
  }

}

