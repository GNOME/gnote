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



#include "config.h"

#include <stdlib.h>

#include <iostream>

#include <boost/bind.hpp>
#include <boost/format.hpp>

#include <glibmm/thread.h>
#include <glibmm/i18n.h>
#include <glibmm/optionentry.h>
#include <gtkmm/main.h>
#include <gtkmm/aboutdialog.h>

#include "gnote.hpp"
#include "actionmanager.hpp"
#include "addinmanager.hpp"
#include "applicationaddin.hpp"
#include "debug.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferencesdialog.hpp"
#include "remotecontrolproxy.hpp"
#include "utils.hpp"
#include "xkeybinder.hpp"
#include "dbus/remotecontrol.hpp"
#include "dbus/remotecontrolclient.hpp"
#include "sharp/streamreader.hpp"
#include "sharp/files.hpp"
#include "synchronization/syncmanager.hpp"


namespace gnote {

  Gnote *Gnote::s_obj = NULL;

  Gnote::Gnote()
    : Gtk::Application("org.gnome.Gnote", Gio::APPLICATION_HANDLES_COMMAND_LINE)
    , m_manager(NULL)
    , m_keybinder(NULL)
    , m_is_background(false)
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
    bool handle = false;
    for(int i = 0; i < argc; ++i) {
      if(!strcmp(argv[i], "--help") || !strcmp(argv[i], "--version")) {
        handle = true;
        break;
      }
    }

    if(handle) {
      cmd_line.parse(argc, argv);
      cmd_line.immediate_execute();
      return 0;
    }

