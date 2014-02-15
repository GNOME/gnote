/*
 * gnote
 *
 * Copyright (C) 2012-2014 Aurimas Cernius
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
#include <gtkmm/stock.h>

#include "ignote.hpp"
#include "iconmanager.hpp"
#include "note.hpp"
#include "notewindow.hpp"

#include "backlinkmenuitem.hpp"

namespace backlinks {

Glib::RefPtr<Gtk::Action> BacklinkAction::create(const sigc::slot<void, Gtk::Menu*> & slot)
{
  return Glib::RefPtr<Gtk::Action>(new BacklinkAction(slot));
}

BacklinkAction::BacklinkAction(const sigc::slot<void, Gtk::Menu*> & slot)
  : gnote::NoteWindow::NonModifyingAction("BacklinkAction", Gtk::Stock::JUMP_TO,
                                          _("What links here?"),
                                          _("Which notes have links to here?"))
  , m_update_menu_slot(slot)
{
}

Gtk::Widget *BacklinkAction::create_menu_item_vfunc()
{
  m_submenu_built = false;
  Gtk::MenuItem *menu_item = new Gtk::ImageMenuItem;
  m_menu = manage(new Gtk::Menu);
  m_menu->signal_hide().connect(
    sigc::mem_fun(*this, &BacklinkAction::on_menu_hidden));
  menu_item->set_submenu(*m_menu);
  return menu_item;
}

void BacklinkAction::on_activate()
{
  Gtk::Action::on_activate();
  if(m_submenu_built) {
    return;
  }
  update_menu();
}

void BacklinkAction::on_menu_hidden()
{
  m_submenu_built = false;
}

void BacklinkAction::update_menu()
{
  m_update_menu_slot(m_menu);
  m_submenu_built = true;
}


Glib::RefPtr<Gdk::Pixbuf> BacklinkMenuItem::get_note_icon()
{
  return gnote::IconManager::obj().get_icon(gnote::IconManager::NOTE, 16);
}


BacklinkMenuItem::BacklinkMenuItem(const gnote::NoteBase::Ptr & note,
                                   const std::string & title_search)
  : Gtk::ImageMenuItem(note->get_title())
  , m_note(note)
  , m_title_search(title_search)
{
  set_image(*manage(new Gtk::Image(get_note_icon())));
}


void BacklinkMenuItem::on_activate()
{
  if (!m_note) {
    return;
  }

  gnote::MainWindow::present_in_new_window(static_pointer_cast<gnote::Note>(m_note),
    gnote::Preferences::obj().get_schema_settings(gnote::Preferences::SCHEMA_GNOTE)->
      get_boolean(gnote::Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE));
}


}
