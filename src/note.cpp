

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <tr1/functional>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/find.hpp>

#include <libxml++/parsers/textreader.h>
#include <libxml++/parsers/domparser.h>

#include <glibmm/i18n.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

#include "note.hpp"
#include "notemanager.hpp"
#include "notetag.hpp"
#include "notewindow.hpp"
#include "tagmanager.hpp"
#include "utils.hpp"
#include "debug.hpp"
#include "sharp/exception.hpp"
#include "sharp/string.hpp"
#include "sharp/xmlconvert.hpp"
#include "sharp/xmlwriter.hpp"
#include "sharp/foreach.hpp"


namespace gnote {

	namespace noteutils {

		void show_deletion_dialog (const std::list<Note::Ptr> & notes, Gtk::Window * parent)
		{
			std::string message;
			
			if (notes.size() == 1)
				message = _("Really delete this note?");
			else
				message = str(boost::format(_("Really delete these %1% notes?")) % notes.size());
			
			utils::HIGMessageDialog dialog(parent, GTK_DIALOG_DESTROY_WITH_PARENT,
			        Gtk::MESSAGE_QUESTION,
			        Gtk::BUTTONS_NONE,
			        message,
			        _("If you delete a note it is permanently lost."));

			Gtk::Button *button;

			button = manage(new Gtk::Button(Gtk::Stock::CANCEL));
			button->property_can_default().set_value(true);
			button->show ();
			dialog.add_action_widget(*button, Gtk::RESPONSE_CANCEL);
			dialog.set_default_response(Gtk::RESPONSE_CANCEL);

			button = manage(new Gtk::Button (Gtk::Stock::DELETE));
			button->property_can_default().set_value(true);
			button->show ();
			dialog.add_action_widget(*button, 666);

			int result = dialog.run();
			if (result == 666) {
				foreach(const Note::Ptr & note, notes) {
					note->manager().delete_note(note);
				}
			}
		}

	}

	namespace {
		
		void show_io_error_dialog (Gtk::Window * parent)
		{
			utils::HIGMessageDialog dialog(
				                      parent,
				                      GTK_DIALOG_DESTROY_WITH_PARENT,
				                      Gtk::MESSAGE_ERROR,
				                      Gtk::BUTTONS_OK,
				                      _("Error saving note data."),
				                      _("An error occurred while saving your notes. "
																"Please check that you have sufficient disk "
																"space, and that you have appropriate rights "
																"on ~/.gnote. Error details can be found in "
																"~/.gnote.log."));
			dialog.run();
		}

	}

	class NoteData
	{
	public:
		typedef std::map<std::string, Tag::Ptr> TagMap;
		NoteData(const std::string & _uri);

		const std::string & uri() const
			{
				return m_uri;
			}
		const std::string & title() const
			{
				return m_title;
			}
		std::string & title()
			{
				return m_title;
			}
		const std::string & text() const
			{ 
				return m_text;
			}
		std::string & text()
			{ 
				return m_text;
			}
		const sharp::DateTime & create_date() const
			{
				return m_create_date;
			}
		sharp::DateTime & create_date()
			{
				return m_create_date;
			}
		const sharp::DateTime & change_date() const
			{
				return m_change_date;
			}
		void set_change_date(const sharp::DateTime & date)
			{
				m_change_date = date;
				m_metadata_change_date = date;
			}
		const sharp::DateTime & metadata_change_date() const
			{
				return m_metadata_change_date;
			}
		sharp::DateTime & metadata_change_date()
			{
				return m_metadata_change_date;
			}
		int cursor_position() const
			{
				return m_cursor_pos;
			}
		void set_cursor_position(int new_pos)
			{
				m_cursor_pos = new_pos;
			}
		int width() const
			{
				return m_width;
			}
		int & width()
			{
				return m_width;
			}
		int height() const
			{
				return m_height;
			}
		int & height()
			{
				return m_height;
			}
		int x() const
			{
				return m_x;
			}
		int & x()
			{
				return m_x;
			}
		int y() const
			{
				return m_y;
			}
		int & y()
			{
				return m_y;
			}
		const TagMap & tags() const
			{
				return m_tags;
			}
		TagMap & tags()
			{
				return m_tags;
			}
		
		bool is_open_on_startup() const
			{
				return m_open_on_startup;
			}
		void set_is_open_on_startup(bool v)
			{
				m_open_on_startup = v;
			}
		void set_position_extent(int x, int y, int width, int height);
		bool has_position();
		bool has_extent();

	private:
		const std::string m_uri;
		std::string       m_title;
		std::string       m_text;
		sharp::DateTime             m_create_date;
		sharp::DateTime             m_change_date;
		sharp::DateTime             m_metadata_change_date;
		int               m_cursor_pos;
		int               m_width, m_height;
		int               m_x, m_y;
		bool              m_open_on_startup;

