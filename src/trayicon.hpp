

#ifndef _TRAYICON_HPP_
#define _TRAYICON_HPP_

#include <boost/shared_ptr.hpp>

#include <gtkmm/statusicon.h>

#include "notemanager.hpp"
#include "prefskeybinder.hpp"

namespace gnote {

class TrayIcon;

class Tray
{
public:
	Tray(const boost::shared_ptr<NoteManager> &, TrayIcon &);

	Gtk::Menu * make_tray_notes_menu();
	Gtk::Menu * tray_menu() 
		{ return m_tray_menu; }
private:
	boost::shared_ptr<NoteManager> m_manager;
	TrayIcon & m_trayicon;
	Gtk::Menu * m_tray_menu;
};


class TrayIcon
	: public Gtk::StatusIcon
{
public:
	TrayIcon(const boost::shared_ptr<NoteManager> & manager);
	~TrayIcon();

	boost::shared_ptr<Tray> tray() const
		{ return m_tray; }

	void show_menu(bool select_first_item);

	void get_tray_menu_pos(int & x, int &y, bool & push_in);
	Gtk::Menu * get_right_click_menu();

	void on_activate();
	void on_popup_menu(guint button, guint32 activate_time);
	void on_exit();

	void show_preferences();
	void show_help_contents();
	void show_about();
	void quit();
private:
	boost::shared_ptr<Tray> m_tray;
	boost::shared_ptr<PrefsKeybinder> m_keybinder;
	Gtk::Menu               *m_context_menu;
};


}
#endif
