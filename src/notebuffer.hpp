


#ifndef __NOTE_BUFFER_HPP_
#define __NOTE_BUFFER_HPP_

#include <gtkmm/textbuffer.h>

namespace gnote {

	class NoteTagTable;
	class Note;


class NoteBuffer 
	: public Gtk::TextBuffer
{
public:
	NoteBuffer(const NoteTagTable*, Note &)
		{}

};

class NoteBufferArchiver
{
public:
	static std::string serialize(const Glib::RefPtr<Gtk::TextBuffer> & )
		{
			return "";
		}
	static void deserialize(const Glib::RefPtr<Gtk::TextBuffer> &buffer,
													const std::string & content)
		{
			deserialize(buffer, buffer->begin(), content);
		}
	static void deserialize(const Glib::RefPtr<Gtk::TextBuffer> &, const Gtk::TextIter & ,
													const std::string & )
		{
			
		}
};


}

#endif

