

#ifndef _NOTEMANAGER_HPP__
#define _NOTEMANAGER_HPP__

#include <string>
#include <list>

#include <gconf/gconf.h>

#include <sigc++/signal.h>

#include "preferences.hpp"
#include "note.hpp"

namespace gnote {

	class AddinManager;
	class TrieController;
	class TrieTree;

	class NoteManager 
	{
	public:
		typedef boost::shared_ptr<NoteManager> Ptr;
		typedef sigc::signal<void, Note::Ptr> ChangedHandler;
		
		NoteManager(const std::string & );
		NoteManager(const std::string & directory, const std::string & backup);
		~NoteManager();

		void on_setting_changed(Preferences*, GConfEntry*);
		const Note::List & get_notes() const
			{ 
				return m_notes;
			}

		const std::string & start_note_uri() const
			{ return m_start_note_uri; }
		Note::Ptr find(const std::string &) const;
		Note::Ptr find_by_uri(const std::string &) const;
		void delete_note(const Note::Ptr & note);

		Note::Ptr create();
		Note::Ptr create(const std::string & title);
		Note::Ptr create(const std::string & title, const std::string & xml_content);
		Note::Ptr create_with_guid(const std::string & title, std::string & guid);
		Note::Ptr get_or_create_template_note();
		static std::string get_note_template_content(const std::string & title);
		static std::string split_title_from_content (std::string title, std::string & body);

		ChangedHandler signal_note_deleted;
		ChangedHandler signal_note_added;

		Note::RenamedHandler   signal_note_renamed;
		Note::SavedHandler     signal_note_saved;

	private:
		TrieController *create_trie_controller();
		AddinManager *create_addin_manager() const;
		bool directory_exists(const std::string & directory) const;
		bool create_directory(const std::string & directory) const;
		void on_note_rename(const Note::Ptr & note, const std::string & old_title);
		void create_start_notes();
		void on_note_save(const Note::Ptr & note);
		void load_notes();
		bool first_run() const;
		void create_notes_dir() const;
		void on_exiting_event();
		std::string make_new_file_name() const;
		std::string make_new_file_name(const std::string & guid) const;
		Note::Ptr create_new_note (std::string title, const std::string & guid);
		Note::Ptr create_new_note (const std::string & title, const std::string & xml_content, 
														 const std::string & guid);

		void _common_init(const std::string & directory, const std::string & backup);

		std::string m_notes_dir;
		std::string m_backup_dir;
		Note::List m_notes;
		AddinManager   *m_addin_mgr;
		TrieController *m_trie_controller;
		std::string m_note_template_title;
		std::string m_start_note_uri;
	};

	class TrieController
	{
	public:
		TrieController(NoteManager &);
		~TrieController();

		void update();
	private:
		void on_note_added (const Note::Ptr & added);
		void on_note_deleted (const Note::Ptr & deleted);
		void on_note_renamed (const Note::Ptr & renamed, const std::string & old_title);
			
		NoteManager & m_manager;
		TrieTree *    m_title_trie;
	};

}

#endif

