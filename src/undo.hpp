/*
 * gnote
 *
 * Copyright (C) 2009 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



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
