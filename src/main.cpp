

#include <gtkmm/main.h>

#include "gnote.hpp"

int main(int argc, char **argv)
{
//	if(!Glib::thread_supported()) {
//		Glib::thread_init();
//	}
	Gtk::Main kit(argc, argv);
	gnote::Gnote app;
	return app.main(argc, argv);
}
