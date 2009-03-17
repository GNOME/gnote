


#include <gtkmm/texttagtable.h>

namespace gnote {

class NoteTagTable
	: public Gtk::TextTagTable
{
public:
// TODO
	static bool tag_is_serializable(const Glib::RefPtr<Gtk::TextBuffer::Tag> & )
		{ return false; }
};


}