		TagMap m_tags;
	
		static const int  s_noPosition;
	};


	const int  NoteData::s_noPosition = -1;

	NoteData::NoteData(const std::string & _uri)
		: m_uri(_uri)
		, m_cursor_pos(0)
		, m_width(0)
		, m_height(0)
		, m_x(s_noPosition)
		, m_y(s_noPosition)
		, m_open_on_startup(false)
	{
	}


	void NoteData::set_position_extent(int _x, int _y, int _width, int _height)
	{
		if (_x < 0 || _y < 0)
			return;
		if (_width <= 0 || _height <= 0)
			return;

		m_x = _x;
		m_y = _y;
		m_width = _width;
		m_height = _height;
	}

	bool NoteData::has_position()
	{
		return (m_x != s_noPosition) && (m_y != s_noPosition);
	}

	bool NoteData::has_extent()
	{
		return (m_width != 0) && (m_height != 0);
	}

	NoteDataBufferSynchronizer::~NoteDataBufferSynchronizer()
	{
		delete m_data;
	}

	void NoteDataBufferSynchronizer::set_buffer(const Glib::RefPtr<NoteBuffer> & b)
	{
		m_buffer = b;
		m_buffer->signal_changed().connect(sigc::mem_fun(*this, &NoteDataBufferSynchronizer::buffer_changed));
		m_buffer->signal_apply_tag()
			.connect(sigc::mem_fun(*this, &NoteDataBufferSynchronizer::buffer_tag_applied));
		m_buffer->signal_remove_tag()
			.connect(sigc::mem_fun(*this, &NoteDataBufferSynchronizer::buffer_tag_removed));

		synchronize_buffer();

		invalidate_text();
	}

	const std::string & NoteDataBufferSynchronizer::text()
	{
		synchronize_text();
		return m_data->text();
	}

	void NoteDataBufferSynchronizer::set_text(const std::string & t)
	{
		m_data->text() = t;
		synchronize_buffer();
	}

	void NoteDataBufferSynchronizer::invalidate_text()
	{
		m_data->text() = "";
	}

	bool NoteDataBufferSynchronizer::is_text_invalid() const
	{
		return m_data->text().empty();
	}

	void NoteDataBufferSynchronizer::synchronize_text() const
	{
		if(is_text_invalid() && m_buffer) {
			m_data->text() = NoteBufferArchiver::serialize(m_buffer);
		}
	}

	void NoteDataBufferSynchronizer::synchronize_buffer()
	{
		if(!is_text_invalid() && m_buffer) {
			// Don't create Undo actions during load
			m_buffer->undoer().freeze_undo ();

			m_buffer->erase(m_buffer->begin(), m_buffer->end());

			// Load the stored xml text
			NoteBufferArchiver::deserialize (m_buffer,
																			 m_buffer->begin(),
																			 m_data->text());
			m_buffer->set_modified(false);

			Gtk::TextIter cursor;
			if (m_data->cursor_position() != 0) {
				// Move cursor to last-saved position
				cursor = m_buffer->get_iter_at_offset (m_data->cursor_position());
			} 
			else {
				// Avoid title line
				cursor = m_buffer->get_iter_at_line(2);
			}
			m_buffer->place_cursor(cursor);

			// New events should create Undo actions
			m_buffer->undoer().thaw_undo ();
		}
	}

	void NoteDataBufferSynchronizer::buffer_changed()
	{
		invalidate_text();
	}

	void NoteDataBufferSynchronizer::buffer_tag_applied(const Glib::RefPtr<Gtk::TextBuffer::Tag> & tag,
																											const Gtk::TextBuffer::iterator &,
																											const Gtk::TextBuffer::iterator &)
	{
		if(NoteTagTable::tag_is_serializable(tag)) {
			invalidate_text();
		}
	}

	void NoteDataBufferSynchronizer::buffer_tag_removed(const Glib::RefPtr<Gtk::TextBuffer::Tag> & tag,
																											const Gtk::TextBuffer::iterator &,
																											const Gtk::TextBuffer::iterator &)
	{
		if(NoteTagTable::tag_is_serializable(tag)) {
			invalidate_text();
		}
	}

	Note::Note(NoteData * _data, const std::string & filepath, NoteManager & _manager)
		: m_data(_data)
		, m_filepath(filepath)
		, m_save_needed(true)
		, m_is_deleting(false)
		, m_manager(_manager)
		, m_window(NULL)
		, m_tag_table(NULL)
	{
		foreach(const NoteData::TagMap::value_type & value, _data->tags()) {
			add_tag(value.second);
		}
		m_save_timeout = new utils::InterruptableTimeout();
		m_save_timeout->signal_timeout.connect(sigc::mem_fun(*this, &Note::on_save_timeout));
	}

