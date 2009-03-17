


#ifndef __TAG_HPP_
#define __TAG_HPP_

namespace gnote {

	class Note;

	class Tag 
	{
	public:
		typedef boost::shared_ptr<Tag> Ptr;

		void add_note(Note* n)
			{ }
		std::string name()
			{
				return "";
			}
		std::string normalized_name()
			{ return ""; }
		void remove_note(Note* n)
			{ }

	};


}

#endif
