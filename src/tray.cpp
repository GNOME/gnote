/*
 * gnote
 *
 * Copyright (C) 2010-2013 Aurimas Cernius
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

#include <boost/format.hpp>

#include <glibmm/i18n.h>
#include <gtkmm/main.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/stock.h>


#include "tray.hpp"
#include "debug.hpp"
#include "actionmanager.hpp"
#include "iconmanager.hpp"
#include "utils.hpp"
#include "ignote.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "prefskeybinder.hpp"
#include "tag.hpp"
#include "itagmanager.hpp"
#include "preferences.hpp"
#include "sharp/datetime.hpp"
#include "sharp/string.hpp"

namespace gnote {

  std::string tray_util_get_tooltip_text()
  {
    std::string tip_text = _("Take notes");

#ifdef HAVE_X11_SUPPORT
    if (Preferences::obj().get_schema_settings(
            Preferences::SCHEMA_GNOTE)->get_boolean(Preferences::ENABLE_KEYBINDINGS)) {
      std::string shortcut = KeybindingToAccel::get_shortcut(Preferences::KEYBINDING_SHOW_NOTE_MENU);
      if (!shortcut.empty())
        tip_text += str(boost::format(" (%1%)") % shortcut);
    }
#endif

    return tip_text;
  }



  NoteMenuItem::NoteMenuItem(const Note::Ptr & note, bool show_pin)
    : Gtk::ImageMenuItem(get_display_name(note))
    , m_note(note)
    , m_pin_img(NULL)
    , m_pinned(false)
    , m_inhibit_activate(false)
  {
    set_image(*manage(new Gtk::Image(IconManager::obj().get_icon(IconManager::NOTE, 16))));
    if(show_pin) {
      Gtk::Grid *box = manage(new Gtk::Grid);
      Gtk::Widget *child = get_child();
      Gtk::Container::remove(*child);
      child->set_hexpand(true);
      box->attach(*child, 0, 0, 1, 1);
      add(*box);
      box->show();

      m_pinned = note->is_pinned();
      m_pin_img = manage(new Gtk::Image(m_pinned ? get_pindown_icon() : get_pinup_icon()));
      m_pin_img->show();
      box->attach(*m_pin_img, 1, 0, 1, 1);
    }
  }

  void NoteMenuItem::on_activate()
  {
    if(!m_inhibit_activate) {
      if(m_note) {
        MainWindow::present_in_new_window(m_note,
          Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE)->get_boolean(
            Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE));
      }
    }
  }

  bool NoteMenuItem::on_button_press_event(GdkEventButton *ev)
  {
    if (m_pin_img && (ev->x >= m_pin_img->get_allocation().get_x()) 
        && (ev->x < m_pin_img->get_allocation().get_x() 
            + m_pin_img->get_allocation().get_width())) {
      m_pinned = !m_pinned;
      m_note->set_pinned(m_pinned);
      m_pin_img->set(m_pinned ? get_pindown_icon() : get_pinup_icon());
      m_inhibit_activate = true;
      return true;
    }

    return Gtk::ImageMenuItem::on_button_press_event(ev);
  }


  bool NoteMenuItem::on_button_release_event(GdkEventButton *ev)
  {
    if (m_inhibit_activate) {
      m_inhibit_activate = false;
      return true;
    }
    return Gtk::ImageMenuItem::on_button_release_event(ev);
  }


  bool NoteMenuItem::on_motion_notify_event(GdkEventMotion *ev)
  {
    if (!m_pinned && m_pin_img) {
      if (ev->x >= m_pin_img->get_allocation().get_x() &&
          ev->x < m_pin_img->get_allocation().get_x() + m_pin_img->get_allocation().get_width()) {
        Glib::RefPtr<Gdk::Pixbuf> pin_active = IconManager::obj().get_icon(IconManager::PIN_ACTIVE, 16);
        if (m_pin_img->get_pixbuf() != pin_active) {
          m_pin_img->set(pin_active);
        }
      }
      else {
        Glib::RefPtr<Gdk::Pixbuf> pinup = get_pinup_icon();
        if (m_pin_img->get_pixbuf() != pinup) {
          m_pin_img->set(pinup);
        }
      }
    }

    return Gtk::ImageMenuItem::on_motion_notify_event(ev);
  }


  bool NoteMenuItem::on_leave_notify_event(GdkEventCrossing *ev)
  {
    if (!m_pinned && m_pin_img) {
      m_pin_img->set(get_pinup_icon());
    }
    return Gtk::ImageMenuItem::on_leave_notify_event(ev);
  }

  std::string NoteMenuItem::get_display_name(const Note::Ptr & note)
  {
    std::string display_name = note->get_title();
    int max_length = 100;
    
    if (note->is_new()) {
      std::string new_string = _(" (new)");
      max_length -= new_string.size();
      display_name = ellipsify (display_name, max_length)  + new_string;
    } 
    else {
      display_name = ellipsify (display_name, max_length);
    }

    return display_name;
  }

  std::string NoteMenuItem::ellipsify (const std::string & str, size_t max)
  {
    if(str.size() > max) {
      std::string new_str = str;
      new_str.resize(max);
      return new_str + "...";
    }
    return str;
  }



  Glib::RefPtr<Gdk::Pixbuf> NoteMenuItem::get_pinup_icon()
  {
    return IconManager::obj().get_icon(IconManager::PIN_UP, 16);
  }

  Glib::RefPtr<Gdk::Pixbuf> NoteMenuItem::get_pindown_icon()
  {
    return IconManager::obj().get_icon(IconManager::PIN_DOWN, 16);
  }

#ifdef HAVE_X11_SUPPORT
  Tray::Tray(NoteManager & manager, IGnoteTray & trayicon, IKeybinder & keybinder)
    : m_manager(manager)
    , m_trayicon(trayicon)
    , m_menu_added(false)
    , m_keybinder(keybinder)
  {
    m_tray_menu = make_tray_notes_menu();
  }
#else
  Tray::Tray(NoteManager & manager, IGnoteTray & trayicon)
    : m_manager(manager)
    , m_trayicon(trayicon)
    , m_menu_added(false)
  {
    m_tray_menu = make_tray_notes_menu();
  }
#endif

  Gtk::Menu * Tray::make_tray_notes_menu()
  {
    Gtk::Menu *menu = manage(new Gtk::Menu);

    m_new_note_item = manage(new Gtk::ImageMenuItem);
    m_new_note_item->set_image(*manage(new Gtk::Image(Gtk::Stock::NEW, Gtk::ICON_SIZE_MENU)));
    m_new_note_item->set_label(_("Create _New Note"));
    m_new_note_item->set_use_underline(true);
    m_new_note_item->set_tooltip_text(_("Create a new note"));
    m_new_note_item->show();
    m_new_note_item->signal_activate()
      .connect(sigc::mem_fun(*this, &Tray::on_new_note_activate));
    menu->insert(*m_new_note_item, -1);
    m_search_notes_item = manage(new Gtk::ImageMenuItem);
    m_search_notes_item->set_image(*manage(new Gtk::Image(Gtk::Stock::FIND, Gtk::ICON_SIZE_MENU)));
    m_search_notes_item->set_label(_("_Search All Notes"));
    m_search_notes_item->set_use_underline(true);
    m_search_notes_item->set_tooltip_text(_("Open the Search All Notes window"));
    m_search_notes_item->show();
    m_search_notes_item->signal_activate()
      .connect(sigc::mem_fun(*this, &Tray::on_search_notes_activate));
    menu->insert(*m_search_notes_item, -1);

#ifdef HAVE_X11_SUPPORT
    bool enable_keybindings = Preferences::obj().get_schema_settings(
        Preferences::SCHEMA_GNOTE)->get_boolean(Preferences::ENABLE_KEYBINDINGS);
    if (enable_keybindings) {
      KeybindingToAccel::add_accelerator(m_keybinder, *m_new_note_item, Preferences::KEYBINDING_CREATE_NEW_NOTE);
      KeybindingToAccel::add_accelerator(m_keybinder, *m_search_notes_item, Preferences::KEYBINDING_OPEN_RECENT_CHANGES);
    }
#endif

    return menu;
  }

  void Tray::update_tray_menu(Gtk::Widget * parent)
  {
    if (!m_menu_added) {
      if (parent)
// Can't call the Gtkmm version as it is protected.
//        m_tray_menu->attach_to_widget(*parent);
        gtk_menu_attach_to_widget(m_tray_menu->gobj(), parent->gobj(), NULL);
        m_menu_added = true;
      }

      add_recently_changed_notes();

      m_tray_menu->show_all();
  }

  void Tray::add_recently_changed_notes()
  {
    int min_size = Preferences::obj().get_schema_settings(
        Preferences::SCHEMA_GNOTE)->get_int(Preferences::MENU_NOTE_COUNT);
    int max_size = 18;
    int list_size = 0;
    NoteMenuItem *item;

    std::vector<Gtk::Widget*> old_items = m_tray_menu->get_children();
    for(std::vector<Gtk::Widget*>::iterator iter = old_items.begin();
        iter != old_items.end(); ++iter) {
      m_tray_menu->remove(**iter);
    }

    sharp::DateTime days_ago(sharp::DateTime::now());
    days_ago.add_days(-3);

    // Prevent template notes from appearing in the menu
    Tag::Ptr template_tag = ITagManager::obj()
      .get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);

    // List the most recently changed notes, any currently
    // opened notes, and any pinned notes...
    const Note::List & notes(m_manager.get_notes());
    for(Note::List::const_iterator iter = notes.begin();
        iter != notes.end(); ++iter) {

      const Note::Ptr & note(*iter);
      
      if (note->is_special()) {
        DBG_OUT("skipping special note '%s'", note->get_title().c_str());
        continue;
      }
      
      // Skip template notes
      if (note->contains_tag (template_tag)) {
        DBG_OUT("skipping template '%s'", note->get_title().c_str());
        continue;
      }

      bool show = false;

      // Test for note.IsPinned first so that all of the pinned notes
      // are guaranteed to be included regardless of the size of the
      // list.
      if (note->is_pinned()) {
          show = true;
      } 
      else if ((note->is_opened() && note->get_window()->host()) ||
               (note->change_date().is_valid() && (note->change_date() > days_ago)) ||
               (list_size < min_size)) {
        if (list_size <= max_size)
          show = true;
      }

      if (show) {
        item = Gtk::manage(new NoteMenuItem (note, true));
        m_tray_menu->insert(*item, -1);
        list_size++;
      }
    }

    Note::Ptr start = m_manager.find_by_uri(m_manager.start_note_uri());
    if (start) {
      item = Gtk::manage(new NoteMenuItem(start, false));
      m_tray_menu->insert(*item, -1);
      list_size++;

#ifdef HAVE_X11_SUPPORT
      bool enable_keybindings = Preferences::obj().get_schema_settings(
          Preferences::SCHEMA_GNOTE)->get_boolean(Preferences::ENABLE_KEYBINDINGS);
      if (enable_keybindings) {
        KeybindingToAccel::add_accelerator(m_keybinder, *item, Preferences::KEYBINDING_OPEN_START_HERE);
      }
#endif
    }

    bool menuOpensUpward = m_trayicon.menu_opens_upward();
    ActionManager & am(static_cast<ActionManager &>(IActionManager::obj()));
    int insertion_point = 0; // If menu opens downward
    if(menuOpensUpward) {
      insertion_point = -1;
    }

    Gtk::SeparatorMenuItem *separator = manage(new Gtk::SeparatorMenuItem());
    m_tray_menu->insert(*separator, insertion_point);
    m_tray_menu->insert(*m_search_notes_item, insertion_point);
    std::vector<Gtk::MenuItem*> items = am.get_tray_menu_items();
    for(std::vector<Gtk::MenuItem*>::iterator iter = items.begin(); iter != items.end(); ++iter) {
      m_tray_menu->insert(**iter, insertion_point);
    }
    m_tray_menu->insert(*m_new_note_item, insertion_point);
  }

  void Tray::on_new_note_activate()
  {
    MainWindow &win = IGnote::obj().new_main_window();
    win.new_note();
    win.present();
    win.close_on_escape(Preferences::obj().get_schema_settings(
      Preferences::SCHEMA_GNOTE)->get_boolean(Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE));
  }

  void Tray::on_search_notes_activate()
  {
    IGnote::obj().new_main_window().present();
  }


#ifdef HAVE_X11_SUPPORT
  TrayIcon::TrayIcon(IKeybinder & keybinder, NoteManager & manager)
    : Gtk::StatusIcon()
    , m_tray(new Tray(manager, *this, keybinder))
    , m_context_menu(NULL)
    , m_keybinder(new GnotePrefsKeybinder(keybinder, manager, *this))
#else
  TrayIcon::TrayIcon(NoteManager & manager)
    : Gtk::StatusIcon()
    , m_tray(new Tray(manager, *this))
    , m_context_menu(NULL)
#endif
  {
    gtk_status_icon_set_tooltip_text(gobj(), 
                                     tray_util_get_tooltip_text().c_str());

    IGnote::obj().signal_quit.connect(sigc::mem_fun(*this, &TrayIcon::on_exit));
    signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::on_activate));
    signal_popup_menu().connect(sigc::mem_fun(*this, &TrayIcon::on_popup_menu));
  }

  void TrayIcon::show_menu(bool select_first_item)
  {
    if(m_context_menu) {
      DBG_OUT("context menu found");
    }
    m_tray->update_tray_menu(NULL);
    if(select_first_item) {
      m_tray->tray_menu()->select_first(false);
    }
    
    popup_menu_at_position(*m_tray->tray_menu(), 0, 
                           gtk_get_current_event_time());
  }

  TrayIcon::~TrayIcon()
  {
    delete m_context_menu;
#ifdef HAVE_X11_SUPPORT
    delete m_keybinder;
#endif
  }

  void TrayIcon::on_activate()
  {
    DBG_OUT("activated");
    show_menu(false);
  }

  void TrayIcon::on_popup_menu(guint button, guint32 activate_time)
  {
    DBG_OUT("popup");
    if(button == 3) {
      Gtk::Menu *menu = get_right_click_menu();
      popup_menu_at_position(*menu, button, activate_time);
    }
  }  

  Gtk::Menu * TrayIcon::get_right_click_menu()
  {
    DBG_OUT("get right click menu");
    if(m_context_menu) {
      DBG_OUT("menu already exists");
      return m_context_menu;
    }
    m_context_menu = new Gtk::Menu();
    DBG_OUT("creating menu");

    Glib::RefPtr<Gtk::AccelGroup> accel_group = Gtk::AccelGroup::create();
    m_context_menu->set_accel_group(accel_group);

    Gtk::ImageMenuItem * item;

    item = manage(new Gtk::ImageMenuItem(_("_Preferences"), true));
    item->set_image(*manage(new Gtk::Image(Gtk::Stock::PREFERENCES, Gtk::ICON_SIZE_MENU)));
    item->signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::show_preferences));
    m_context_menu->append(*item);

    item = manage(new Gtk::ImageMenuItem(_("_Help"), true));
    item->set_image(*manage(new Gtk::Image(Gtk::Stock::HELP, Gtk::ICON_SIZE_MENU)));
    item->signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::show_help_contents));
    m_context_menu->append(*item);

    item = manage(new Gtk::ImageMenuItem(_("_About Gnote"), true));
    item->set_image(*manage(new Gtk::Image(Gtk::Stock::ABOUT, Gtk::ICON_SIZE_MENU)));
    item->signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::show_about));
    m_context_menu->append(*item);

    m_context_menu->append(*manage(new Gtk::SeparatorMenuItem()));

    item = manage(new Gtk::ImageMenuItem(_("_Quit"), true));
    item->set_image(*manage(new Gtk::Image(Gtk::Stock::QUIT, Gtk::ICON_SIZE_MENU)));
    item->signal_activate().connect(sigc::mem_fun(*this, &TrayIcon::quit));
    m_context_menu->append(*item);

    m_context_menu->show_all();

    return m_context_menu;
  }

  void TrayIcon::show_preferences()
  {
    ActionManager::obj()["ShowPreferencesAction"]->activate();
  }

  void TrayIcon::show_help_contents()
  {
    ActionManager::obj()["ShowHelpAction"]->activate();
  }

  void TrayIcon::show_about()
  {
    ActionManager::obj()["ShowAboutAction"]->activate();
  }

  void TrayIcon::quit()
  {
    ActionManager::obj()["QuitGNoteAction"]->activate();
  }

  void TrayIcon::on_exit()
  {
    set_visible(false);
  }

  bool TrayIcon::menu_opens_upward()
  {
      bool open_upwards = false;
      int val = 0;
//      Glib::RefPtr<Gdk::Screen> screen;
      GdkScreen *cscreen;

      Gdk::Rectangle area;
      GtkOrientation orientation;
//      get_geometry(screen, area, orientation);
      gtk_status_icon_get_geometry(gobj(), &cscreen, area.gobj(), &orientation);
      val = area.get_y();

      Gtk::Requisition menu_req, unused;
      m_tray->tray_menu()->get_preferred_size(unused, menu_req);
      if (val + menu_req.height >= gdk_screen_get_height(cscreen))
        open_upwards = true;

      return open_upwards;
  }

  bool TrayIcon::on_size_changed(int size)
  {
    if(size < 32) {
      size = 24;
    }
    else if(size < 48) {
      size = 32;
    }
    else size = 48;
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = IconManager::obj().get_icon(IconManager::GNOTE, size);
    set(pixbuf);
    return Gtk::StatusIcon::on_size_changed(size);
  }

#ifdef HAVE_X11_SUPPORT
  //
  // This is a helper to take the XKeybinding string from GConf, and
  // convert it to a widget accelerator label, so note menu items can
  // display their global X keybinding.
  //
  // FIXME: It would be totally sweet to allow setting the accelerator
  // visually through the menuitem, and have the new value be stored in
  // GConf.
  //
  Glib::RefPtr<Gtk::AccelGroup> KeybindingToAccel::s_accel_group;

  
  Glib::RefPtr<Gtk::AccelGroup>
  KeybindingToAccel::get_accel_group()
  {
    if(!s_accel_group) {
      s_accel_group = Gtk::AccelGroup::create();
    }
    return s_accel_group;
  }

  std::string KeybindingToAccel::get_shortcut (const std::string & key)
  {
    try {
      std::string binding = Preferences::obj()
        .get_schema_settings(Preferences::SCHEMA_KEYBINDINGS)->get_string(key);
      if (binding.empty() ||
          binding == "disabled") {
        return "";
      }
      
      binding = sharp::string_replace_all(binding, "<", "");
      binding = sharp::string_replace_all(binding, ">", "-");        
      
      return binding;
    } 
    catch(...)
    {
    }
    return "";
  }

  void KeybindingToAccel::add_accelerator(IKeybinder & keybinder, Gtk::MenuItem & item, const std::string & key)
  {
    guint keyval;
    Gdk::ModifierType mods;

    if(keybinder.get_accel_keys(key, keyval, mods)) {
      item.add_accelerator ("activate",
                             get_accel_group(),
                             keyval,
                             mods,
                             Gtk::ACCEL_VISIBLE);
    }
  }
#endif
  
}