	Note::~Note()
	{
    delete m_save_timeout;
    delete m_window;
	}

  /// <summary>
	/// Returns a Tomboy URL from the given path.
	/// </summary>
	/// <param name="filepath">
	/// A <see cref="System.String"/>
	/// </param>
	/// <returns>
	/// A <see cref="System.String"/>
	/// </returns>
	std::string Note::url_from_path(const std::string & filepath)
	{
		return "note://gnote/" + 	boost::filesystem::path(filepath).stem();
	}

	int Note::get_hash_code() const
	{
		std::tr1::hash<std::string> h;
		return h(get_title());
	}

  /// <summary>
	/// Creates a New Note with the given values.
	/// </summary>
	/// <param name="title">
	/// A <see cref="System.String"/>
	/// </param>
	/// <param name="filepath">
	/// A <see cref="System.String"/>
	/// </param>
	/// <param name="manager">
	/// A <see cref="NoteManager"/>
	/// </param>
	/// <returns>
	/// A <see cref="Note"/>
	/// </returns>
	Note::Ptr Note::create_new_note(const std::string & title,
																	const std::string & filename,
																	NoteManager & manager)
	{
		NoteData * note_data = new NoteData(url_from_path(filename));
		note_data->title() = title;
		sharp::DateTime date(sharp::DateTime::now());
		note_data->create_date() = date;
		note_data->set_change_date(date);
			
		return Note::Ptr(new Note(note_data, filename, manager));
	}

	Note::Ptr Note::create_existing_note(NoteData *data,
																 std::string filepath,
																 NoteManager & manager)
	{
		if (!data->create_date().is_valid()) {
			sharp::DateTime d(boost::filesystem::last_write_time(filepath));
			data->create_date() = d;
		}
		if (!data->change_date().is_valid()) {
			sharp::DateTime d(boost::filesystem::last_write_time(filepath));
			data->set_change_date(d);
		}
		return Note::Ptr(new Note(data, filepath, manager));
	}

	void Note::delete_note()
	{
		m_is_deleting = true;
		m_save_timeout->cancel ();
		
		// Remove the note from all the tags
		foreach(const NoteData::TagMap::value_type & value, m_data.data().tags()) {
			remove_tag(value.second);
		}

		if (m_window) {
			m_window->hide ();
      delete m_window; 
      m_window = NULL;
		}
			
		// Remove note URI from GConf entry menu_pinned_notes
		set_pinned(false);
	}

	
	Note::Ptr Note::load(const std::string & read_file, NoteManager & manager)
	{
		NoteData *data = NoteArchiver::read(read_file, url_from_path(read_file));
		return create_existing_note (data, read_file, manager);
	}

	
	void Note::save()
	{
		// Prevent any other condition forcing a save on the note
		// if Delete has been called.
		if (m_is_deleting)
			return;
			
		// Do nothing if we don't need to save.  Avoids unneccessary saves
		// e.g on forced quit when we call save for every note.
		if (!m_save_needed)
			return;

		DBG_OUT("Saving '%s'...", m_data.data().title().c_str());

		try {
			NoteArchiver::write(m_filepath, m_data.synchronized_data());
		} 
		catch (const sharp::Exception & e) {
			// Probably IOException or UnauthorizedAccessException?
			ERR_OUT("Exception while saving note: %s", e.what());
			show_io_error_dialog(m_window);
		}

		m_signal_saved(shared_from_this());
	}

	
	void Note::on_buffer_changed()
	{
		DBG_OUT("on_buffer_changed queuein save");
		queue_save(CONTENT_CHANGED);
	}

	void Note::on_buffer_tag_applied(const Glib::RefPtr<Gtk::TextTag> &tag,
																	 const Gtk::TextBuffer::iterator &, 
																	 const Gtk::TextBuffer::iterator &)
	{
		if(NoteTagTable::tag_is_serializable(tag)) {
			DBG_OUT("BufferTagApplied queueing save: %s", tag->property_name().get_value().c_str());
			queue_save(CONTENT_CHANGED);
		}
	}

	void Note::on_buffer_tag_removed(const Glib::RefPtr<Gtk::TextTag> &tag,
																	 const Gtk::TextBuffer::iterator &,
																	 const Gtk::TextBuffer::iterator &)
	{
		if(NoteTagTable::tag_is_serializable(tag)) {
			DBG_OUT("BufferTagRemoved queueing save: %s", tag->property_name().get_value().c_str());
			queue_save(CONTENT_CHANGED);
		}
	}

