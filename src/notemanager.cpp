
#include <exception>

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <glib.h>
#include <glibmm/i18n.h>

#include "debug.hpp"
#include "notemanager.hpp"
#include "addinmanager.hpp"
#include "gnote.hpp"
#include "trie.hpp"
#include "sharp/exception.hpp"
#include "sharp/directory.hpp"
#include "sharp/foreach.hpp"
#include "sharp/uuid.hpp"

namespace gnote {


	bool compare_dates(const Note::Ptr & a, const Note::Ptr & b)
	{
		GDate * da = a->change_date();
		GDate * db = b->change_date();
		if(!da) {
			return false;
		}
		if(!db) {
			return true;
		}
		return (g_date_compare(da, db) < 0);
	}

	NoteManager::NoteManager(const std::string & directory)
	{
		std::string backup = directory + "/Backup";
		
		_common_init(directory, backup);
	}


	NoteManager::NoteManager(const std::string & directory, const std::string & backup)
	{
		_common_init(directory, backup);
	}


	void NoteManager::_common_init(const std::string & directory, const std::string & backup_directory)
	{
		m_addin_mgr = NULL;
		m_trie_controller = NULL;

		Preferences * prefs = Preferences::get_preferences();
		// Watch the START_NOTE_URI setting and update it so that the
		// StartNoteUri property doesn't generate a call to
		// Preferences.Get () each time it's accessed.
		m_start_note_uri = prefs->get<std::string>(Preferences::START_NOTE_URI);
		prefs->signal_setting_changed().connect(
			sigc::mem_fun(*this, &NoteManager::on_setting_changed));
		m_note_template_title = _("New Note Template");


		DBG_OUT("NoteManager created with note path \"%s\".", directory.c_str());

		m_notes_dir = directory;
		m_backup_dir = backup_directory;

		bool is_first_run = first_run ();
		create_notes_dir ();

		m_trie_controller = create_trie_controller ();
		m_addin_mgr = create_addin_manager ();

		if (is_first_run) {
			// First run. Create "Start Here" notes.
			create_start_notes ();
		} else {
			load_notes ();
		}

		// TODO
		//Tomboy.ExitingEvent += on_exiting_event;
	}

	NoteManager::~NoteManager()
	{
		delete m_trie_controller;
		delete m_addin_mgr;
	}

	void NoteManager::on_setting_changed(Preferences* , GConfEntry* entry)
	{
		const char * key = gconf_entry_get_key(entry);
		if(strcmp(key, Preferences::START_NOTE_URI) == 0) {
			GConfValue *v = gconf_entry_get_value(entry);
			if(v) {
				const char * s = gconf_value_get_string(v);
				m_start_note_uri = (s ? s : "");
			}
		}
	}


	// Create the TrieController. For overriding in test methods.
	TrieController *NoteManager::create_trie_controller ()
	{
		return new TrieController(*this);
	}

	AddinManager *NoteManager::create_addin_manager()
	{
		std::string gnote_conf_dir = Gnote::conf_dir();
		
		return new AddinManager (gnote_conf_dir);
	}

	// For overriding in test methods.
	bool NoteManager::directory_exists (const std::string & directory)
	{
		boost::filesystem::path p(directory);
		return exists(p) && is_directory(p);
	}

	// For overriding in test methods.
	bool NoteManager::create_directory (const std::string & directory)
	{
		boost::filesystem::path p(directory);
		return boost::filesystem::create_directory(p);
	}

	bool NoteManager::first_run ()
	{
		return !directory_exists (m_notes_dir);
	}

	// Create the notes directory if it doesn't exist yet.
	void NoteManager::create_notes_dir ()
	{
		if (!directory_exists (m_notes_dir)) {
			// First run. Create storage directory.
			create_directory(m_notes_dir);
		}
	}
	

	void NoteManager::on_note_rename (const Note::Ptr & note, const std::string & old_title)
	{
//		if (NoteRenamed != null)
		signal_note_renamed(note, old_title);
		m_notes.sort(boost::bind(&compare_dates, _1, _2));
	}

	void NoteManager::on_note_save (const Note::Ptr & note)
	{
//		if (NoteSaved != null)
		signal_note_saved(note);
		m_notes.sort(boost::bind(&compare_dates, _1, _2));
	}

