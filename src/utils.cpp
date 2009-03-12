

#include <iostream>

#include <gtkmm/icontheme.h>

#include "utils.hpp"

namespace gnote {
	namespace utils {

		static void deactivate_menu(Gtk::Menu *menu)
		{
			menu->popdown();
			if(menu->get_attach_widget()) {
				menu->get_attach_widget()->set_state(Gtk::STATE_NORMAL);
			}
		}

		void popup_menu(Gtk::Menu *menu, const GdkEventButton * ev, 
										Gtk::Menu::SlotPositionCalc calc)
		{
			menu->signal_deactivate().connect(sigc::bind(&deactivate_menu, menu));
			menu->popup(calc, (ev ? ev->button : NULL), 
									(ev ? ev->time : gtk_get_current_event_time()));
			if(menu->get_attach_widget()) {
				menu->get_attach_widget()->set_state(Gtk::STATE_SELECTED);
			}
		}


		Glib::RefPtr<Gdk::Pixbuf> get_icon(const std::string & name, int size)
		{
			try {
				return Gtk::IconTheme::get_default()->load_icon(name, size);
			}
			catch(const Glib::Exception & e)
			{
				std::cout << e.what().c_str() << std::endl;
			}
			return Glib::RefPtr<Gdk::Pixbuf>();
		}

	}
}
