

#ifndef __UNDO_HPP_
#define __UNDO_HPP_

#include <boost/noncopyable.hpp>
#include <sigc++/signal.h>

namespace gnote {


class UndoManager
	: public boost::noncopyable
{
public:


//
	void undo()
		{}
	void redo()
		{}
	bool get_can_undo()
		{ return false; }
	bool get_can_redo()
		{ return false; }
	void thaw_undo()
		{}
	void freeze_undo()
		{}

	sigc::signal<void> & signal_undo_changed()
		{ return m_undo_changed; }

private:
	sigc::signal<void> m_undo_changed;
};


}

#endif
