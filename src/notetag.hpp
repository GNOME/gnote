

#ifndef __NOTE_TAG_HPP_
#define __NOTE_TAG_HPP_

#include <string>

#include <sigc++/signal.h>
#include <glibmm/refptr.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/texttag.h>
#include <gtkmm/texttagtable.h>

#include "contrast.hpp"
#include "sharp/exception.hpp"

namespace sharp {
	class XmlWriter;
}

namespace xmlpp {
	class TextReader;
}

namespace gnote {
	class NoteEditor;




class NoteTag
	: public Gtk::TextTag
{
public:
	typedef Glib::RefPtr<NoteTag> Ptr;
	typedef Glib::RefPtr<const NoteTag> ConstPtr;
	typedef sigc::signal<bool, const NoteTag &, const NoteEditor &,
											 const Gtk::TextIter &, const Gtk::TextIter &> TagActivatedHandler;

	enum TagFlags {
		NO_FLAG       = 0,
		CAN_SERIALIZE = 1,
		CAN_UNDO      = 2,
		CAN_GROW      = 4,
		CAN_SPELL_CHECK = 8,
		CAN_ACTIVATE  = 16,
		CAN_SPLIT     = 32
	};

	static Ptr create(const std::string & tag_name, int flags) throw(sharp::Exception)
		{
			return Ptr(new NoteTag(tag_name, flags));
		}
	const std::string & get_element_name() const
		{ 
			return m_element_name;
		}
	bool can_serialize() const
		{
			return (m_flags & CAN_SERIALIZE) != 0;
		}
	void set_can_serialize(bool value);
	bool can_undo() const
		{
			return (m_flags & CAN_UNDO) != 0;
		}
	void set_can_undo(bool value);
	bool can_grow() const
		{
			return (m_flags & CAN_GROW) != 0;
		}
	void set_can_grow(bool value);
	bool can_spell_check() const
		{
			return (m_flags & CAN_SPELL_CHECK) != 0;
		}
	void set_can_spell_check(bool value);
	bool can_activate() const
		{
			return (m_flags & CAN_ACTIVATE) != 0;
		}
	void set_can_activate(bool value);
	bool can_split() const
		{
			return (m_flags & CAN_SPLIT) != 0;
		}
	void set_can_split(bool value);

	void get_extents(const Gtk::TextIter & iter, Gtk::TextIter & start,
									 Gtk::TextIter & end);
	virtual void write(sharp::XmlWriter &, bool) const;
	virtual void read(xmlpp::TextReader &, bool);
	virtual Glib::RefPtr<Gdk::Pixbuf> get_image() const;
	virtual void set_image(const Glib::RefPtr<Gdk::Pixbuf> &);
	virtual Gtk::Widget * get_widget() const
		{
			return m_widget;
		}
	virtual void set_widget(Gtk::Widget *);
	const Glib::RefPtr<Gtk::TextMark> & get_widget_location() const
		{ 
			return m_widget_location; 
		}
	void set_widget_location(const Glib::RefPtr<Gtk::TextMark> & value)
		{ 
			m_widget_location = value; 
		}
////
	TagActivatedHandler & signal_activate()
		{ 
			return m_signal_activate;
		}
	sigc::signal<void,const Glib::RefPtr<Gtk::TextTag>&,bool> & signal_changed()
		{ 
			return m_signal_changed;
		}
	ContrastPaletteColor get_palette_foreground() const
		{
			return m_palette_foreground;
		}
	void set_palette_foreground(ContrastPaletteColor value)
		{
			m_palette_foreground = value;
    	// XXX We should also watch theme changes.
			property_foreground_gdk().set_value(render_foreground(value));
		}
protected:
	NoteTag(const std::string & tag_name, int flags = 0) throw(sharp::Exception);

	virtual bool on_event(const Glib::RefPtr<Glib::Object> &, GdkEvent *, const Gtk::TextIter & );
	virtual bool on_activate(const NoteEditor & , const Gtk::TextIter &, const Gtk::TextIter &);
private:
	Gdk::Color get_background() const;
	Gdk::Color render_foreground(ContrastPaletteColor symbol);

	std::string         m_element_name;
	Glib::RefPtr<Gtk::TextMark> m_widget_location;
	Gtk::Widget       * m_widget;
	int                 m_flags;
	TagActivatedHandler m_signal_activate;
	ContrastPaletteColor m_palette_foreground;
	sigc::signal<void,const Glib::RefPtr<Gtk::TextTag>&,bool> m_signal_changed;
};


class DynamicNoteTag
	: public NoteTag
{
public:
	typedef Glib::RefPtr<DynamicNoteTag> Ptr;
	typedef Glib::RefPtr<const DynamicNoteTag> ConstPtr;
	typedef std::map<std::string, std::string> AttributeMap;

	const AttributeMap & get_attributes() const
		{
			return m_attributes;
		}
	virtual void write(sharp::XmlWriter &, bool) const;
	virtual void read(xmlpp::TextReader &, bool);
	/// <summary>
	/// Derived classes should override this if they desire
	/// to be notified when a tag attribute is read in.
	/// </summary>
	/// <param name="attributeName">
	/// A <see cref="System.String"/> that is the name of the
	/// newly read attribute.
	/// </param>
	virtual void on_attribute_read(const std::string &) 
		{
		}

private:
	AttributeMap m_attributes;
};


class DepthNoteTag
	: public NoteTag
{
public:
	typedef Glib::RefPtr<DepthNoteTag> Ptr;

	DepthNoteTag(int depth, PangoDirection direction);

	int get_depth() const
		{ 
			return m_depth; 
		}
	PangoDirection get_direction() const
		{
			return PANGO_DIRECTION_LTR;
		}
	virtual void write(sharp::XmlWriter &, bool) const;
private:
	int            m_depth;
	PangoDirection m_direction;
};


class TagType 
{
public:
	typedef sigc::signal<DynamicNoteTag::Ptr, const std::string &> Factory;
	typedef sigc::slot<DynamicNoteTag::Ptr, const std::string &> FactorySlot;
	TagType(const FactorySlot & factory) 
		{
			m_factory.connect(factory);
		}
	TagType(const TagType & rhs) 
		{
			m_factory = rhs.m_factory;
		}
	TagType()
		{}
	DynamicNoteTag::Ptr create(const std::string & name)
		{
			return m_factory(name);
		}
private:
	Factory m_factory;
};


class NoteTagTable
	: public Gtk::TextTagTable
{
public:
	typedef Glib::RefPtr<NoteTagTable> Ptr;

	static NoteTagTable & instance() 
		{
			if(!s_instance) {
				s_instance = new NoteTagTable;
			}
			return *s_instance;
		}
	static bool tag_is_serializable(const Glib::RefPtr<const Gtk::TextTag> & );
	static bool tag_is_growable(const Glib::RefPtr<Gtk::TextTag> & );
	static bool tag_is_undoable(const Glib::RefPtr<Gtk::TextTag> & );
	static bool tag_is_spell_checkable(const Glib::RefPtr<const Gtk::TextTag> & );
	static bool tag_is_activatable(const Glib::RefPtr<Gtk::TextTag> & );
	static bool tag_has_depth(const Glib::RefPtr<Gtk::TextBuffer::Tag> & );
	DepthNoteTag::Ptr get_depth_tag(int depth, PangoDirection direction);
	DynamicNoteTag::Ptr create_dynamic_tag(const std::string & );
	void register_dynamic_tag (const std::string & tag_name, const TagType & type);
	bool is_dynamic_tag_registered(const std::string &);

protected:
	NoteTagTable()
		{
			_init_common_tags();
		}

	virtual void on_tag_added(const Glib::RefPtr<Gtk::TextTag> &);
	virtual void on_tag_removed(const Glib::RefPtr<Gtk::TextTag> &);
private:
	void _init_common_tags();

	static NoteTagTable                   *s_instance;
	std::map<std::string, TagType>         m_tag_types;
	std::list<Glib::RefPtr<Gtk::TextTag> > m_added_tags;
};


}

#endif