	void Note::on_buffer_insert_mark_set(const Gtk::TextBuffer::iterator & iter,
																			 const Glib::RefPtr<Gtk::TextBuffer::Mark> & insert)
	{
		if(insert != m_buffer->get_insert()) {
			return;
		}
		m_data.data().set_cursor_position(iter.get_offset());
		DBG_OUT("BufferInsertSetMark queueing save");
		queue_save(NO_CHANGE);
	}


	bool Note::on_window_configure(GdkEventConfigure * /*ev*/)
	{
		int cur_x, cur_y, cur_width, cur_height;

		// Ignore events when maximized.  We don't want notes
		// popping up maximized the next run.
		if ((m_window->get_window()->get_state() & Gdk::WINDOW_STATE_MAXIMIZED) != 0)
			return true;

		m_window->get_position(cur_x, cur_y);
		m_window->get_size(cur_width, cur_height);

		if (m_data.data().x() == cur_x &&
				m_data.data().y() == cur_y &&
				m_data.data().width() == cur_width &&
				m_data.data().height() == cur_height)
			return true;

		m_data.data().set_position_extent(cur_x, cur_y, cur_width, cur_height);

		DBG_OUT("WindowConfigureEvent queueing save");
		queue_save(NO_CHANGE);
		return true;
	}

	bool Note::on_window_destroyed(GdkEventAny * /*ev*/)
	{
		m_window = NULL;
		return true;
	}

	void Note::queue_save (ChangeType changeType)
	{
		DBG_OUT("Got QueueSave");

		// Replace the existing save timeout.  Wait 4 seconds
		// before saving...
		m_save_timeout->reset(4000);
		if (!m_is_deleting)
			m_save_needed = true;
			
		switch (changeType)
		{
		case CONTENT_CHANGED:
			// NOTE: Updating ChangeDate automatically updates MetdataChangeDate to match.
			m_data.data().set_change_date(sharp::DateTime::now());
			break;
		case OTHER_DATA_CHANGED:
			// Only update MetadataChangeDate.  Used by sync/etc
			// to know when non-content note data has changed,
			// but order of notes in menu and search UI is
			// unaffected.
			m_data.data().metadata_change_date() = sharp::DateTime::now();
			break;
		default:
			break;
		}
	}

	void Note::on_save_timeout()
	{
		try {
			save();
			m_save_needed = false;
		}
		catch(const sharp::Exception &e) 
		{
			ERR_OUT("Error while saving: %s", e.what());
		}
	}

	void Note::add_tag(const Tag::Ptr & tag)
	{
		if(!tag) {
			throw sharp::Exception ("Note.AddTag () called with a null tag.");
		}
		tag->add_note (*this);

		NoteData::TagMap & thetags(m_data.data().tags());
		if (thetags.find(tag->normalized_name()) == thetags.end()) {
			thetags[tag->normalized_name()] = tag;

			m_signal_tag_added(shared_from_this(), tag);

			DBG_OUT ("Tag added, queueing save");
			queue_save(OTHER_DATA_CHANGED);
		}
	}

	void Note::remove_tag(Tag & tag)
	{
		NoteData::TagMap & thetags(m_data.data().tags());
		NoteData::TagMap::iterator iter = thetags.find(tag.normalized_name());
		if (iter == thetags.end())
			return;

		m_signal_tag_removing(*this, tag);

		thetags.erase(iter);
		tag.remove_note(*this);

		m_signal_tag_removed(shared_from_this(), tag.normalized_name());

		DBG_OUT("Tag removed, queueing save");
		queue_save(OTHER_DATA_CHANGED);
	}


	void Note::remove_tag(const Tag::Ptr & tag)
	{
		if (!tag)
			throw sharp::Exception ("Note.RemoveTag () called with a null tag.");
		remove_tag(*tag);
	}
		
	bool Note::contains_tag(const Tag::Ptr & tag) const
	{
		const NoteData::TagMap & thetags(m_data.data().tags());
		return (thetags.find(tag->normalized_name()) != thetags.end());
	}

	void Note::add_child_widget(const Glib::RefPtr<Gtk::TextChildAnchor> & child_anchor,
															Gtk::Widget * widget)
	{
		m_child_widget_queue.push(ChildWidgetData(child_anchor, widget));
		if(has_window()) {
			process_child_widget_queue();
		}
	}

	void Note::process_child_widget_queue()
	{
		// Insert widgets in the childWidgetQueue into the NoteEditor
		if (!has_window())
			return; // can't do anything without a window

		while(!m_child_widget_queue.empty()) {
			ChildWidgetData & qdata(m_child_widget_queue.front());
			qdata.widget->show();
			m_window->editor()->add_child_at_anchor(*qdata.widget, qdata.anchor);
			m_child_widget_queue.pop();
		}
	}

	const std::string & Note::uri() const
	{
		return m_data.data().uri();
	}

