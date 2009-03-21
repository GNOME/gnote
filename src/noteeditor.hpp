


#ifndef __NOTE_EDITOR_HPP_
#define __NOTE_EDITOR_HPP_

#include <glibmm/refptr.h>
#include <gtkmm/textview.h>

#include "notebuffer.hpp"

namespace gnote {

class NoteEditor
	: public Gtk::TextView
{
public:
	typedef Glib::RefPtr<NoteEditor> Ptr;

	NoteEditor(const NoteBuffer::Ptr &)
		{
		}

};


}

#endif
