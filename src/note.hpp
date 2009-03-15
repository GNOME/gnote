


#ifndef __NOTE_HPP_
#define __NOTE_HPP_

#include <list>
#include <string>
#include <boost/shared_ptr.hpp>

#include <sigc++/signal.h>
#include <gtkmm/textbuffer.h>

#include "tag.hpp"

namespace gnote {

class NoteManager;

class Note 
{
public:
	typedef boost::shared_ptr<Note> Ptr;
	typedef std::list<Ptr> List;

	typedef sigc::signal<void, const Note::Ptr&, const std::string& > RenamedHandler;
	typedef sigc::signal<void, const Note::Ptr&> SavedHandler;

	typedef enum {
		NO_CHANGE,
		CONTENT_CHANGED,
		OTHER_DATA_CHANGED
	} ChangeType;

	// TODO
	void queue_save(ChangeType c)
		{
		}
	bool is_pinned() const
		{ return m_pinned; }
	void set_pinned(bool pinned)
		{ m_pinned = pinned; }
	bool is_new() const
		{ return true; }
	bool is_special() const
		{ return false; }
	bool is_opened() const
		{ return false; }
	bool is_open_on_startup() const
		{ return false; }
	void set_is_open_on_startup(bool)
		{ }
	bool contains_tag(const Tag::Ptr &) const
		{ return false; }
	void add_tag(const Tag::Ptr &)
		{}
	GDate * change_date() const
		{ return NULL; }
	std::string title() const
		{ return ""; }
	std::string uri() const
		{ return ""; }
	static Note::Ptr create_new_note(const std::string & title,
																	 const std::string & filename,
																	 NoteManager & manager)
		{
			return Note::Ptr(new Note(title, false));
		}
	static Note::Ptr load(const std::string &, NoteManager &)
		{
			return Note::Ptr();
		}
	static std::string file_path()
		{
			return "";
		}
	void save()
		{}
	void delete_note()
		{}
	std::string & xml_content()
		{
			return m_xml_content;
		}
	Glib::RefPtr<Gtk::TextBuffer> get_buffer()
		{
			return Glib::RefPtr<Gtk::TextBuffer>();
		}
	RenamedHandler & signal_renamed()
		{ return m_signal_renamed; }
	SavedHandler & signal_saved()
		{ return m_signal_saved; }
private:
	Note(const std::string & _title, bool pinned = false)
		: m_title(_title)
		, m_pinned(pinned)
		{
		}

	std::string m_title;
	bool m_pinned;
	std::string m_xml_content;
	RenamedHandler m_signal_renamed;
	SavedHandler m_signal_saved;
};



}

#endif
