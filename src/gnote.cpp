/*
 * gnote
 *
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

#include <stdlib.h>

#include <glibmm/thread.h>
#include <glibmm/i18n.h>
#include <glibmm/optionentry.h>
#include <gtkmm/main.h>
#include <gtkmm/aboutdialog.h>

#if HAVE_PANELAPPLETMM
#include <libpanelappletmm/init.h>
#endif

#include "gnote.hpp"
#include "actionmanager.hpp"
#include "addinmanager.hpp"
#include "applicationaddin.hpp"
#include "debug.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferencesdialog.hpp"
#include "recentchanges.hpp"
#include "utils.hpp"
#include "xkeybinder.hpp"
#include "sharp/string.hpp"

#if HAVE_PANELAPPLETMM
#include "applet.hpp"
#endif

namespace gnote {

  bool Gnote::s_tray_icon_showing = false;

  Gnote::Gnote()
    : m_manager(NULL)
    , m_keybinder(NULL)
    , m_is_panel_applet(false)
    , m_prefsdlg(NULL)
  {
  }

  Gnote::~Gnote()
  {
    delete m_prefsdlg;
    delete m_manager;
    delete m_keybinder;
  }


  int Gnote::main(int argc, char **argv)
  {
    GnoteCommandLine cmd_line;


    Glib::OptionContext context;
    context.set_ignore_unknown_options(true);
    context.set_main_group(cmd_line);
    try {
      context.parse(argc, argv);
    }
    catch(...)
    {
    }

    if(cmd_line.needs_execute()) {
      cmd_line.execute();
    }

    m_icon_theme = Gtk::IconTheme::get_default();
    m_icon_theme->append_search_path(DATADIR"/icons");
    m_icon_theme->append_search_path(DATADIR"/gnote/icons");

    std::string note_path = get_note_path(cmd_line.note_path());
    m_manager = new NoteManager(note_path);
    m_keybinder = new XKeybinder();

    // TODO
    // SyncManager::init()

    ActionManager & am(ActionManager::obj());
    am.load_interface();
//    register_remote_control(m_manager);
    setup_global_actions();
    
    std::list<ApplicationAddin*> addins;
    m_manager->get_addin_manager().get_application_addins(addins);
    for(std::list<ApplicationAddin*>::const_iterator iter = addins.begin();
        iter != addins.end(); ++iter) {
      (*iter)->initialize();
    }

    if(cmd_line.use_panel_applet()) {
      DBG_OUT("starting applet");
      s_tray_icon_showing = true;
      m_is_panel_applet = true;

      am["CloseWindowAction"]->set_visible(true);
      am["QuitGNoteAction"]->set_visible(false);
      
      // register panel applet factory
#if HAVE_PANELAPPLETMM
      Gnome::Panel::init("gnote", VERSION, argc, argv);

      panel::register_applet();
#endif
      return 0;

    }
    else {
      DBG_OUT("starting tray icon");
      //register session manager restart
      start_tray_icon();
    }
    return 0;
  }

  std::string Gnote::get_note_path(const std::string & override_path)
  {
    std::string note_path;
    if(override_path.empty()) {
      const char * s = getenv("GNOTE_PATH");
      note_path = s?s:"";
    }
    else {
      note_path = override_path;
    }
    if(note_path.empty()) {
      note_path = Gnote::conf_dir();
    }
    std::string home_dir;
    const char *s = getenv("HOME");
    if(s) {
      home_dir = s;
      note_path = sharp::string_replace_first(note_path, "~", home_dir);
    }

    return note_path;
  }

  void Gnote::start_tray_icon()
  {
    // Create the tray icon and run the main loop
    m_tray_icon = Glib::RefPtr<TrayIcon>(new TrayIcon(default_note_manager()));
    m_tray = m_tray_icon->tray();

    // Give the TrayIcon 2 seconds to appear.  If it
    // doesn't by then, open the SearchAllNotes window.
    s_tray_icon_showing = m_tray_icon->is_embedded() 
      && m_tray_icon->get_visible();
      
    if (!s_tray_icon_showing) {
      Glib::RefPtr<Glib::TimeoutSource> timeout 
        = Glib::TimeoutSource::create(2000);
      timeout->connect(sigc::mem_fun(*this, &Gnote::check_tray_icon_showing));
      timeout->attach();
    }
    
    Gtk::Main::run();
  }


  bool Gnote::check_tray_icon_showing()
  {
    s_tray_icon_showing = m_tray_icon->is_embedded() 
      && m_tray_icon->get_visible();
    if(!s_tray_icon_showing) {
      ActionManager & am(ActionManager::obj());
      am["ShowSearchAllNotesAction"]->activate();
    }
    return false;
  }


  void Gnote::setup_global_actions()
  {
    ActionManager & am(ActionManager::obj());
    am["NewNoteAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::on_new_note_action));
    am["QuitGNoteAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::on_quit_gnote_action));
    am["ShowPreferencesAction"]->signal_activate() 
      .connect(sigc::mem_fun(*this, &Gnote::on_show_preferences_action));
    am["ShowHelpAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::on_show_help_action));
    am["ShowAboutAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::on_show_about_action));
    am["TrayNewNoteAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::on_new_note_action));
    am["ShowSearchAllNotesAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::open_search_all));
    am["NoteSynchronizationAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::open_note_sync_window));
  }

  void Gnote::on_new_note_action()
  {
    try {
      Note::Ptr new_note = default_note_manager().create();
      new_note->get_window()->show();
    }
    catch(const std::exception & e) 
    {
      utils::HIGMessageDialog dialog (
        NULL,  (GtkDialogFlags)0,
        Gtk::MESSAGE_ERROR,
        Gtk::BUTTONS_OK,
        _("Cannot create new note"),
        e.what());
      dialog.run ();
    }    
  }

  void Gnote::on_quit_gnote_action()
  {
    if(m_is_panel_applet) {
      return;
    }
    Gtk::Main::quit();
  }

  void Gnote::on_preferences_response(int /*res*/)
  {
    delete m_prefsdlg;
    m_prefsdlg = NULL;
  }


  void Gnote::on_show_preferences_action()
  {
    if(!m_prefsdlg) {
      m_prefsdlg = new PreferencesDialog(m_manager->get_addin_manager());
      m_prefsdlg->signal_response().connect(
        sigc::mem_fun(*this, &Gnote::on_preferences_response));
    }
    m_prefsdlg->present();
  }

  void Gnote::on_show_help_action()
  {
    GdkScreen *cscreen;
    Gdk::Rectangle area;
    GtkOrientation orientation;
    gtk_status_icon_get_geometry(m_tray_icon->gobj(), &cscreen, area.gobj(), &orientation);
    utils::show_help("gnote", "", cscreen, NULL);
  }

  void Gnote::on_show_about_action()
  {
    std::list<Glib::ustring> authors;
    authors.push_back("Hubert Figuiere <hub@figuiere.net>");
    authors.push_back(_("and Tomboy original authors."));
    
    std::list<Glib::ustring> documenters;
    documenters.push_back("Alex Graveley <alex@beatniksoftware.com>");
    documenters.push_back("Boyd Timothy <btimothy@gmail.com>");
    documenters.push_back("Brent Smith <gnome@nextreality.net>");
    documenters.push_back("Paul Cutler <pcutler@foresightlinux.org>");
    documenters.push_back("Sandy Armstrong <sanfordarmstrong@gmail.com>");

    std::string translators(_("translator-credits"));
    if (translators == "translator-credits")
      translators = "";

    Gtk::AboutDialog about;
    about.set_name("Gnote");
    about.set_version(VERSION);
    about.set_logo(utils::get_icon("gnote", 48));
    about.set_copyright("Copyright (c) 2009 Hubert Figuiere");
    about.set_comments(_("A simple and easy to use desktop "
                         "note-taking application."));
// I don't think we need a hook.
//      Gtk.AboutDialog.SetUrlHook (delegate (Gtk.AboutDialog dialog, string link) {
//        try {
//          Services.NativeApplication.OpenUrl (link);
//        } catch (Exception e) {
//          GuiUtils.ShowOpeningLocationError (dialog, link, e.Message);
//        }
//      }); 
    about.set_website("http://live.gnome.org/Gnote");
    about.set_website_label(_("Homepage"));
    about.set_authors(authors);
    about.set_documenters(documenters);
    about.set_translator_credits(translators);
//      about.set_icon_name("gnote");
    about.run();
  }

  void Gnote::open_search_all()
  {
    NoteRecentChanges::get_instance(default_note_manager())->present();
  }

  void Gnote::open_note_sync_window()
  {
#if 0
    // TODO
    if (sync_dlg == null) {
      sync_dlg = new SyncDialog ();
      sync_dlg.Response += OnSyncDialogResponse;
    }

    sync_dlg.Present 
#endif
  }


  std::string Gnote::conf_dir()
  {
    std::string dir;
    const char * home = getenv("HOME");
    if(!home) {
      home = ".";
    }
    dir = std::string(home) + "/.gnote";
    return dir;
  }


  GnoteCommandLine::GnoteCommandLine()
    : Glib::OptionGroup("Gnote", _("A note taking application"))
    , m_new_note(false)
    , m_open_search(false)
    , m_open_start_here(false)
    , m_use_panel(false)
  {
    Glib::OptionEntry entry;
    entry.set_long_name("panel-applet");
    entry.set_description(_("Run Gnote as a GNOME panel applet"));
    add_entry(entry, m_use_panel);
  }

  int GnoteCommandLine::execute()
  {
    // TODO
    return 0;
  }

  bool GnoteCommandLine::needs_execute() const
  {
    return m_new_note ||
      !m_open_note_name.empty() ||
      !m_open_note_uri.empty() ||
      m_open_search ||
      m_open_start_here ||
      !m_open_external_note_path.empty();
  }


}
