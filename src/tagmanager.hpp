


#ifndef __TAG_MANAGER_HPP_
#define __TAG_MANAGER_HPP_


#include <sigc++/signal.h>

#include <gtkmm/liststore.h>
#include <gtkmm/treemodelsort.h>

#include "tag.hpp"


namespace gnote {

class TagManager
{
public:
	static TagManager & instance();

	static const char * TEMPLATE_NOTE_SYSTEM_TAG;
	Tag::Ptr get_tag (const std::string & tag_name) const;
	Tag::Ptr get_or_create_tag(const std::string &);
	Tag::Ptr get_system_tag (const std::string & tag_name) const;
	Tag::Ptr get_or_create_system_tag(const std::string & name);
	void remove_tag (const Tag::Ptr & tag);
	Glib::RefPtr<Gtk::TreeModel> get_tags() const
		{
			return m_sorted_tags;
		}
	std::list<Tag::Ptr> all_tags();
private:
	TagManager();

	class ColumnRecord
		: public Gtk::TreeModelColumnRecord
	{
	public:
		ColumnRecord()
			{
				add(m_tag);
			}
		Gtk::TreeModelColumn<Tag::Ptr> m_tag;
	};
	ColumnRecord                     m_columns;
	Glib::RefPtr<Gtk::ListStore>     m_tags;
	Glib::RefPtr<Gtk::TreeModelSort> m_sorted_tags;
	// <summary>
	// The key for this dictionary is Tag.Name.ToLower ().
	// </summary>
  typedef std::map<std::string, Gtk::TreeIter> TagMap;
	TagMap m_tag_map;
  typedef std::map<std::string, Tag::Ptr> InternalMap;
	InternalMap m_internal_tags;
	
	sigc::signal<void, Tag::Ptr, const Gtk::TreeIter &> m_signal_tag_added;
	sigc::signal<void, const std::string &> m_signal_tag_removed;
};

}

#endif
