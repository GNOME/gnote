

#ifndef _TRAYICON_HPP_
#define _TRAYICON_HPP_

#include <boost/shared_ptr.hpp>

#include <gtkmm/statusicon.h>
#include <gtkmm/imagemenuitem.h>

#include "notemanager.hpp"
#include "prefskeybinder.hpp"
#include "note.hpp"

namespace gnote {

class TrayIcon;

class NoteMenuItem 
	: public Gtk::ImageMenuItem
{
public:
	NoteMenuItem(const Note::Ptr & note, bool show_pin);

protected:
	virtual void on_activate();
	virtual bool on_button_press_event(GdkEventButton *);
	virtual bool on_button_release_event(GdkEventButton *);
	virtual bool on_motion_notify_event(GdkEventMotion *);
	virtual bool on_leave_notify_event(GdkEventCrossing *);

private:
	Note::Ptr   m_note;
	Gtk::Image *m_pin_img;
	bool        m_pinned;
	bool        m_inhibit_activate;

	static std::string format_for_label (const std::string & name);
	static std::string get_display_name(const Note::Ptr & n);
	static std::string ellipsify (const std::string & str, size_t max);
	static void _init_static();

	static bool                      s_static_inited;
	static Glib::RefPtr<Gdk::Pixbuf> s_note_icon;
	static Glib::RefPtr<Gdk::Pixbuf> s_pinup;
	static Glib::RefPtr<Gdk::Pixbuf> s_pinup_active;
	static Glib::RefPtr<Gdk::Pixbuf> s_pindown;
};

class Tray
{
public:
	Tray(const boost::shared_ptr<NoteManager> &, TrayIcon &);

	Gtk::Menu * make_tray_notes_menu();
	Gtk::Menu * tray_menu() 
		{ return m_tray_menu; }
	void update_tray_menu(Gtk::Widget * parent);
	void remove_recently_changed_notes();
	void add_recently_changed_notes();
private:
	boost::shared_ptr<NoteManager> m_manager;
	TrayIcon & m_trayicon;
	Gtk::Menu *m_tray_menu;
	bool       m_menu_added;
	std::list<Gtk::MenuItem*> m_recent_notes;
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
	bool on_exit();
	bool menu_opens_upward();

	void show_preferences();
	void show_help_contents();
	void show_about();
	void quit();
private:
	boost::shared_ptr<Tray> m_tray;
	boost::shared_ptr<PrefsKeybinder> m_keybinder;
	Gtk::Menu               *m_context_menu;
};

class GConfKeybindingToAccel
{
public:
	static std::string get_shortcut (const std::string & gconf_path);
	static void add_accelerator (Gtk::MenuItem & item, const std::string & gconf_path);

	static Glib::RefPtr<Gtk::AccelGroup> get_accel_group();
private:
	static Glib::RefPtr<Gtk::AccelGroup> s_accel_group;
};

}
#endif