    int retval = run(argc, argv);
    signal_quit();
    return retval;
  }


  void Gnote::on_startup()
  {
    Gtk::Application::on_startup();
    m_icon_theme = Gtk::IconTheme::get_default();
    m_icon_theme->append_search_path(DATADIR"/icons");
    m_icon_theme->append_search_path(DATADIR"/gnote/icons");
  }


  int Gnote::on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine> & command_line)
  {
    Gtk::Application::on_command_line(command_line);
    int argc = 0;
    char **argv = command_line->get_arguments(argc);
    cmd_line.parse(argc, argv);
    if(!m_manager) {
      common_init();
      Glib::RefPtr<Gio::Settings> settings = Preferences::obj()
        .get_schema_settings(Preferences::SCHEMA_GNOTE);
      settings->signal_changed()
        .connect(sigc::mem_fun(*this, &Gnote::on_setting_changed));
      register_object();
    }
    else if(cmd_line.needs_execute()) {
      cmd_line.execute();
    }
    else {
      new_main_window()->present();
    }

    return 0;
  }


  void Gnote::on_window_removed(Gtk::Window *window)
  {
    // Do not remove last window if background or status icon mode
    if(windowed() || get_windows().size() > 1) {
      Gtk::Application::on_window_removed(window);
    }
  }


  void Gnote::common_init()
  {
    std::string note_path = get_note_path(cmd_line.note_path());
    m_manager = new NoteManager(note_path, sigc::mem_fun(*this, &Gnote::start_note_created));
    m_keybinder = new XKeybinder();
    ActionManager::obj().load_interface();
    sync::SyncManager::init();
    setup_global_actions();
    m_manager->get_addin_manager().initialize_application_addins();
  }


  void Gnote::end_main(bool bus_acquired, bool name_acquired)
  {
    ActionManager & am(ActionManager::obj());
    if((m_is_background = cmd_line.background())) {
      am["QuitGNoteAction"]->set_visible(false);
    }
    if(cmd_line.needs_execute()) {
      cmd_line.execute();
    }

    if(bus_acquired) {
      if(name_acquired) {
        DBG_OUT("Gnote remote control active.");
      } 
      else {
        // If Gnote is already running, open the search window
        // so the user gets some sort of feedback when they
        // attempt to run Gnote again.
        Glib::RefPtr<RemoteControlClient> remote;
        try {
          remote = RemoteControlProxy::get_instance();
          DBG_ASSERT(remote, "remote is NULL, something is wrong");
          if(remote) {
            remote->DisplaySearch();
          }
        } 
        catch (...)
        {
        }

        ERR_OUT ("Gnote is already running.  Exiting...");
        ::exit(-1);
      }
    }

    make_app_actions();
    make_app_menu();
    Glib::RefPtr<Gio::Settings> settings = Preferences::obj()
      .get_schema_settings(Preferences::SCHEMA_GNOTE);
    if(settings->get_boolean(Preferences::USE_STATUS_ICON)) {
      DBG_OUT("starting tray icon");
      start_tray_icon();
    }
    else if(m_is_background) {
      // Create Search All Notes window as we need it present for application to run
      new_main_window();
    }
    else {
      am["ShowSearchAllNotesAction"]->activate();
    }
  }


  void Gnote::start_note_created(const Note::Ptr & start_note)
  {
    DBG_OUT("we will show the start note");
    start_note->get_window()->show();
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
      note_path = Gnote::data_dir();
    }

    return note_path;
  }

  void Gnote::start_tray_icon()
  {
    // Create Search All Notes window as we need it present for application to run
    get_main_window();

    // Create the tray icon and run the main loop
    m_tray_icon = Glib::RefPtr<TrayIcon>(new TrayIcon(default_note_manager()));
    m_tray = m_tray_icon->tray();
  }


  void Gnote::register_remote_control(NoteManager & manager, RemoteControlProxy::slot_name_acquire_finish on_finish)
  {
    RemoteControlProxy::register_remote(manager, on_finish);
  }


  void Gnote::register_object()
  {
    RemoteControlProxy::register_object(Gio::DBus::Connection::get_sync(Gio::DBus::BUS_TYPE_SESSION),
                                        Gnote::obj().default_note_manager(),
                                        sigc::mem_fun(Gnote::obj(), &Gnote::end_main));
  }


  void Gnote::on_setting_changed(const Glib::ustring & key)
  {
    if(key != Preferences::USE_STATUS_ICON) {
      return;
    }

    bool use_status_icon = Preferences::obj()
      .get_schema_settings(Preferences::SCHEMA_GNOTE)->get_boolean(key);
    if(use_status_icon) {
      if(!m_tray_icon) {
        m_tray_icon = Glib::RefPtr<TrayIcon>(new TrayIcon(default_note_manager()));
      }
      m_tray_icon->set_visible(true);
    }
    else {
      if(m_tray_icon)
        m_tray_icon->set_visible(false);
      ActionManager::obj()["ShowSearchAllNotesAction"]->activate();
    }
  }


  void Gnote::setup_global_actions()
  {
    ActionManager & am(ActionManager::obj());
    am["QuitGNoteAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::quit));
    am["ShowPreferencesAction"]->signal_activate().connect(
      boost::bind(sigc::mem_fun(*this, &Gnote::on_show_preferences_action), Glib::VariantBase()));
    am["ShowHelpAction"]->signal_activate()
      .connect(boost::bind(sigc::mem_fun(*this, &Gnote::on_show_help_action), Glib::VariantBase()));
    am["ShowAboutAction"]->signal_activate()
      .connect(boost::bind(sigc::mem_fun(*this, &Gnote::on_show_about_action), Glib::VariantBase()));
    am["TrayNewNoteAction"]->signal_activate()
      .connect(boost::bind(sigc::mem_fun(*this, &Gnote::on_new_note_app_action), Glib::VariantBase()));
    am["ShowSearchAllNotesAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &Gnote::open_search_all));
  }

  void Gnote::on_quit_gnote_action(const Glib::VariantBase&)
  {
    quit();
  }

  void Gnote::on_preferences_response(int /*res*/)
  {
    delete m_prefsdlg;
    m_prefsdlg = NULL;
  }


  void Gnote::on_show_preferences_action(const Glib::VariantBase&)
  {
    if(!m_prefsdlg) {
      m_prefsdlg = new PreferencesDialog(m_manager->get_addin_manager());
      m_prefsdlg->signal_response().connect(
        sigc::mem_fun(*this, &Gnote::on_preferences_response));
    }
    m_prefsdlg->present();
  }

  void Gnote::on_show_help_action(const Glib::VariantBase&)
  {
    GdkScreen *cscreen = NULL;
    if(m_tray_icon) {
      Gdk::Rectangle area;
      GtkOrientation orientation;
      gtk_status_icon_get_geometry(m_tray_icon->gobj(), &cscreen, area.gobj(), &orientation);
    }
    utils::show_help("gnote", "", cscreen, NULL);
  }

  void Gnote::on_show_about_action(const Glib::VariantBase&)
  {
    std::vector<Glib::ustring> authors;
    authors.push_back("Aurimas Černius <aurisc4@gmail.com>");
    authors.push_back("Debarshi Ray <debarshir@src.gnome.org>");
    authors.push_back("Hubert Figuiere <hub@figuiere.net>");
    authors.push_back("Iain Nicol <iainn@src.gnome.org>");
    authors.push_back(_("and Tomboy original authors."));
    
    std::vector<Glib::ustring> documenters;
    documenters.push_back("Pierre-Yves Luyten <py@luyten.fr>");
    documenters.push_back("Aurimas Černius <aurisc4@gmail.com>");

    std::string translators(_("translator-credits"));
    if (translators == "translator-credits")
      translators = "";

    Gtk::AboutDialog about;
    about.set_name("Gnote");
    about.set_program_name("Gnote");
    about.set_version(VERSION);
    about.set_logo(utils::get_icon("gnote", 48));
    about.set_copyright(_("Copyright \xc2\xa9 2010-2012 Aurimas Cernius\n"
                          "Copyright \xc2\xa9 2009-2011 Debarshi Ray\n"
                          "Copyright \xc2\xa9 2009 Hubert Figuiere\n"
                          "Copyright \xc2\xa9 2004-2009 the Tomboy original authors."));
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
    NoteRecentChanges::Ptr recent_changes = get_main_window();
    if(recent_changes && recent_changes->get_visible()) {
      about.set_transient_for(*recent_changes);
      recent_changes->present();
    }
    about.run();
  }

  NoteRecentChanges::Ptr Gnote::new_main_window()
  {
    NoteRecentChanges::Ptr win = NoteRecentChanges::create(default_note_manager());
    std::list<NoteRecentChanges::Ptr>::iterator pos = m_main_windows.insert(m_main_windows.end(), win);
    win->signal_hide().connect(boost::bind(sigc::mem_fun(*this, &Gnote::on_main_window_closed), pos));
    add_window(*win);
    return win;
  }

  NoteRecentChanges::Ptr Gnote::get_main_window()
  {
    for(std::list<NoteRecentChanges::Ptr>::iterator iter = m_main_windows.begin();
        iter != m_main_windows.end(); ++iter) {
      return *iter;
    }

    NoteRecentChanges::Ptr win = new_main_window();
    return win;
  }

  void Gnote::on_main_window_closed(std::list<NoteRecentChanges::Ptr>::iterator pos)
  {
    remove_window(**pos);
    m_main_windows.erase(pos);

    // if background mode, we need to have a window, to prevent quit
    if(m_is_background && !m_main_windows.size()) {
      new_main_window();
    }
  }

  NoteRecentChanges::Ptr Gnote::get_window_for_note()
  {
    NoteRecentChanges::Ptr window;
    //Find first visible window or any window otherwise
    for(std::list<NoteRecentChanges::Ptr>::iterator iter = m_main_windows.begin();
        iter != m_main_windows.end(); ++iter) {
      if((*iter)->get_visible()) {
        window = *iter;
        break;
      }
      if(!window) {
        window = *iter;
      }
    }
    if(!window) {
      window = new_main_window();
    }

    return window;
  }

  void Gnote::open_search_all()
  {
    get_main_window()->present();
  }

  void Gnote::open_note_sync_window(const Glib::VariantBase&)
  {
    if(m_sync_dlg == 0) {
      m_sync_dlg = sync::SyncDialog::create();
      m_sync_dlg->signal_response().connect(sigc::mem_fun(*this, &Gnote::on_sync_dialog_response));
    }

    m_sync_dlg->present();
  }


  void Gnote::on_sync_dialog_response(int)
  {
    m_sync_dlg->hide();
    m_sync_dlg.reset();
  }


  void Gnote::make_app_actions()
  {
    ActionManager & am(ActionManager::obj());
    am.get_app_action("new-note")->signal_activate().connect(sigc::mem_fun(*this, &Gnote::on_new_note_app_action));
    am.get_app_action("new-window")->signal_activate().connect(sigc::mem_fun(*this, &Gnote::on_new_window_action));
    am.get_app_action("show-preferences")->signal_activate().connect(
      sigc::mem_fun(*this, &Gnote::on_show_preferences_action));
    am.get_app_action("sync-notes")->signal_activate().connect(sigc::mem_fun(*this, &Gnote::open_note_sync_window));
    am.get_app_action("help-contents")->signal_activate().connect(sigc::mem_fun(*this, &Gnote::on_show_help_action));
    am.get_app_action("about")->signal_activate().connect(sigc::mem_fun(*this, &Gnote::on_show_about_action));
    am.get_app_action("quit")->signal_activate().connect(sigc::mem_fun(*this, &Gnote::on_quit_gnote_action));

    add_app_actions(am.get_app_actions());
  }


  void Gnote::add_app_actions(const std::vector<Glib::RefPtr<Gio::SimpleAction> > & actions)
  {
    for(std::vector<Glib::RefPtr<Gio::SimpleAction> >::const_iterator iter = actions.begin();
        iter != actions.end(); ++iter) {
      add_action(*iter);
    }
  }


  void Gnote::on_new_window_action(const Glib::VariantBase&)
  {
    new_main_window()->present();
  }


  NoteRecentChanges *Gnote::get_active_window()
  {
    std::vector<Gtk::Window*> windows = Gtk::Window::list_toplevels();
    for(std::vector<Gtk::Window*>::iterator iter = windows.begin(); iter != windows.end(); ++iter) {
      if((*iter)->property_is_active()) {
        return dynamic_cast<NoteRecentChanges*>(*iter);
      }
    }

    return NULL;
  }


  void Gnote::on_new_note_app_action(const Glib::VariantBase&)
  {
    NoteRecentChanges *rc = get_active_window();
    if(rc) {
      rc->new_note();
    }
    else {
      get_main_window()->new_note();
    }
  }


  void Gnote::make_app_menu()
  {
    set_app_menu(ActionManager::obj().get_app_menu());
  }


  std::string Gnote::cache_dir()
  {
    return Glib::get_user_cache_dir() + "/gnote";
  }


  std::string Gnote::conf_dir()
  {
    return Glib::get_user_config_dir() + "/gnote";
  }


  std::string Gnote::data_dir()
  {
    return Glib::get_user_data_dir() + "/gnote";
  }


  std::string Gnote::old_note_dir()
  {
    std::string home_dir = Glib::get_home_dir();

    if (home_dir.empty())
      home_dir = Glib::get_current_dir();

    return home_dir + "/.gnote";
  }



  GnoteCommandLine::GnoteCommandLine()
    : m_context(g_option_context_new("Foobar"))
    , m_use_panel(false)
    , m_background(false)
    , m_note_path(NULL)
    , m_do_search(false)
    , m_show_version(false)
    , m_do_new_note(false)
    , m_open_note(NULL)
    , m_open_start_here(false)
    , m_highlight_search(NULL)
  {
    static const GOptionEntry entries[] =
      {
        { "background", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &m_background, _("Run Gnote in background."), NULL },
        { "note-path", 0, 0, G_OPTION_ARG_STRING, &m_note_path, _("Specify the path of the directory containing the notes."), _("path") },
        { "search", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, (void*)GnoteCommandLine::parse_func, _("Open the search all notes window with the search text."), _("text") },
        { "version", 0, 0, G_OPTION_ARG_NONE, &m_show_version, _("Print version information."), NULL },
        { "new-note", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, (void*)GnoteCommandLine::parse_func, _("Create and display a new note, with a optional title."), _("title") },
        { "open-note", 0, 0, G_OPTION_ARG_STRING, &m_open_note, _("Display the existing note matching title."), _("title/url") },
        { "start-here", 0, 0, G_OPTION_ARG_NONE, &m_open_start_here, _("Display the 'Start Here' note."), NULL },
        { "highlight-search", 0, 0, G_OPTION_ARG_STRING, &m_highlight_search, _("Search and highlight text in the opened note."), _("text") },
        { NULL, 0, 0, (GOptionArg)0, NULL, NULL, NULL }
      };

    GOptionGroup *group = g_option_group_new("Gnote", _("A note taking application"), _("Gnote options at launch"), this, NULL);
    g_option_group_add_entries(group, entries);
    g_option_context_set_main_group (m_context, group);

    // we need this for the panel support.
    g_option_context_set_ignore_unknown_options(m_context, TRUE);
  }

  
  GnoteCommandLine::~GnoteCommandLine()
  {
    g_option_context_free(m_context);
  }

  gboolean GnoteCommandLine::parse_func(const gchar *option_name,
                                        const gchar *value,
                                        gpointer data,
                                        GError ** /*error*/)
  {
    GnoteCommandLine * self = (GnoteCommandLine*)data;
  
    if(g_str_equal (option_name, "--search")) {
      self->m_do_search = true;
      if(value) {
        self->m_search = value;
      }
    }
    else if(g_str_equal (option_name, "--new-note")) {
      self->m_do_new_note = true;
      if(value) {
        self->m_new_note_name = value;
      }
    }
    return TRUE;
  }


  void GnoteCommandLine::parse(int &argc, gchar ** & argv)
  {
    GError *error = NULL;

    if(!g_option_context_parse (m_context, &argc, &argv, &error)) {
      g_print ("option parsing failed: %s\n", error->message);
      exit (1);
    }

    if(m_open_note && *m_open_note) {
      if (Glib::str_has_prefix(m_open_note, "note://gnote/")) {
        m_open_note_uri = m_open_note;
      }
      else if (sharp::file_exists(m_open_note)) {
        // This is potentially a note file
        m_open_external_note_path = m_open_note;
      } 
      else {
        m_open_note_name = m_open_note;
      }
    }
    else if(!m_open_note && argc > 1 && Glib::str_has_prefix(argv[argc - 1], "note://gnote/")) {
      m_open_note = argv[argc -1];
      m_open_note_uri = m_open_note;
    }
  }


  int GnoteCommandLine::execute()
  {
    DBG_OUT("running args");

    RemoteControl *remote_control = RemoteControlProxy::get_remote_control();
    if(remote_control) {
      execute(remote_control);
    }
    else {
      //gnote already running, execute via D-Bus and exit this instance
      Glib::RefPtr<RemoteControlClient> remote = RemoteControlProxy::get_instance();
      if(!remote) {
        ERR_OUT("Could not connect to remote instance.");
      }
      else {
        execute(remote);
      }
      Gnote::obj().quit();
    }
    return 0;
  }


  int GnoteCommandLine::immediate_execute()
  {
    if(m_show_version) {
      print_version();
      exit(0);
    }

    return 0;
  }


  template <typename T>
  void GnoteCommandLine::execute(T & remote)
  {
    if (m_do_new_note) {
      std::string new_uri;

      if (!m_new_note_name.empty()) {
        new_uri = remote->FindNote (m_new_note_name);

        if (new_uri.empty()) {
          new_uri = remote->CreateNamedNote(m_new_note_name);
        }
      } 
      else {
        new_uri = remote->CreateNote ();
      }

      if (!new_uri.empty()) {
        remote->DisplayNote (new_uri);
      }
    }

    if (m_open_start_here) {
      m_open_note_uri = remote->FindStartHereNote ();
    }
    if (!m_open_note_name.empty()) {
      m_open_note_uri = remote->FindNote (m_open_note_name);
    }
    if (!m_open_note_uri.empty()) {
      display_note(remote, m_open_note_uri);
    }

    if (!m_open_external_note_path.empty()) {
      std::string note_id = sharp::file_basename(m_open_external_note_path);
      if (!note_id.empty()) {
        // Attempt to load the note, assuming it might already
        // be part of our notes list.
        if (!display_note(remote, str(boost::format("note://gnote/%1%") % note_id))) {
          sharp::StreamReader sr;
          sr.init(m_open_external_note_path);
          if (sr.file()) {
            std::string noteTitle;
            std::string noteXml;
            sr.read_to_end (noteXml);

            // Make sure noteXml is parseable
            xmlDocPtr doc = xmlParseDoc((const xmlChar*)noteXml.c_str());
            if(doc) {
              xmlFreeDoc(doc);
            }
            else {
              noteXml = "";
            }

            if (!noteXml.empty()) {
              noteTitle = NoteArchiver::obj().get_title_from_note_xml (noteXml);
              if (!noteTitle.empty()) {
                // Check for conflicting titles
                std::string baseTitle = noteTitle;
                for (int i = 1; !remote->FindNote (noteTitle).empty(); i++) {
                  noteTitle = str(boost::format("%1% (%2%)") % baseTitle % i);
                }

                std::string note_uri = remote->CreateNamedNote (noteTitle);

                // Update title in the note XML
                noteXml = NoteArchiver::obj().get_renamed_note_xml (noteXml, baseTitle, noteTitle);

                if (!note_uri.empty()) {
                  // Load in the XML contents of the note file
                  if (remote->SetNoteCompleteXml (note_uri, noteXml)) {
                    display_note(remote, note_uri);
                  }
                }
              }
            }
          }
        }
      }
    }

    if (m_do_search) {
      if (!m_search.empty()) {
        remote->DisplaySearchWithText(m_search);
      }
      else {
        remote->DisplaySearch();
      }
    }
  }


  void GnoteCommandLine::print_version()
  {
    Glib::ustring version = str(boost::format(_("Version %1%"))
                                % VERSION);
    std::cerr << version << std::endl;
  }


  template <typename T>
  bool GnoteCommandLine::display_note(T & remote, std::string uri)
  {
    if (m_highlight_search) {
      return remote->DisplayNoteWithSearch(uri, m_highlight_search);
    }
    else {
      return remote->DisplayNote (uri);
    }
  }


  bool GnoteCommandLine::needs_execute() const
  {
    DBG_OUT("needs execute?");
    return m_do_new_note ||
      m_open_note ||
      m_do_search ||
      m_open_start_here ||
      m_highlight_search;
  }

  bool GnoteCommandLine::needs_immediate_execute() const
  {
    return m_show_version;
  }

}
