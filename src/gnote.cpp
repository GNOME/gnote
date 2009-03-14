

#include <stdlib.h>

#include <boost/algorithm/string/replace.hpp>

#include <glibmm/thread.h>
#include <gtkmm/main.h>

#include "config.h"

#include "gnote.hpp"
#include "actionmanager.hpp"
#include "preferencesdialog.hpp"


int main(int argc, char **argv)
{
//	if(!Glib::thread_supported()) {
//		Glib::thread_init();
//	}
	Gtk::Main kit(argc, argv);
	gnote::Gnote app;
	return app.main(argc, argv);
}


namespace gnote {

	Gnote::Gnote()
		: m_tray_icon_showing(false)
		, m_is_panel_applet(false)
		, m_prefsdlg(NULL)
	{
	}

	Gnote::~Gnote()
	{
		delete m_prefsdlg;
	}


	int Gnote::main(int argc, char **argv)
	{
		GnoteCommandLine cmd_line(argc, argv);

		if(cmd_line.needs_execute()) {
			cmd_line.execute();
		}

		m_icon_theme = Gtk::IconTheme::get_default();
		m_icon_theme->append_search_path(DATADIR"/icons");
		m_icon_theme->append_search_path(DATADIR"/gnote/icons");

		std::string note_path = get_note_path(cmd_line.note_path());
		m_manager.reset(new NoteManager(note_path));

		// TODO
		// SyncManager::init()

		ActionManager & am = *ActionManager::get_manager();
		am.load_interface();
//		register_remote_control(m_manager);
		setup_global_actions();
		
		// TODO
		// addins. load + init

		if(cmd_line.use_panel_applet()) {
			m_tray_icon_showing = true;
			m_is_panel_applet = true;

			am["CloseWindowAction"]->set_visible(true);
			am["QuitGNoteAction"]->set_visible(false);
			
			// register panel applet factory
			return 0;

		}
		else {
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
			boost::algorithm::replace_first(note_path, "~", home_dir);
		}

		return note_path;
	}

	void Gnote::start_tray_icon()
	{
		// Create the tray icon and run the main loop
		m_tray_icon = Glib::RefPtr<TrayIcon>(new TrayIcon(m_manager));
		m_tray = m_tray_icon->tray();

		// Give the TrayIcon 2 seconds to appear.  If it
		// doesn't by then, open the SearchAllNotes window.
		m_tray_icon_showing = m_tray_icon->is_embedded() 
			&& m_tray_icon->get_visible();
			
		if (!m_tray_icon_showing) {
			Glib::RefPtr<Glib::TimeoutSource> timeout 
				= Glib::TimeoutSource::create(2000);
			timeout->connect(sigc::mem_fun(*this, &Gnote::check_tray_icon_showing));
		}
		
		Gtk::Main::run();
	}


	bool Gnote::check_tray_icon_showing()
	{
		if(!m_tray_icon_showing) {
			ActionManager & am = *ActionManager::get_manager();
			am["ShowSearchAllNotesAction"]->activate();
		}
		return false;
	}


	void Gnote::setup_global_actions()
	{
		ActionManager & am = *ActionManager::get_manager();
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
			m_prefsdlg = new PreferencesDialog(/*addin manager*/);
			m_prefsdlg->signal_response().connect(sigc::mem_fun(*this, &Gnote::on_preferences_response));
		}
		m_prefsdlg->run();
	}

	void Gnote::on_show_help_action()
	{
	}

	void Gnote::on_show_about_action()
	{
	}

	void Gnote::open_search_all()
	{
	}

	void Gnote::open_note_sync_window()
	{
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


	GnoteCommandLine::GnoteCommandLine(int & argc, char **&argv)
		:	m_new_note(false)
		,	m_open_search(false)
		, m_open_start_here(false)
		, m_use_panel(false)
	{
		parse(argc, argv);
	}

	int GnoteCommandLine::execute()
	{
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

	void GnoteCommandLine::parse(int & argc, char **&argv)
	{
		bool quit = false;
		for(int i = 0; i < argc; i++) {
			const char * current = argv[i];
			// TODO
		}
	}
}
