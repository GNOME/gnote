


#ifndef _NOTEWINDOW_HPP__
#define _NOTEWINDOW_HPP__

#include <gtkmm/window.h>

namespace gnote {

	class Note;

class NoteWindow 
	: public Gtk::Window
{
public:
	NoteWindow(Note &)
		{}

};


}

#endif