	const std::string Note::id() const
	{
		// TODO: Store on Note instantiation
		return sharp::string_replace_first(m_data.data().uri(), "note://gnote/","");
	}


	const std::string & Note::get_title() const
	{
		return m_data.data().title();
	}


	void Note::set_title(const std::string & new_title)
	{
		if (m_data.data().title() != new_title) {
			if (m_window) {
				m_window->set_title(new_title);
			}

			std::string old_title = m_data.data().title();
			m_data.data().title() = new_title;

			m_signal_renamed(shared_from_this(), old_title);

			queue_save (CONTENT_CHANGED); // TODO: Right place for this?
		}
	}


	void Note::rename_without_link_update(const std::string & newTitle)
	{
		if (m_data.data().title() != newTitle) {
			if (m_window) {
				m_window->set_title(newTitle);
			}

			m_data.data().title() = newTitle;

			// HACK:
			m_signal_renamed(shared_from_this(), newTitle);

			queue_save(CONTENT_CHANGED); // TODO: Right place for this?
		}
	}

	void Note::set_xml_content(const std::string & xml)
	{
		if (m_buffer) {
			m_buffer->set_text("");
			NoteBufferArchiver::deserialize(m_buffer, xml);
		} 
		else {
			m_data.set_text(xml);
		}
	}

	std::string Note::get_complete_note_xml()
	{
		return NoteArchiver::write_string(m_data.synchronized_data());
	}

	void Note::load_foreign_note_xml(const std::string & foreignNoteXml, ChangeType changeType)
	{
		if (foreignNoteXml.empty())
			throw sharp::Exception ("foreignNoteXml");

		// Arguments to this method cannot be trusted.  If this method
		// were to throw an XmlException in the middle of processing,
		// a note could be damaged.  Therefore, we check for parseability
		// ahead of time, and throw early.
		{
			xmlpp::DomParser parser;
			parser.parse_memory(foreignNoteXml);
			if(!parser) {
				throw sharp::Exception("invalid XML in foreignNoteXml");
			}
		}

		xmlpp::TextReader xml((const unsigned char*)foreignNoteXml.c_str(), 
													foreignNoteXml.size());

		// Remove tags now, since a note with no tags has
		// no "tags" element in the XML
		foreach (const Tag::Ptr & tag, tags()) {
			remove_tag(tag);
		}
		Glib::ustring name;

		while (xml.read()) {
			switch (xml.get_node_type()) {
			case xmlpp::TextReader::Element:
				name = xml.get_name();
				if (name == "title") {
					set_title(xml.read_string());
				}
				else if (name == "text") {
					set_xml_content(xml.read_inner_xml());
				}
				else if (name == "last-change-date") {
					m_data.data().set_change_date(
						sharp::XmlConvert::to_date_time(xml.read_string(), NoteArchiver::DATE_TIME_FORMAT));
				}
				else if(name == "last-metadata-change-date") {
					m_data.data().metadata_change_date() =
						sharp::XmlConvert::to_date_time(xml.read_string(), NoteArchiver::DATE_TIME_FORMAT);
				}
				else if(name == "create-date") {
					m_data.data().create_date() =
						sharp::XmlConvert::to_date_time(xml.read_string (), NoteArchiver::DATE_TIME_FORMAT);
				}
				else if(name == "tags") {
					xmlpp::DomParser parser;
					parser.parse_memory(xml.read_outer_xml());
					if(parser) {
						const xmlpp::Document * doc2 = parser.get_document();
						std::list<std::string> tag_strings = parse_tags (doc2->get_root_node());
						foreach (const std::string & tag_str, tag_strings) {
							DBG_OUT("TODO: create tag %s", tag_str.c_str());
							Tag::Ptr tag = TagManager::instance().get_or_create_tag(tag_str);
							add_tag(tag);
						}
					}
					else {
						DBG_OUT("loading tag subtree failed");
					}
				}
				else if(name == "open-on-startup") {
					// TODO this is a hacked on observation about how Mono convert
					// a bool from a string.
					set_is_open_on_startup(xml.read_string() == "True");
				}
				break;
			default:
				break;
			}
		}

		xml.close ();

		// Allow method caller to specify ChangeType (mostly needed by sync)
		queue_save (changeType);
	}


	std::list<std::string> Note::parse_tags(const xmlpp::Node *tagnodes)
	{
		std::list<std::string> tags;
		xmlpp::NodeSet nodes = tagnodes->find("//tag");
		foreach(const xmlpp::Node * node, nodes) {
			
			const xmlpp::ContentNode * content = dynamic_cast<const xmlpp::ContentNode *>(node);
			if(content) {
				std::string tag = content->get_content();
				tags.push_back(tag);
			}
		}
		return tags;
	}

