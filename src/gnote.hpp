


#ifndef _GNOTE_HPP_
#define _GNOTE_HPP_

#include <string>

#include <boost/shared_ptr.hpp>

#include <gtkmm/icontheme.h>
#include <gtkmm/statusicon.h>

#include "notemanager.hpp"
#include "actionmanager.hpp"
#include "tray.hpp"

namespace gnote {

class PreferencesDialog;

class Gnote
{
public:
	Gnote();
	~Gnote();
	int main(int argc, char **argv);
	std::string get_note_path(const std::string & override_path);

	void setup_global_actions();
	void start_tray_icon();
	bool check_tray_icon_showing();

	void on_new_note_action();
	void on_quit_gnote_action();
	void on_preferences_response(int res);
	void on_show_preferences_action();
	void on_show_help_action();
	void on_show_about_action();
	void open_search_all();
	void open_note_sync_window();

	static std::string conf_dir();
private:
	Glib::RefPtr<Gtk::IconTheme> m_icon_theme;
	boost::shared_ptr<NoteManager> m_manager;
	bool m_tray_icon_showing;
	Glib::RefPtr<TrayIcon> m_tray_icon;
	boost::shared_ptr<Tray> m_tray;
	bool m_is_panel_applet;
	PreferencesDialog *m_prefsdlg;
};


class GnoteCommandLine
{
public:
	GnoteCommandLine(int & argc, char **&argv);
	int execute();

	const std::string & note_path() const
		{
			return m_note_path;
		}
	bool        needs_execute() const;
	bool        use_panel_applet() const
		{
			return m_use_panel;
		}

private:
	void parse(int & argc, char **&argv);

	bool        m_new_note;
	bool        m_open_search;
	bool        m_open_start_here;
	bool        m_use_panel;
	std::string m_open_note_name;
	std::string m_open_note_uri;
	std::string m_open_external_note_path;
	std::string m_note_path;
};

}

#endif
