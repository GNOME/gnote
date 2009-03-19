


#ifndef __NOTE_EDITOR_HPP_
#define __NOTE_EDITOR_HPP_

#include <gtkmm/textview.h>

namespace gnote {

class NoteEditor
	: public Gtk::TextView
{
public:
	NoteEditor(const Glib::RefPtr<NoteBuffer>&)
		{
		}

};


}

#endif