	std::string Note::text_content()
	{
		if(m_buffer) {
			return m_buffer->get_slice(m_buffer->begin(), m_buffer->end());
		}
		return utils::XmlDecoder::decode(xml_content());
	}

	void Note::set_text_content(const std::string & text)
	{
		if(m_buffer) {
			m_buffer->set_text(text);
		}
		else {
			ERR_OUT("Setting text content for closed notes not supported");
		}
	}

	const NoteData & Note::data() const
	{
		return m_data.synchronized_data();
	}

	NoteData & Note::data()
	{
		return m_data.synchronized_data();
	}

	const sharp::DateTime & Note::create_date() const
	{
		return m_data.data().create_date();
	}

	const sharp::DateTime & Note::change_date() const
	{
		return m_data.data().change_date();
	}

	const sharp::DateTime & Note::metadata_change_date() const
	{
		return m_data.data().metadata_change_date();
	}

	const Glib::RefPtr<NoteTagTable> & Note::get_tag_table()
	{
		if (m_tag_table == NULL) {
#if FIXED_GTKSPELL
			// NOTE: Sharing the same TagTable means
			// that formatting is duplicated between
			// buffers.
			m_tag_table = NoteTagTable::Ptr(&NoteTagTable::instance());
#else
			// NOTE: GtkSpell chokes on shared
			// TagTables because it blindly tries to
			// create a new "gtkspell-misspelling"
			// tag, which fails if one already
			// exists in the table.
			m_tag_table = NoteTagTable::Ptr(new NoteTagTable());
#endif
		}
		return m_tag_table;
	}

	const Glib::RefPtr<NoteBuffer> & Note::get_buffer()
	{
		if(!m_buffer) {
			DBG_OUT("Creating buffer for %s", m_data.data().title().c_str());
			m_buffer = Glib::RefPtr<NoteBuffer>(new NoteBuffer(get_tag_table(), *this));
			m_data.set_buffer(m_buffer);

			m_buffer->signal_changed().connect(
				sigc::mem_fun(*this, &Note::on_buffer_changed));
			m_buffer->signal_apply_tag().connect(
				sigc::mem_fun(*this, &Note::on_buffer_tag_applied));
			m_buffer->signal_remove_tag().connect(
				sigc::mem_fun(*this, &Note::on_buffer_tag_removed));
			m_buffer->signal_mark_set().connect(
				sigc::mem_fun(*this, &Note::on_buffer_insert_mark_set));
		}
		return m_buffer;
	}


	NoteWindow * Note::get_window()
	{
		if(!m_window) {
			m_window = new NoteWindow(*this);
			m_window->signal_delete_event().connect(sigc::mem_fun(*this, &Note::on_window_destroyed));
			m_window->signal_configure_event().connect(sigc::mem_fun(*this, &Note::on_window_configure));

			if (m_data.data().has_extent())
				m_window->set_default_size(m_data.data().width(),
																	 m_data.data().height());

			if (m_data.data().has_position())
				m_window->move(m_data.data().x(), m_data.data().y());

			// This is here because emiting inside
			// OnRealized causes segfaults.
			m_signal_opened(*this);

			// Add any child widgets if any exist now that
			// the window is showing.
			process_child_widget_queue();
		}
		return m_window;
	}


	bool Note::is_special() const
	{ 
		return (m_manager.start_note_uri() == m_data.data().uri());
	}


	bool Note::is_new() const
	{
		return m_data.data().create_date() > sharp::DateTime::now().add_hours(-24);
	}

	bool Note::is_pinned() const
	{
		std::string pinned_uris = Preferences::get_preferences()
			->get<std::string>(Preferences::MENU_PINNED_NOTES);
		return (boost::find_first(pinned_uris, uri()));
	}


	void Note::set_pinned(bool pinned) const
	{
		std::string new_pinned;
		// this is like a calle to is_pinned() but we want to reating the gconf value
		std::string old_pinned = Preferences::get_preferences()
			->get<std::string>(Preferences::MENU_PINNED_NOTES);
		bool is_currently_pinned = (boost::find_first(old_pinned, uri()));

		if (pinned == is_currently_pinned)
			return;

		if (is_currently_pinned) {
			new_pinned = uri() + " " + old_pinned;
		} 
		else {
			std::vector<std::string> pinned_split;
			sharp::string_split(pinned_split, old_pinned, " \t\n");
			foreach(const std::string & pin, pinned_split) {
				if (!pin.empty() && (pin != uri())) {
					new_pinned += pin + " ";
				}
			}
		}
		Preferences::get_preferences()->set<std::string>(Preferences::MENU_PINNED_NOTES, new_pinned);
	}

	
	bool Note::is_open_on_startup()
	{
		return data().is_open_on_startup();
	}


