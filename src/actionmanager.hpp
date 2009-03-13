


#ifndef __ACTIONMANAGER_HPP_
#define __ACTIONMANAGER_HPP_

#include <string>
#include <list>

#include <gtkmm/action.h>
#include <gtkmm/uimanager.h>
#include <gdkmm/pixbuf.h>

namespace gnote {

class ActionManager
{
public:
	ActionManager();

	static ActionManager * get_manager()
		{
			if(!s_instance) {
				s_instance = new ActionManager();
			}
			return s_instance;
		}
	Glib::RefPtr<Gtk::Action> operator[](const std::string & n)
		{
			return find_action_by_name(n);
		}
	Gtk::Widget * get_widget(const std::string &n)
		{
			return m_ui->get_widget(n);
		}
	void load_interface();
	std::list<Gtk::Widget*> get_placeholder_children(const std::string & p);
	void populate_action_groups();
	Glib::RefPtr<Gtk::Action> find_action_by_name(const std::string & n);

private:
	static ActionManager * s_instance;
	Glib::RefPtr<Gtk::UIManager> m_ui;
	Glib::RefPtr<Gtk::ActionGroup> m_main_window_actions;
	Glib::RefPtr<Gdk::Pixbuf> m_newNote;
};


}

#endif

