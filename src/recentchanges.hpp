

#ifndef __NOTE_RECENT_CHANGES_HPP_
#define __NOTE_RECENT_CHANGES_HPP_

#include <string>


namespace gnote {

	class NoteManager;

class NoteRecentChanges
{
public:
	static NoteRecentChanges *get_instance(NoteManager&)
		{
			static NoteRecentChanges * s_instance = new NoteRecentChanges();
			return s_instance;
		}

	void set_search_text(const std::string &)
		{}
	void present()
		{}
};


}

#endif