	void NoteManager::create_start_notes ()
	{
		// FIXME: Delay the creation of the start notes so the panel/tray
		// icon has enough time to appear so that Tomboy.TrayIconShowing
		// is valid.  Then, we'll be able to instruct the user where to
		// find the Tomboy icon.
		//string icon_str = Tomboy.TrayIconShowing ?
		//     Catalog.GetString ("System Tray Icon area") :
		//     Catalog.GetString ("GNOME Panel");
		std::string start_note_content =
			_("<note-content>"
				"Start Here\n\n"
				"<bold>Welcome to Tomboy!</bold>\n\n"
				"Use this \"Start Here\" note to begin organizing "
				"your ideas and thoughts.\n\n"
				"You can create new notes to hold your ideas by "
				"selecting the \"Create New Note\" item from the "
				"Tomboy Notes menu in your GNOME Panel. "
				"Your note will be saved automatically.\n\n"
				"Then organize the notes you create by linking "
				"related notes and ideas together!\n\n"
				"We've created a note called "
				"<link:internal>Using Links in Tomboy</link:internal>.  "
				"Notice how each time we type <link:internal>Using "
				"Links in Tomboy</link:internal> it automatically "
				"gets underlined?  Click on the link to open the note."
				"</note-content>");

		std::string links_note_content =
			_("<note-content>"
				"Using Links in Tomboy\n\n"
				"Notes in Tomboy can be linked together by "
				"highlighting text in the current note and clicking"
				" the <bold>Link</bold> button above in the toolbar.  "
				"Doing so will create a new note and also underline "
				"the note's title in the current note.\n\n"
				"Changing the title of a note will update links "
				"present in other notes.  This prevents broken links "
				"from occurring when a note is renamed.\n\n"
				"Also, if you type the name of another note in your "
				"current note, it will automatically be linked for you."
				"</note-content>");

		try {
			Note::Ptr start_note = create (_("Start Here"),
																start_note_content);
			start_note->queue_save (Note::CONTENT_CHANGED);
			Preferences::get_preferences()->set<std::string>(Preferences::START_NOTE_URI, start_note->uri());

			Note::Ptr links_note = create (_("Using Links in Tomboy"),
																links_note_content);
			links_note->queue_save (Note::CONTENT_CHANGED);

// TODO
//			start_note->window()->Show ();
		} catch (const std::exception & e) {
			ERR_OUT("Error creating start notes: %s",
							e.what());
		}
	}
	
	void NoteManager::load_notes()
	{
		std::list<std::string> files = sharp::directory_get_files(m_notes_dir, "*.note");

		foreach(std::string file_path, files) {
			try {
				Note::Ptr note = Note::load(file_path, *this);
				if (note) {
					note->signal_renamed().connect(sigc::mem_fun(*this, &NoteManager::on_note_rename));
					note->signal_saved().connect(sigc::mem_fun(*this, &NoteManager::on_note_save));
					m_notes.push_back(note);
				}
			} 
			catch (...) {
//				Logger.Log ("Error parsing note XML, skipping \"{0}\": {1}",
//										file_path,
//										e.Message);
			}
		}
			
		m_notes.sort (boost::bind(&compare_dates, _1, _2));

		// Update the trie so addins can access it, if they want.
		m_trie_controller->update ();

		bool startup_notes_enabled = Preferences::get_preferences()->get<bool>(Preferences::ENABLE_STARTUP_NOTES);

		// Load all the addins for our notes.
		// Iterating through copy of notes list, because list may be
		// changed when loading addins.
		Note::List notesCopy(m_notes);
		foreach(Note::Ptr note, notesCopy) {

			m_addin_mgr->LoadAddinsForNote (note);

				// Show all notes that were visible when tomboy was shut down
			if (note->is_open_on_startup()) {
				if (startup_notes_enabled) {
// TODO
//						note.Window.Show ();
				}
				
				note->set_is_open_on_startup(false);
				note->queue_save (Note::NO_CHANGE);
			}
		}

		// Make sure that a Start Note Uri is set in the preferences, and
		// make sure that the Uri is valid to prevent bug #508982. This
		// has to be done here for long-time Tomboy users who won't go
		// through the create_start_notes () process.
		if (start_note_uri().empty() ||
				!find_by_uri(start_note_uri())) {
			// Attempt to find an existing Start Here note
			Note::Ptr start_note = find (_("Start Here"));
			if (start_note) {
				Preferences::get_preferences()->set<std::string>(Preferences::START_NOTE_URI, start_note->uri());
			}
		}
		
	}

