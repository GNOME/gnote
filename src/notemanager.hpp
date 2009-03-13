

#ifndef _NOTEMANAGER_HPP__
#define _NOTEMANAGER_HPP__

#include <string>

#include "note.hpp"

namespace gnote {

	class NoteManager 
	{
	public:
		NoteManager(const std::string & )
			{}
		// TODO
		const Note::List get_notes()
			{ 
				Note::List l;
				bool pinned = false;

				for(int i = 0; i < 4; i++) {
					Note::Ptr n(new Note("foo", pinned));
					pinned = !pinned;
					l.push_back(n);
				}
				return l;
			}

		static std::string start_note_uri()
			{ return ""; }
		Note::Ptr find_by_uri(const std::string &)
			{ return Note::Ptr(); }
	};

}

#endif

