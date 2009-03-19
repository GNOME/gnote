


#ifndef __NOTE_BUFFER_HPP_
#define __NOTE_BUFFER_HPP_

#include <gtkmm/textbuffer.h>

namespace gnote {

	class NoteTagTable;
	class Note;
	class UndoManager;


class NoteBuffer 
	: public Gtk::TextBuffer
{
public:
////
	NoteBuffer(const NoteTagTable*, Note &)
		{}
	UndoManager * undoer() const
		{ return NULL; }
	std::string get_selection() const
		{ return ""; }
	void change_cursor_depth_directional(bool)
		{}
	void increase_cursor_depth()
		{}
	void decrease_cursor_depth()
		{}
	void toggle_selection_bullets()
		{}
	bool is_active_tag(const std::string & )
		{ return false; }
	void remove_active_tag(const std::string &)
		{}
	void set_active_tag(const std::string &)
		{}
	void toggle_active_tag(const std::string &)
		{}
	bool is_bulleted_list_active()
		{ return false; }
	bool can_make_bulleted_list()
		{ return false; }
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

