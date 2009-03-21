

#ifndef __NOTE_TAG_HPP_
#define __NOTE_TAG_HPP_

#include <glibmm/refptr.h>
#include <gtkmm/texttag.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/texttagtable.h>

namespace gnote {

class NoteTag
	: public Gtk::TextTag
{
public:
	typedef Glib::RefPtr<NoteTag> Ptr;
	typedef Glib::RefPtr<const NoteTag> ConstPtr;

	std::string get_element_name() const
		{ return ""; }
	Gtk::Widget * get_widget() const
		{ return NULL; }
	Glib::RefPtr<Gtk::TextMark> get_widget_location() const
		{ return Glib::RefPtr<Gtk::TextMark>(); }
	void set_widget_location(const Glib::RefPtr<Gtk::TextMark> &)
		{  }
};


class DepthNoteTag
	: public NoteTag
{
public:
	typedef Glib::RefPtr<DepthNoteTag> Ptr;
	int get_depth()
		{ return 0; }
	PangoDirection get_direction()
		{
			return PANGO_DIRECTION_LTR;
		}
};

class DynamicNoteTag
	: public NoteTag
{
public:
	typedef Glib::RefPtr<DynamicNoteTag> Ptr;
	typedef Glib::RefPtr<const DynamicNoteTag> ConstPtr;
};

class NoteTagTable
	: public Gtk::TextTagTable
{
public:
	typedef Glib::RefPtr<NoteTagTable> Ptr;
// TODO
	DepthNoteTag::Ptr get_depth_tag(int depth, PangoDirection direction)
		{ return DepthNoteTag::Ptr(); }
	static bool tag_is_serializable(const Glib::RefPtr<Gtk::TextBuffer::Tag> & )
		{ return false; }
	static bool tag_is_growable(const Glib::RefPtr<Gtk::TextBuffer::Tag> & )
		{ return false; }
	static bool tag_has_depth(const Glib::RefPtr<Gtk::TextBuffer::Tag> & )
		{ return false; }
};


}

#endif