	void Note::set_is_open_on_startup(bool value)
	{
		if (data().is_open_on_startup() != value) {
			data().set_is_open_on_startup(value);
			m_save_needed = true;
		}
	}
	
	std::list<Tag::Ptr> Note::tags()
	{
		std::list<Tag::Ptr> l;
		foreach(const NoteData::TagMap::value_type & val, m_data.data().tags()) {
			l.push_back(val.second);
		}
		return l;
	}

	const char *NoteArchiver::CURRENT_VERSION = "0.3";
	const char *NoteArchiver::DATE_TIME_FORMAT = "yyyy-MM-ddTHH:mm:ss.fffffffzzz";
	NoteArchiver *NoteArchiver::s_instance = NULL;

	NoteArchiver & NoteArchiver::instance()
	{
		if(!s_instance) {
			s_instance = new NoteArchiver();
		}
		return *s_instance;
	}

	NoteData *NoteArchiver::read(const std::string & read_file, const std::string & uri)
	{
		return instance()._read(read_file, uri);
	}


	NoteData *NoteArchiver::_read(const std::string & read_file, const std::string & uri)
	{
		NoteData *note = new NoteData(uri);
		std::string version;

		xmlpp::TextReader xml(read_file);

		Glib::ustring name;

		while (xml.read ()) {
			switch (xml.get_node_type()) {
			case xmlpp::TextReader::Element:
				name = xml.get_name();
				
				if(name == "note") {
					version = xml.get_attribute("version");
				}
				else if(name == "title") {
					note->title() = xml.read_string();
				} 
				else if(name == "text") {
					// <text> is just a wrapper around <note-content>
					// NOTE: Use .text here to avoid triggering a save.
					note->text() = xml.read_inner_xml();
				}
				else if(name == "last-change-date") {
					note->set_change_date(
						sharp::XmlConvert::to_date_time (xml.read_string(), DATE_TIME_FORMAT));
				}
				else if(name == "last-metadata-change-date") {
					note->metadata_change_date() =
						sharp::XmlConvert::to_date_time(xml.read_string(), DATE_TIME_FORMAT);
				}
				else if(name == "create-date") {
					note->create_date() =
						sharp::XmlConvert::to_date_time (xml.read_string(), DATE_TIME_FORMAT);
				}
				else if(name == "cursor-position") {
					note->set_cursor_position(boost::lexical_cast<int>(xml.read_string()));
				}
				else if(name == "width") {
					note->width() = boost::lexical_cast<int>(xml.read_string());
				}
				else if(name == "height") {
					note->height() = boost::lexical_cast<int>(xml.read_string());
				}
				else if(name == "x") {
					note->x() = boost::lexical_cast<int>(xml.read_string());
				}
				else if(name == "y") {
					note->y() = boost::lexical_cast<int>(xml.read_string());
				}
				else if(name == "tags") {
					xmlpp::DomParser parser;
					parser.parse_memory(xml.read_outer_xml());
					if(parser) {
						const xmlpp::Document * doc2 = parser.get_document();
						std::list<std::string> tag_strings = Note::parse_tags(doc2->get_root_node());
						foreach (const std::string & tag_str, tag_strings) {
							Tag::Ptr tag = TagManager::instance().get_or_create_tag(tag_str);
							note->tags()[tag->normalized_name()] = tag;
						}
					}
					else {
						DBG_OUT("loading tag subtree failed");
					}
				}
				else if(name == "open-on-startup") {
					note->set_is_open_on_startup(xml.read_string() == "True");
				}
				break;

			default:
				break;
			}
		}
		xml.close ();

		if (version != NoteArchiver::CURRENT_VERSION) {
			// Note has old format, so rewrite it.  No need
			// to reread, since we are not adding anything.
			DBG_OUT("Updating note XML to newest format...");
			NoteArchiver::write (read_file, *note);
		}
		return note;
	}

	std::string NoteArchiver::write_string(const NoteData & note)
	{
		std::string str;
		sharp::XmlWriter xml;
		instance().write(xml, note);
		xml.close();
		str = xml.to_string();
		return str;
	}
	

	void NoteArchiver::write(const std::string & write_file, const NoteData & data)
	{
		instance().write_file(write_file, data);
	}

	void NoteArchiver::write_file(const std::string & _write_file, const NoteData & note)
	{
		std::string tmp_file = _write_file + ".tmp";
    // TODO Xml doc settings
		sharp::XmlWriter xml(tmp_file); //, XmlEncoder::DocumentSettings);
		write(xml, note);
		xml.close ();

		if (boost::filesystem::exists(_write_file)) {
			std::string backup_path = _write_file + "~";
			if (boost::filesystem::exists(backup_path)) {
				boost::filesystem::remove(backup_path);
			}
			
			// Backup the to a ~ file, just in case
			boost::filesystem::rename(_write_file, backup_path);
			
			// Move the temp file to write_file
			boost::filesystem::rename(tmp_file, _write_file);

			// Delete the ~ file
			boost::filesystem::remove(backup_path);
		} 
		else {
			// Move the temp file to write_file
			boost::filesystem::rename(tmp_file, _write_file);
		}
	}