	void NoteManager::on_exiting_event()
	{
		// Call ApplicationAddin.Shutdown () on all the known ApplicationAddins
		// TODO
#if 0
		foreach (ApplicationAddin addin in addin_mgr.GetApplicationAddins ()) {
			try {
				addin.Shutdown ();
			} catch (Exception e) {
				Logger.Warn ("Error calling {0}.Shutdown (): {1}",
										 addin.GetType ().ToString (), e.Message);
			}
		}
#endif

		DBG_OUT("Saving unsaved notes...");
			
		// Use a copy of the notes to prevent bug #510442 (crash on exit
		// when iterating the notes to save them.
		Note::List notesCopy(m_notes);
		foreach (Note::Ptr note, notesCopy) {
			// If the note is visible, it will be shown automatically on
			// next startup
// TODO
//				if (note.HasWindow && note.Window.Visible)
//					note->set_is_open_on_startup(true);

			note->save();
		}
	}

	void NoteManager::delete_note(const Note::Ptr & note)
	{
		if (boost::filesystem::exists(note->file_path())) {
			if (!m_backup_dir.empty()) {
				if (!boost::filesystem::exists(m_backup_dir)) {
					boost::filesystem::create_directory(m_backup_dir);
				}
				std::string backup_path 
					= m_backup_dir + "/" + boost::filesystem::path(note->file_path()).filename();
					
				if (boost::filesystem::exists(backup_path))
					boost::filesystem::remove(backup_path);

				boost::filesystem::rename(note->file_path(), backup_path);
			} 
			else {
				boost::filesystem::remove(note->file_path());
			}
		}

		m_notes.remove(note);
		note->Delete();

		DBG_OUT("Deleting note '%s'.", note->title().c_str());

//		if (NoteDeleted != null)
		signal_note_deleted(note);
	}

	std::string NoteManager::make_new_file_name()
	{
		return make_new_file_name (sharp::uuid().string());
	}

	std::string NoteManager::make_new_file_name(const std::string & guid)
	{
		return m_notes_dir + "/" + guid + ".note";
	}

	Note::Ptr NoteManager::create()
	{
		int new_num = m_notes.size();
		std::string temp_title;
		
		while (true) {
			++new_num;
			temp_title = str(boost::format(_("New Note %1%")) %	new_num);
			if (!find(temp_title)) {
				break;
			}
		}

		return create(temp_title);
	}

	std::string NoteManager::split_title_from_content (std::string title, std::string & body)
	{
		body = "";

		if (title.empty())
			return "";

		boost::trim(title);
		if (title.empty())
			return "";

		std::vector<std::string> lines;
		boost::split(lines, title, boost::is_any_of("\n\r"));
		if (lines.size() > 0) {
			title = lines [0];
			boost::trim(title);
			boost::trim_if(title, boost::is_any_of(".,;"));
			if (title.empty())
				return "";
		}

		if (lines.size() > 1)
			body = lines [1];

		return title;
	}


	Note::Ptr NoteManager::create (const std::string & title)
	{
		return create_new_note (title, "");
	}


	Note::Ptr NoteManager::create(const std::string & title, const std::string & xml_content)
	{
		return create_new_note (title, xml_content, "");
	}

	Note::Ptr NoteManager::create_with_guid (const std::string & title, std::string & guid)
	{
		return create_new_note (title, guid);
	}

	// Create a new note with the specified title, and a simple
	// "Describe..." body or the body from the "New Note Template"
	// note if it exists.  If the "New Note Template" body is found
	// the text will not automatically be highlighted.
	Note::Ptr NoteManager::create_new_note (std::string title, const std::string & guid)
	{
		std::string body;
		
		title = split_title_from_content (title, body);
		if (title.empty())
			return Note::Ptr();
			
		Note::Ptr note_template = find(m_note_template_title);
		if (note_template) {
			// Use the body from the "New Note Template" note
			std::string xml_content =
				boost::replace_first_copy(note_template->xml_content(), 
														 m_note_template_title,
														 title);
// TODO
//											 XmlEncoder.Encode (title));
			return create_new_note (title, xml_content, guid);
		}
			
		// Use a simple "Describe..." body and highlight
		// it so it can be easily overwritten
		body = _("Describe your new note here.");
		
		std::string header = title + "\n\n";
		std::string content =
			boost::str(boost::format("<note-content>%1%%2%</note-content>") %
								 header % body );
// TODO
//										 XmlEncoder.Encode (header) % XmlEncoder.Encode (body));
		
		Note::Ptr new_note = create_new_note (title, content, guid);
		
		// Select the inital
		// "Describe..." text so typing will overwrite the body text,
		//NoteBuffer 
		Glib::RefPtr<Gtk::TextBuffer> buffer = new_note->get_buffer();
// TODO this is not supposed to be NULL
		if(buffer) {
			Gtk::TextIter iter = buffer->get_iter_at_offset(header.size());
			buffer->move_mark (buffer->get_selection_bound(), iter);
			buffer->move_mark (buffer->get_insert(), buffer->end());
		}
		
		return new_note;
	}

