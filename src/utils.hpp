

#ifndef _GNOTE_UTILS_HPP__
#define _GNOTE_UTILS_HPP__

#include <gdkmm/pixbuf.h>
#include <gtkmm/menu.h>


namespace gnote {
	namespace utils {

		Glib::RefPtr<Gdk::Pixbuf> get_icon(const std::string & , int );

		void popup_menu(Gtk::Menu *menu, const GdkEventButton *, Gtk::Menu::SlotPositionCalc calc);
	}
}

#endif
