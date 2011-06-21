/*
 * gnote
 *
 * Copyright (C) 2011 Aurimas Cernius
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


#include "config.h"

#include <boost/format.hpp>

#include <gdkmm/dragcontext.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/selectiondata.h>

#include <panel-applet.h>
#include <glibmm/i18n.h>

#include "sharp/string.hpp"
#include "sharp/uri.hpp"
#include "gnote.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "prefskeybinder.hpp"
#include "tray.hpp"
#include "undo.hpp"
#include "utils.hpp"

#include "applet.hpp"

namespace gnote {
namespace panel {


#define IID "GnoteApplet"
#define FACTORY_IID "GnoteAppletFactory"

enum PanelOrientation
{
  HORIZONTAL,
  VERTICAL
};

class GnotePanelAppletEventBox
  : public Gtk::EventBox
  , public gnote::IGnoteTray
{
public:
  GnotePanelAppletEventBox(NoteManager & manager);
  virtual ~GnotePanelAppletEventBox();

  const Tray::Ptr & get_tray() const
    {
      return m_tray;
    }
  Gtk::Image & get_image()
    {
      return m_image;
    }

  virtual void show_menu(bool select_first_item);
  virtual bool menu_opens_upward();

protected:
  virtual void on_size_allocate(Gtk::Allocation& allocation);

private:
  bool button_press(GdkEventButton *);
  void prepend_timestamped_text(const Note::Ptr & note,
                                const sharp::DateTime & timestamp,
                                const std::string & text);
  bool paste_primary_clipboard();
  void setup_drag_and_drop();
  void on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>&,int,int,
                             const Gtk::SelectionData&,guint,guint);
  void init_pixbuf();
  PanelOrientation get_panel_orientation();

  NoteManager & m_manager;
  Tray::Ptr     m_tray;
  int           m_panel_size;
  Gtk::Image    m_image;
};


GnotePanelAppletEventBox::GnotePanelAppletEventBox(NoteManager & manager)
  : m_manager(manager)
  , m_tray(new Tray(manager, *this))
  , m_panel_size(16)
  , m_image(utils::get_icon("gnote", m_panel_size))
{
  property_can_focus() = true;
  signal_button_press_event().connect(sigc::mem_fun(
                                        *this, &GnotePanelAppletEventBox::button_press));
  add(m_image);
  show_all();
  set_tooltip_text(tray_util_get_tooltip_text());
}


GnotePanelAppletEventBox::~GnotePanelAppletEventBox()
{
}


bool GnotePanelAppletEventBox::button_press(GdkEventButton *ev)
{
  Gtk::Widget * parent = get_parent();
  switch (ev->button)
  {
  case 1:
    m_tray->update_tray_menu(parent);

    utils::popup_menu(*m_tray->tray_menu(), ev);

    return true;
    break;
  case 2:
    if (Preferences::obj().get_schema_settings(
            Preferences::SCHEMA_GNOTE)->get_boolean(Preferences::ENABLE_ICON_PASTE)) {
      // Give some visual feedback
      drag_highlight();
      bool retval = paste_primary_clipboard ();
      drag_unhighlight();
      return retval;
    }
    break;
  }
  return false;
}


void GnotePanelAppletEventBox::prepend_timestamped_text(const Note::Ptr & note,
                                                        const sharp::DateTime & timestamp,
                                                        const std::string & text)
{
  NoteBuffer::Ptr buffer = note->get_buffer();
  std::string insert_text =
    str(boost::format("\n%1%\n%2%\n") % timestamp.to_string("%c") % text);

  buffer->undoer().freeze_undo();

  // Insert the date and list of links...
  Gtk::TextIter cursor = buffer->begin();
  cursor.forward_lines (1); // skip title

  cursor = buffer->insert (cursor, insert_text);

  // Make the date string a small font...
  cursor = buffer->begin();
  cursor.forward_lines (2); // skip title & leading newline

  Gtk::TextIter end = cursor;
  end.forward_to_line_end (); // end of date

  buffer->apply_tag_by_name ("datetime", cursor, end);

  // Select the text we've inserted (avoid trailing newline)...
  end = cursor;
  end.forward_chars (insert_text.length() - 1);

  buffer->move_mark(buffer->get_selection_bound(), cursor);
  buffer->move_mark(buffer->get_insert(), end);

  buffer->undoer().thaw_undo();
}



bool GnotePanelAppletEventBox::paste_primary_clipboard()
{
  Glib::RefPtr<Gtk::Clipboard> clip = get_clipboard ("PRIMARY");
  Glib::ustring text = clip->wait_for_text ();

  if (text.empty() || sharp::string_trim(text).empty()) {
    return false;
  }

  Note::Ptr link_note = m_manager.find_by_uri(m_manager.start_note_uri());
  if (!link_note) {
    return false;
  }

  link_note->get_window()->present();
  prepend_timestamped_text (link_note, sharp::DateTime::now(), text);

  return true;
}

void GnotePanelAppletEventBox::show_menu(bool select_first_item)
{
  m_tray->update_tray_menu(this);
  if(select_first_item) {
    m_tray->tray_menu()->select_first(false);
  }
  utils::popup_menu(*m_tray->tray_menu(), NULL);
}


// Support dropping text/uri-lists and _NETSCAPE_URLs currently.
void GnotePanelAppletEventBox::setup_drag_and_drop()
{
  std::vector<Gtk::TargetEntry> targets;

  targets.push_back(Gtk::TargetEntry ("text/uri-list", (Gtk::TargetFlags)0, 0));
  targets.push_back(Gtk::TargetEntry ("_NETSCAPE_URL", (Gtk::TargetFlags)0, 0));

  drag_dest_set(targets, Gtk::DEST_DEFAULT_ALL, Gdk::ACTION_COPY);

  signal_drag_data_received().connect(
    sigc::mem_fun(*this, &GnotePanelAppletEventBox::on_drag_data_received));
}

void GnotePanelAppletEventBox::on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>&,
                                                     int,int,
                                                     const Gtk::SelectionData & data,
                                                     guint,guint)
{
  utils::UriList uri_list(data);
  if (uri_list.empty()) {
    return;
  }

  std::string insert_text;
  bool more_than_one = false;

  for(utils::UriList::const_iterator iter = uri_list.begin();
      iter != uri_list.end(); ++iter) {

    const sharp::Uri & uri(*iter);

    if (more_than_one) {
      insert_text += "\n";
    }

    if (uri.is_file()) {
      insert_text += uri.local_path();
    }
    else {
      insert_text += uri.to_string();
    }

    more_than_one = true;
  }

  Note::Ptr link_note = m_manager.find_by_uri (m_manager.start_note_uri());
  if (link_note) {
    link_note->get_window()->present();
    prepend_timestamped_text(link_note, sharp::DateTime::now(),
                              insert_text);
  }
}


void GnotePanelAppletEventBox::init_pixbuf()
{
  // For some reason, the first time we ask for the allocation,
  // it's a 1x1 pixel.  Prevent against this by returning a
  // reasonable default.  Setting the icon causes OnSizeAllocated
  // to be called again anyhow.
  int icon_size = m_panel_size;
  if (icon_size < 16) {
    icon_size = 16;
  }

  // Control specifically which icon is used at the smaller sizes
  // so that no scaling occurs.  In the case of the panel applet,
  // add a couple extra pixels of padding so it matches the behavior
  // of the notification area tray icon.  See bug #403500 for more
  // info.
  if (Gnote::obj().is_panel_applet())
    icon_size = icon_size - 2; // padding
  if (icon_size <= 21)
    icon_size = 16;
  else if (icon_size <= 31)
    icon_size = 22;
  else if (icon_size <= 47)
    icon_size = 32;

  Glib::RefPtr<Gdk::Pixbuf> new_icon = utils::get_icon("gnote", icon_size);
  m_image.property_pixbuf() = new_icon;
}


PanelOrientation GnotePanelAppletEventBox::get_panel_orientation()
{
  if (!get_parent_window()) {
    return HORIZONTAL;
  }

  Glib::RefPtr<Gdk::Window> top_level_window = get_parent_window()->get_toplevel();

  Gdk::Rectangle rect;
  top_level_window->get_frame_extents(rect);
  if (rect.get_width() < rect.get_height()) {
    return HORIZONTAL;
  }

  return VERTICAL;
}



void GnotePanelAppletEventBox::on_size_allocate(Gtk::Allocation& allocation)
{
  Gtk::EventBox::on_size_allocate(allocation);

  // Determine the orientation
  if (get_panel_orientation () == HORIZONTAL) {
    if (m_panel_size == get_allocation().get_height()) {
      return;
    }

    m_panel_size = get_allocation().get_height();
  }
  else {
    if (m_panel_size == get_allocation().get_width()) {
      return;
    }

    m_panel_size = get_allocation().get_width();
  }

  init_pixbuf ();
}


bool GnotePanelAppletEventBox::menu_opens_upward()
{
  bool open_upwards = false;
  int val = 0;
  Glib::RefPtr<Gdk::Screen> screen;

  int x, y;
  get_window()->get_origin(x, y);
  val = y;
  screen = get_screen();

  Gtk::Requisition req, unused;
  m_tray->tray_menu()->get_preferred_size(unused, req);
  if ((val + req.height) >= screen->get_height()) {
    open_upwards = true;
  }

  return open_upwards;
}


static void show_preferences_action(GtkAction*, gpointer)
{
  ActionManager::obj()["ShowPreferencesAction"]->activate();
}


static void show_help_action(GtkAction*, gpointer data)
{
  // Don't use the ActionManager in this case because
  // the handler won't know about the Screen.
  utils::show_help("gnote", "", gtk_widget_get_screen(GTK_WIDGET(data)), NULL);
}

static void show_about_action(GtkAction*, gpointer)
{
  ActionManager::obj()["ShowAboutAction"]->activate();
}


static const GtkActionEntry applet_menu_actions[] = {
  { "Props", GTK_STOCK_PREFERENCES, N_("_Preferences"), NULL, NULL, G_CALLBACK(show_preferences_action) },
  { "Help", GTK_STOCK_HELP, N_("_Help"), NULL, NULL, G_CALLBACK(show_help_action) },
  { "About", GTK_STOCK_ABOUT, N_("_About"), NULL, NULL, G_CALLBACK(show_about_action) }
};



static void on_applet_change_size(PanelApplet*, gint size, gpointer data)
{
  static_cast<GnotePanelAppletEventBox*>(data)->set_size_request(size, size);
}


gboolean gnote_applet_fill(PanelApplet *applet, const gchar *iid, gpointer data)
{
  if(strcmp(iid, IID) != 0)
    return FALSE;

  GnotePanelAppletEventBox *applet_event_box = static_cast<GnotePanelAppletEventBox*>(data);
  gtk_container_add(GTK_CONTAINER(applet), GTK_WIDGET(applet_event_box->gobj()));
  Gnote::obj().set_tray(applet_event_box->get_tray());
  g_signal_connect(G_OBJECT(applet), "change-size",
                   G_CALLBACK(on_applet_change_size), applet_event_box);
  Gdk::RGBA color;
  color.set_alpha(1.0);
  applet_event_box->override_background_color(color);

  GtkActionGroup *action_group = gtk_action_group_new("Gnote Applet Actions");
  gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions(action_group, applet_menu_actions,
                               G_N_ELEMENTS(applet_menu_actions), applet);
  std::string ui_path = Glib::build_filename(DATADIR"/gnote", "GNOME_GnoteApplet.xml");
  panel_applet_setup_menu_from_file(applet, ui_path.c_str(), action_group);
  g_object_unref(action_group);

  gtk_widget_show_all(GTK_WIDGET(applet));
  // Set initial icon size according current panel size
  g_signal_emit_by_name(applet, "change-size", panel_applet_get_size(applet));
  return TRUE;
}


int register_applet()
{
  NoteManager &manager = Gnote::obj().default_note_manager();
  GnotePanelAppletEventBox applet_event_box(manager);
  GnotePrefsKeybinder key_binder(manager, applet_event_box);
  int returncode = panel_applet_factory_main(FACTORY_IID, PANEL_TYPE_APPLET,
                                             gnote_applet_fill, &applet_event_box);
  return returncode;
}

}
}

