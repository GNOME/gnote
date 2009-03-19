

#ifndef __UNDO_HPP_
#define __UNDO_HPP_


#include <sigc++/signal.h>

namespace gnote {


class UndoManager
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

	sigc::signal<void> & signal_undo_changed()
		{ return m_undo_changed; }

private:
	sigc::signal<void> m_undo_changed;
};


}

#endif
