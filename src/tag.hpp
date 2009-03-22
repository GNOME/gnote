


#ifndef __TAG_HPP_
#define __TAG_HPP_

#include <string>
#include <tr1/memory>

namespace gnote {

	class Note;

	class Tag 
	{
	public:
		typedef std::tr1::shared_ptr<Tag> Ptr;
		static const char * SYSTEM_TAG_PREFIX;

		Tag(const std::string & name);
		~Tag();

		// <summary>
		// Associates the specified note with this tag.
		// </summary>
		void add_note(Note & );
		// <summary>
		// Unassociates the specified note with this tag.
		// </summary>
		void remove_note(const Note & );
    // <summary>
		// The name of the tag.  This is what the user types in as the tag and
		// what's used to show the tag to the user. This includes any 'system:' prefixes
		// </summary>
		const std::string & name() const
			{
				return m_name;
			}
		void set_name(const std::string & );
		// <summary>
		// Use the string returned here to reference the tag in Dictionaries.
		// </summary>
		const std::string & normalized_name() const
			{ 
				return m_normalized_name; 
			}
	 	/// <value>
		/// Is Tag a System Value
		/// </value>
		bool is_system() const
			{
				return m_issystem;
			}
		/// <value>
		/// Is Tag a Property?
		/// </value>
		bool is_property() const
			{
				return m_isproperty;
			}
		// <summary>
		// Returns the number of notes this is currently tagging.
		// </summary>
		int popularity() const;
		/// remove all notes. Because the note map is private.
		void remove_all_notes();
/////

	private:
		class NoteMap;
		std::string m_name;
		std::string m_normalized_name;
		bool        m_issystem;
		bool        m_isproperty;
    // <summary>
		// Used to track which notes are currently tagged by this tag.  The
		// dictionary key is the Note.Uri.
		// </summary>
		NoteMap *   m_notes;
	};


}

#endif
