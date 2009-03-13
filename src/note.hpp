


#ifndef __NOTE_HPP_
#define __NOTE_HPP_

#include <list>
#include <string>
#include <boost/shared_ptr.hpp>

#include "tag.hpp"

namespace gnote {

class Note 
{
public:
	typedef boost::shared_ptr<Note> Ptr;
	typedef std::list<Ptr> List;

	Note(const std::string & _title, bool pinned = false)
		: m_title(_title)
		, m_pinned(pinned)
		{
		}
	// TODO
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
	bool contains_tag(const Tag::Ptr &)
		{ return false; }
	GDate * change_date() const
		{ return NULL; }
	std::string title() const
		{ return ""; }
private:
	std::string m_title;
	bool m_pinned;
};

}

#endif
