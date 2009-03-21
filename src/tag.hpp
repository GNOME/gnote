


#ifndef __TAG_HPP_
#define __TAG_HPP_

#include <tr1/memory>

namespace gnote {

	class Note;

	class Tag 
	{
	public:
		typedef std::tr1::shared_ptr<Tag> Ptr;

		void add_note(Note* )
			{ }
		std::string name()
			{
				return "";
			}
		std::string normalized_name()
			{ return ""; }
		void remove_note(Note* )
			{ }

	};


}

#endif