	// Create a new note with the specified Xml content
	Note::Ptr NoteManager::create_new_note (const std::string & title, const std::string & xml_content, 
																				const std::string & guid)
	{ 
		if (title.empty())
			throw sharp::Exception("Invalid title");

		if (find(title))
			throw sharp::Exception("A note with this title already exists: " + title);

		std::string filename;
		if (!guid.empty())
			filename = make_new_file_name (guid);
		else
			filename = make_new_file_name ();

		Note::Ptr new_note = Note::create_new_note (title, filename, *this);
		new_note->xml_content() = xml_content;
		new_note->signal_renamed().connect(sigc::mem_fun(*this, &NoteManager::on_note_rename));
		new_note->signal_saved().connect(sigc::mem_fun(*this, &NoteManager::on_note_save));

		m_notes.push_back(new_note);

		// Load all the addins for the new note
		m_addin_mgr->LoadAddinsForNote (new_note);

		signal_note_added(new_note);

		return new_note;
	}

	/// <summary>
	/// Get the existing template note or create a new one
	/// if it doesn't already exist.
	/// </summary>
	/// <returns>
	/// A <see cref="Note"/>
	/// </returns>
	Note::Ptr NoteManager::get_or_create_template_note()
	{
		Note::Ptr template_note = find(m_note_template_title);
		if (!template_note) {
			template_note =
				create (m_note_template_title,
								get_note_template_content(m_note_template_title));
					
			// Select the initial text
//			NoteBuffer 
			Glib::RefPtr<Gtk::TextBuffer> buffer = template_note->get_buffer();
			if(buffer) {
				Gtk::TextIter iter = buffer->get_iter_at_line_offset(2, 0);
				buffer->move_mark(buffer->get_selection_bound(), iter);
				buffer->move_mark(buffer->get_insert(), buffer->end());
			}
			// Flag this as a template note
			// TODO
			Tag::Ptr tag;// = TagManager.GetOrCreateSystemTag (TagManager.TemplateNoteSystemTag);
			template_note->add_tag(tag);

			template_note->queue_save(Note::CONTENT_CHANGED);
		}
			
		return template_note;
	}
		
	std::string NoteManager::get_note_template_content(const std::string & title)
	{
		return str(boost::format("<note-content>"
														 "<note-title>%1%</note-title>\n\n"
														 "%2%"
														 "</note-content>") % title
// TODO
//			                      XmlEncoder.Encode (title),
							 % _("Describe your new note here."));
	}

	Note::Ptr NoteManager::find(const std::string & linked_title) const
	{
		foreach (Note::Ptr note, m_notes) {
			if (boost::to_lower_copy(note->title()) == boost::to_lower_copy(linked_title))
				return note;
		}
		return Note::Ptr();
	}

	Note::Ptr NoteManager::find_by_uri(const std::string & uri) const
	{
		foreach (Note::Ptr note, m_notes) {
			if (note->uri() == uri)
				return note;
		}
		return Note::Ptr();
	}


	TrieController::TrieController (NoteManager & manager)
		: m_manager(manager)
		,	m_title_trie(NULL)
	{
		m_manager.signal_note_deleted.connect(sigc::mem_fun(*this, &TrieController::on_note_deleted));
		m_manager.signal_note_added.connect(sigc::mem_fun(*this, &TrieController::on_note_added));
		m_manager.signal_note_renamed.connect(sigc::mem_fun(*this, &TrieController::on_note_renamed));

		update ();
	}

	TrieController::~TrieController()
	{
		delete m_title_trie;
	}

	void TrieController::on_note_added (const Note::Ptr & )
	{
		update ();
	}

	void TrieController::on_note_deleted (const Note::Ptr & )
	{
		update ();
	}

	void TrieController::on_note_renamed (const Note::Ptr & , const std::string & )
	{
		update ();
	}

	void TrieController::update ()
	{
		if(m_title_trie) {
			delete m_title_trie;
		}
		m_title_trie = new TrieTree(false /* !case_sensitive */);

		foreach (Note::Ptr note, m_manager.get_notes()) {
			m_title_trie->add_keyword (note->title(), note);
		}
	}

}
