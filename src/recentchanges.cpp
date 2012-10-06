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
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "recentchanges.hpp"


namespace gnote {

  NoteRecentChanges::NoteRecentChanges(NoteManager& m)
    : NoteRecentChangesParent(_("Notes"))
    , m_search_notes_widget(m)
    , m_content_vbox(false, 0)
  {
    set_default_size(450,400);
    set_resizable(true);

    add_accel_group(ActionManager::obj().get_ui()->get_accel_group());

    set_has_resize_grip(true);

    m_search_notes_widget.signal_open_note
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note));
    m_search_notes_widget.signal_open_note_new_window
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note_new_window));

    Gtk::Toolbar *toolbar = manage(new Gtk::Toolbar);
    m_menu = manage(new Gtk::Menu);
    utils::ToolMenuButton *tool_button = manage(new utils::ToolMenuButton(
      *manage(new Gtk::Image(utils::get_icon("note", 24))), _("_Show"), m_menu));
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
    m_embeded_widgets[&m_search_notes_widget]->toggled(); //need initial event to show search widget
  }


  NoteRecentChanges::~NoteRecentChanges()
  {
    while(m_embeded_widgets.size()) {
      unembed_widget(*m_embeded_widgets.begin()->first);
    }
  }


  void NoteRecentChanges::present_note(const Note::Ptr & note)
  {
    embed_widget(*note->get_window());
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
      on_close_window ();
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
      foreground_embeded(*m_embeded_widgets.rbegin()->first);
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
    if(m_embeded_widgets.find(&widget) == m_embeded_widgets.end()) {
      Gtk::RadioMenuItem *item = new Gtk::RadioMenuItem(m_tool_menu_group, widget.get_name());
      item->signal_toggled().connect(
        boost::bind(sigc::mem_fun(*this, &NoteRecentChanges::on_embeded_widget_menu_item_toggled),
                    &widget));
      item->show();
      m_menu->append(*item);
      m_embeded_widgets[&widget] = item;
      widget.embed(this);
    }
    foreground_embeded(widget);
  }

  void NoteRecentChanges::unembed_widget(utils::EmbedableWidget & widget)
  {
    bool show_other = false;
    std::map<utils::EmbedableWidget*, Gtk::RadioMenuItem*>::iterator iter = m_embeded_widgets.find(&widget);
    if(iter != m_embeded_widgets.end()) {
      if(is_foreground(*iter->first)) {
        background_embeded(widget);
        show_other = true;
      }
      m_menu->remove(*iter->second);
      delete iter->second;
      m_embeded_widgets.erase(iter);
      widget.unembed();
    }
    if(show_other) {
      if(m_embeded_widgets.size()) {
	foreground_embeded(*m_embeded_widgets.rbegin()->first);
      }
      else if(get_visible()) {
        on_close_window();
      }
    }
  }

  void NoteRecentChanges::foreground_embeded(utils::EmbedableWidget & widget)
  {
    m_embeded_widgets[&widget]->set_active(true);
  }

  void NoteRecentChanges::background_embeded(utils::EmbedableWidget & widget)
  {
    if(get_visible()) {
      m_embeded_widgets[&widget]->set_active(false);
    }
    else {
      widget.background();
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

  void NoteRecentChanges::on_embeded_widget_menu_item_toggled(utils::EmbedableWidget * widget)
  {
    Gtk::Widget *wid = dynamic_cast<Gtk::Widget*>(widget);
    if(!wid) {
      ERR_OUT("Not an embedable widget!");
      return;
    }
    Gtk::RadioMenuItem *item = m_embeded_widgets[widget];
    if(!item) {
      ERR_OUT("Widget has no associated menu item!");
      return;
    }
    if(item->get_active()) {
      m_embed_box.pack_start(*wid, true, true, 0);
      widget->foreground();
      wid->show();
    }
    else {
      widget->background();
      m_embed_box.remove(*wid);
    }
  }

}