	void NoteArchiver::write(sharp::XmlWriter & xml, const NoteData & note)
	{
		xml.write_start_document();
		xml.write_start_element("", "note", "http://beatniksoftware.com/tomboy");
		xml.write_attribute_string("",
														 "version",
														 "",
														 CURRENT_VERSION);
		xml.write_attribute_string("xmlns",
														 "link",
														 "",
														 "http://beatniksoftware.com/tomboy/link");
		xml.write_attribute_string("xmlns",
														 "size",
														 "",
														 "http://beatniksoftware.com/tomboy/size");

		xml.write_start_element ("", "title", "");
		xml.write_string (note.title());
		xml.write_end_element ();

		xml.write_start_element ("", "text", "");
		xml.write_attribute_string ("xml", "space", "", "preserve");
		// Insert <note-content> blob...
		xml.write_raw (note.text());
		xml.write_end_element ();

		xml.write_start_element ("", "last-change-date", "");
		xml.write_string (
			sharp::XmlConvert::to_string (note.change_date(), DATE_TIME_FORMAT));
		xml.write_end_element ();

		xml.write_start_element ("", "last-metadata-change-date", "");
		xml.write_string (
			sharp::XmlConvert::to_string (note.metadata_change_date(), DATE_TIME_FORMAT));
		xml.write_end_element ();

		if (note.create_date().is_valid()) {
			xml.write_start_element ("", "create-date", "");
			xml.write_string (
				sharp::XmlConvert::to_string (note.create_date(), DATE_TIME_FORMAT));
			xml.write_end_element ();
		}

		xml.write_start_element ("", "cursor-position", "");
		xml.write_string (boost::lexical_cast<std::string>(note.cursor_position()));
		xml.write_end_element ();

		xml.write_start_element ("", "width", "");
		xml.write_string (boost::lexical_cast<std::string>(note.width()));
		xml.write_end_element ();

		xml.write_start_element("", "height", "");
		xml.write_string(boost::lexical_cast<std::string>(note.height()));
		xml.write_end_element();

		xml.write_start_element("", "x", "");
		xml.write_string (boost::lexical_cast<std::string>(note.x()));
		xml.write_end_element();

		xml.write_start_element ("", "y", "");
		xml.write_string (boost::lexical_cast<std::string>(note.y()));
		xml.write_end_element();

		if (note.tags().size() > 0) {
			xml.write_start_element ("", "tags", "");
			foreach (const NoteData::TagMap::value_type & val,  note.tags()) {
				xml.write_start_element("", "tag", "");
				xml.write_string(val.second->name());
				xml.write_end_element();
			}
			xml.write_end_element();
		}

		xml.write_start_element("", "open-on-startup", "");
		xml.write_string(boost::lexical_cast<std::string>(note.is_open_on_startup()));
		xml.write_end_element();

		xml.write_end_element(); // Note
		xml.write_end_document();

	}
	
	std::string NoteArchiver::get_renamed_note_xml(const std::string & note_xml, 
																								 const std::string & old_title,
																								 const std::string & new_title) const
	{
		std::string updated_xml;
		// Replace occurences of oldTitle with newTitle in noteXml
		std::string titleTagPattern =	
			str(boost::format("<title>%1%</title>") % old_title);
		std::string titleTagReplacement =
			str(boost::format("<title>%1%</title>") % new_title);
		updated_xml = sharp::string_replace_regex(note_xml, titleTagPattern, titleTagReplacement);

		std::string titleContentPattern =
			str(boost::format("<note-content([^>]*)>\\s*%1%") % old_title);
		std::string titleContentReplacement =
			str(boost::format("<note-content$1>%1%") % new_title);
		std::string updated_xml2 = sharp::string_replace_regex(updated_xml, titleContentPattern, 
																													 titleContentReplacement);

		return updated_xml2;

	}

	std::string NoteArchiver::get_title_from_note_xml(const std::string & noteXml) const
	{
		if (!noteXml.empty()) {
			xmlpp::TextReader xml((const unsigned char*)noteXml.c_str(), noteXml.size());

			while (xml.read ()) {
				switch (xml.get_node_type()) {
				case xmlpp::TextReader::Element:
					if (xml.get_name() == "title") {
						return xml.read_string ();
					}
					break;
				default:
					break;
				}
			}
		}

		return "";
	}
	

}
