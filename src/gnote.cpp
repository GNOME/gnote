/*
 * gnote
 *
 * Copyright (C) 2010-2025 Aurimas Cernius
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

#include <adwaita.h>
#include <glibmm/i18n.h>
#include <glibmm/stringutils.h>
#include <glibmm/optionentry.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/builder.h>
#include <gtkmm/shortcutswindow.h>

#include "gnote.hpp"
#include "addinmanager.hpp"
#include "applicationaddin.hpp"
#include "debug.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferencesdialog.hpp"
#include "recentchanges.hpp"
#include "remotecontrolproxy.hpp"
#include "utils.hpp"
#include "tagmanager.hpp"
#include "dbus/remotecontrol.hpp"
#include "sharp/streamreader.hpp"
#include "sharp/files.hpp"
#include "notebooks/notebookmanager.hpp"


namespace gnote {

  Gnote::Gnote()
    : Gtk::Application("org.gnome.Gnote", Gio::Application::Flags::HANDLES_COMMAND_LINE)
    , m_manager(NULL)
    , m_sync_manager(NULL)
    , m_is_background(false)
    , m_is_shell_search(false)
    , m_cmd_line(*this)
  {
  }

  Gnote::~Gnote()
  {
    if(m_sync_manager) {
      delete m_sync_manager;
    }
    // why this crashes inside GDataTime sometimes when deleted?
    //delete m_manager;
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
      m_cmd_line.parse(argc, argv);
      m_cmd_line.immediate_execute();
      return 0;
    }

    int retval = run(argc, argv);
    signal_quit();
    return retval;
  }


  void Gnote::on_startup()
  {
    adw_init();
    Gtk::Application::on_startup();
    auto display = Gdk::Display::get_default();
    m_icon_theme = Gtk::IconTheme::get_for_display(display);
    m_icon_theme->add_search_path(DATADIR"/icons");
    m_icon_theme->add_search_path(DATADIR"/gnote/icons");
  }


  void Gnote::on_activate()
  {
    get_main_window().present();
  }


  int Gnote::on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine> & command_line)
  {
    Gtk::Application::on_command_line(command_line);
    int argc = 0;
    char **argv = command_line->get_arguments(argc);
    GnoteCommandLine passed_cmd_line(*this);
    GnoteCommandLine &cmdline = m_manager ? passed_cmd_line : m_cmd_line;
    cmdline.parse(argc, argv);
    m_is_background = cmdline.background();
    m_is_shell_search = m_cmd_line.shell_search();
    if(!m_manager) {
      common_init();
      register_object();
      end_main();
    }
    else {
      cmdline.set_note_manager(*m_manager);
      if(cmdline.needs_execute()) {
        cmdline.execute();
      }
      else if(!(cmdline.background() || cmdline.shell_search())) {
        if(cmdline.do_new_window()) {
          new_main_window().present();
        }
        else {
          get_main_window().present();
        }
      }
    }

    g_strfreev(argv);
    return 0;
  }


  void Gnote::common_init()
  {
    Glib::ustring note_path = get_note_path(m_cmd_line.note_path());

    //create singleton objects
    m_preferences.init();
    m_manager = new NoteManager(*this);
    m_manager->init(note_path);
    m_action_manager.init();
    m_sync_manager = new sync::SyncManager(*this, default_note_manager());
    m_sync_manager->init();

    m_preferences.signal_color_scheme_changed.connect(sigc::mem_fun(*this, &Gnote::on_color_scheme_pref_changed));
    on_color_scheme_pref_changed();

    m_manager->get_addin_manager().initialize_application_addins();
  }


  void Gnote::end_main()
  {
    m_cmd_line.set_note_manager(*m_manager);
    if(m_cmd_line.needs_execute()) {
      m_cmd_line.execute();
    }

    make_app_actions();
    if(is_background()) {
      // do not exit when all windows are closed
      hold();
      if(m_is_shell_search) {
        set_inactivity_timeout(30000);
        release();
      }
    }
    else {
      get_main_window().present();
    }
  }

  Glib::ustring Gnote::get_note_path(const Glib::ustring & override_path)
  {
    Glib::ustring note_path;
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


  void Gnote::register_object()
  {
    auto conn = get_dbus_connection();
    if(conn) {
      m_remote_control.register_object(conn, *this, default_note_manager());
    }
    else {
      ERR_OUT(_("No D-Bus connection available, shell search and remote control will not work."));
    }
  }


  void Gnote::on_preferences_response(int /*res*/)
  {
    m_prefsdlg.reset();
  }


  void Gnote::on_show_preferences_action(const Glib::VariantBase&)
  {
    if(!m_prefsdlg) {
      m_prefsdlg = std::make_unique<PreferencesDialog>(*this, default_note_manager());
      m_prefsdlg->set_modal(true);
      m_prefsdlg->set_transient_for(get_main_window());
      m_prefsdlg->signal_response().connect(
        sigc::mem_fun(*this, &Gnote::on_preferences_response));
    }
    m_prefsdlg->present();
  }

  void Gnote::on_show_help_action(const Glib::VariantBase&)
  {
    utils::show_help("gnote", "", get_main_window());
  }

  void Gnote::on_show_help_shortcuts_action(const Glib::VariantBase&)
  {
    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_file(DATADIR"/gnote/shortcuts-gnote.ui");
    auto win = builder->get_widget<Gtk::ShortcutsWindow >("shortcuts-gnote");
    if(win == nullptr) {
      ERR_OUT(_("Failed to get shortcuts window!"));
      return;
    }

    win->set_transient_for(get_main_window());
    manage(win)->show();
  }

  void Gnote::on_show_about_action(const Glib::VariantBase&)
  {
    std::vector<Glib::ustring> authors = {
      "Aurimas Černius <aurimas.cernius@mailo.com>",
      "Debarshi Ray <debarshir@src.gnome.org>",
      "Hubert Figuiere <hub@figuiere.net>",
      _("and Tomboy original authors."),
    };

    std::vector<Glib::ustring> contributors = {
      "Iain Nicol <iainn@src.gnome.org>",
      "Kevin Joly <kevin.joly@posteo.net>",
      "Balló György <ballogyor@gmail.com>",
    };
    
    std::vector<Glib::ustring> documenters = {
      "Pierre-Yves Luyten <py@luyten.fr>",
      "Aurimas Černius <aurimas.cernius@mailo.com>",
    };

    Glib::ustring translators(_("translator-credits"));
    if (translators == "translator-credits")
      translators = "";

    auto about = Gtk::make_managed<Gtk::AboutDialog>();
    about->set_name("Gnote");
    about->set_program_name("Gnote");
    about->set_version(VERSION);
    about->set_logo_icon_name(IconManager::GNOTE);
    about->set_copyright(_("Copyright \xc2\xa9 2010-2025 Aurimas Cernius\n"
                           "Copyright \xc2\xa9 2009-2011 Debarshi Ray\n"
                           "Copyright \xc2\xa9 2009 Hubert Figuiere\n"
                           "Copyright \xc2\xa9 2004-2009 the Tomboy original authors."));
    about->set_comments(_("A simple and easy to use desktop note-taking application."));
    /* TRANSLATORS:
     * The link to main wiki page in English. You can localize it in subdirectory for your locale and have this link point to it.
    */
    about->set_website(_("https://gitlab.gnome.org/GNOME/gnote/-/wikis/Gnote"));
    about->set_website_label(_("Homepage"));
    about->set_authors(authors);
    about->set_documenters(documenters);
    about->set_translator_credits(translators);
    about->add_credit_section(_("Contributors"), contributors);
    MainWindow & recent_changes = get_main_window();
    if(recent_changes.get_visible()) {
      about->set_modal(true);
      about->set_transient_for(recent_changes);
      recent_changes.present();
    }
    about->show();
  }

  void Gnote::on_color_scheme_pref_changed()
  {
    auto scheme = m_preferences.color_scheme();
    auto color_scheme = ADW_COLOR_SCHEME_DEFAULT;
    if(scheme == Preferences::COLOR_SCHEME_DARK_VAL) {
      color_scheme = ADW_COLOR_SCHEME_FORCE_DARK;
    }
    else if(scheme == Preferences::COLOR_SCHEME_LIGHT_VAL) {
      color_scheme = ADW_COLOR_SCHEME_FORCE_LIGHT;
    }

    auto style_manager = adw_style_manager_get_default();
    g_object_set(style_manager, "color-scheme", color_scheme, nullptr);
  }

  MainWindow & Gnote::new_main_window()
  {
    NoteRecentChanges *win = new NoteRecentChanges(*this, default_note_manager());
    win->signal_hide().connect([this, win]() { on_main_window_closed(win); });
    add_window(*win);
    auto group = Gtk::WindowGroup::create();
    group->add_window(*win);
    return *win;
  }

  MainWindow & Gnote::get_main_window()
  {
    MainWindow *rc = get_active_window();
    if(rc) {
      return *rc;
    }
    std::vector<Gtk::Window*> windows = Gtk::Window::list_toplevels();
    for(std::vector<Gtk::Window*>::iterator iter = windows.begin();
        iter != windows.end(); ++iter) {
      rc = dynamic_cast<MainWindow*>(*iter);
      if(rc) {
        return *rc;
      }
    }

    return new_main_window();
  }

  void Gnote::on_main_window_closed(Gtk::Window *win)
  {
    delete win;
  }

  MainWindow & Gnote::get_window_for_note()
  {
    std::vector<Gtk::Window*> windows = Gtk::Window::list_toplevels();
    MainWindow *window = NULL;
    for(std::vector<Gtk::Window*>::iterator iter = windows.begin(); iter != windows.end(); ++iter) {
      MainWindow *rc = dynamic_cast<MainWindow*>(*iter);
      if(rc) {
        window = rc;
        if(rc->get_visible()) {
          return *rc;
        }
      }
    }

    if(window) {
      return *window;
    }

    return new_main_window();
  }

  MainWindow & Gnote::open_search_all()
  {
    // if active window is search, just show it
    MainWindow *rc = get_active_window();
    if(rc) {
      if(rc->is_search()) {
        return *rc;
      }
    }

    // present already open search window, if there is one
    std::vector<Gtk::Window*> windows = Gtk::Window::list_toplevels();
    for(std::vector<Gtk::Window*>::iterator iter = windows.begin();
        iter != windows.end(); ++iter) {
      auto win = dynamic_cast<MainWindow*>(*iter);
      if(win) {
        if(win->is_search()) {
          return *win;
        }
      }
    }

    MainWindow & main_window = new_main_window();
    main_window.present_search();
    return main_window;
  }

  void Gnote::open_note_sync_window(const Glib::VariantBase&)
  {
    if(m_sync_dlg == 0) {
      m_sync_dlg = sync::SyncDialog::create(*this, default_note_manager());
      m_sync_dlg->signal_response().connect(sigc::mem_fun(*this, &Gnote::on_sync_dialog_response));
    }

    m_sync_dlg->present_ui();
  }


  void Gnote::on_sync_dialog_response(int)
  {
    m_sync_dlg->hide();
    m_sync_dlg.reset();
  }


  void Gnote::make_app_actions()
  {
    m_action_manager.get_app_action("new-note")->signal_activate().connect(sigc::mem_fun(*this, &Gnote::on_new_note_app_action));
    m_action_manager.get_app_action("new-window")->signal_activate().connect(sigc::mem_fun(*this, &Gnote::on_new_window_action));
    m_action_manager.get_app_action("show-preferences")->signal_activate().connect(
      sigc::mem_fun(*this, &Gnote::on_show_preferences_action));
    m_action_manager.get_app_action("sync-notes")->signal_activate().connect(sigc::mem_fun(*this, &Gnote::open_note_sync_window));
    m_action_manager.get_app_action("help-contents")->signal_activate().connect(sigc::mem_fun(*this, &Gnote::on_show_help_action));
    m_action_manager.get_app_action("help-shortcuts")->signal_activate().connect(sigc::mem_fun(*this, &Gnote::on_show_help_shortcuts_action));
    m_action_manager.get_app_action("about")->signal_activate().connect(sigc::mem_fun(*this, &Gnote::on_show_about_action));

    add_app_actions(m_action_manager.get_app_actions());
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
    new_main_window().present();
  }


  MainWindow *Gnote::get_active_window()
  {
    std::vector<Gtk::Window*> windows = Gtk::Window::list_toplevels();
    for(std::vector<Gtk::Window*>::iterator iter = windows.begin(); iter != windows.end(); ++iter) {
      if((*iter)->property_is_active()) {
        return dynamic_cast<MainWindow*>(*iter);
      }
    }

    return NULL;
  }


  void Gnote::on_new_note_app_action(const Glib::VariantBase&)
  {
    MainWindow *rc = get_active_window();
    if(rc) {
      rc->new_note();
    }
    else {
      MainWindow & recent_changes = get_main_window();
      recent_changes.new_note();
      recent_changes.present();
    }
  }


  void Gnote::open_note(const NoteBase & note)
  {
    auto & n = const_cast<NoteBase&>(note);
    MainWindow::present_in(get_window_for_note(), static_cast<Note&>(n));
  }

  notebooks::NotebookManager & Gnote::notebook_manager()
  {
    return m_manager->notebook_manager();
  }


  GnoteCommandLine::GnoteCommandLine(IGnote & ignote)
    : m_context(g_option_context_new("Foobar"))
    , m_gnote(ignote)
    , m_manager(NULL)
    , m_use_panel(false)
    , m_background(false)
    , m_shell_search(false)
    , m_note_path(NULL)
    , m_do_search(false)
    , m_show_version(false)
    , m_do_new_note(false)
    , m_do_new_window(false)
    , m_open_note(NULL)
    , m_open_start_here(false)
    , m_highlight_search(NULL)
  {
    const GOptionEntry entries[] =
      {
        { "background", 0, G_OPTION_ARG_NONE, G_OPTION_ARG_NONE, &m_background, _("Run Gnote in background."), NULL },
        { "shell-search", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &m_shell_search, _("Run Gnote as GNOME Shell search provider."), NULL },
        { "note-path", 0, 0, G_OPTION_ARG_STRING, &m_note_path, _("Specify the path of the directory containing the notes."), _("path") },
        { "search", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, (void*)GnoteCommandLine::parse_func, _("Open the search all notes window with the search text."), _("text") },
        { "version", 0, 0, G_OPTION_ARG_NONE, &m_show_version, _("Print version information."), NULL },
        { "new-note", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, (void*)GnoteCommandLine::parse_func, _("Create and display a new note, with a optional title."), _("title") },
        { "new-window", 0, 0, G_OPTION_ARG_NONE, &m_do_new_window, _("Open a new window."), NULL },
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

    RemoteControl *remote_control = static_cast<Gnote&>(m_gnote).remote_control().get_remote_control();
    if(remote_control) {
      execute(remote_control);
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
      Glib::ustring new_uri;

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
      Glib::ustring note_id = sharp::file_basename(m_open_external_note_path);
      if (!note_id.empty()) {
        // Attempt to load the note, assuming it might already
        // be part of our notes list.
        if (!display_note(remote, "note://gnote/" + note_id)) {
          sharp::StreamReader sr;
          sr.init(m_open_external_note_path);
          if (sr.file()) {
            Glib::ustring noteTitle;
            Glib::ustring noteXml;
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
              noteTitle = m_manager->note_archiver().get_title_from_note_xml (noteXml);
              if (!noteTitle.empty()) {
                // Check for conflicting titles
                Glib::ustring baseTitle = noteTitle;
                for (int i = 1; !remote->FindNote (noteTitle).empty(); i++) {
                  noteTitle = Glib::ustring::compose("%1 (%2)", baseTitle, i);
                }

                Glib::ustring note_uri = remote->CreateNamedNote(noteTitle);

                // Update title in the note XML
                noteXml = m_manager->note_archiver().get_renamed_note_xml (noteXml, baseTitle, noteTitle);

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
    // TRANSLATORS: %1: format placeholder for the version string.
    Glib::ustring version = Glib::ustring::compose(_("Version %1"), VERSION);
    std::cerr << version << std::endl;
  }


  template <typename T>
  bool GnoteCommandLine::display_note(T & remote, Glib::ustring uri)
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
