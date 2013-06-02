/*
 * gnote
 *
 * Copyright (C) 2010-2013 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



#ifndef _NOTEMANAGER_HPP__
#define _NOTEMANAGER_HPP__

#include <string>
#include <list>
#include <memory>

#include <sigc++/signal.h>

#include "preferences.hpp"
#include "note.hpp"
#include "triehit.hpp"

namespace gnote {

  class AddinManager;
  class TrieController;

  class NoteManager 
  {
  public:
    typedef std::tr1::shared_ptr<NoteManager> Ptr;
    typedef sigc::signal<void, const Note::Ptr &> ChangedHandler;
    typedef sigc::slot<void, const Note::Ptr &> NoteChangedSlot;
    
    NoteManager(const std::string &);
    ~NoteManager();

    void on_setting_changed(const Glib::ustring & key);
    const Note::List & get_notes() const
      { 
        return m_notes;
      }

    // the trie for the note names
    size_t trie_max_length();
    TrieHit<Note::WeakPtr>::ListPtr find_trie_matches(const std::string &);

    AddinManager & get_addin_manager()
      {
        return *m_addin_mgr;
      }
    const std::string & get_notes_dir() const
      {
        return m_notes_dir;
      }
    const std::string & start_note_uri() const
      { 
        return m_start_note_uri; 
      }
    void read_only(bool ro)
      {
        m_read_only = ro;
      }
    bool read_only() const
      {
        return m_read_only;
      }
    Note::Ptr find(const std::string &) const;
    Note::Ptr find_by_uri(const std::string &) const;
    static std::string sanitize_xml_content(const std::string & xml_content);
    Note::Ptr create_note_from_template(const std::string & title, const Note::Ptr & template_note);
    std::string get_unique_name (const std::string & basename) const;
    void delete_note(const Note::Ptr & note);

    Note::Ptr create();
    Note::Ptr create(const std::string & title);
    Note::Ptr create(const std::string & title, const std::string & xml_content);
    // Import a note read from file_path
    // Will ensure the sanity including the unique title.
    Note::Ptr import_note(const std::string & file_path);
    Note::Ptr create_with_guid(const std::string & title, const std::string & guid);
    Note::Ptr find_template_note() const;
    Note::Ptr get_or_create_template_note();
    static std::string get_note_template_content(const std::string & title);
    static std::string split_title_from_content (std::string title, std::string & body);

    ChangedHandler signal_note_deleted;
    ChangedHandler signal_note_added;
    ChangedHandler signal_note_buffer_changed;
    /** this signal is emitted when the start note has been created
     *  This is supposed to happen once in a life time *sigh*
     *  This to avoid relying a the Gnote class for that.
     */

    Note::RenamedHandler   signal_note_renamed;
    Note::SavedHandler     signal_note_saved;

  private:
    TrieController *create_trie_controller();
    AddinManager *create_addin_manager();
    bool directory_exists(const std::string & directory) const;
    bool create_directory(const std::string & directory) const;
    void on_note_rename(const Note::Ptr & note, const std::string & old_title);
    void create_start_notes();
    void on_note_save(const Note::Ptr & note);
    void load_notes();
    void migrate_notes(const std::string & old_note_dir);
    void post_load();
    bool first_run() const;
    void create_notes_dir() const;
    void on_exiting_event();
    std::string make_new_file_name() const;
    std::string make_new_file_name(const std::string & guid) const;
    Note::Ptr create_new_note (std::string title, const std::string & guid);
    Note::Ptr create_new_note (const std::string & title, const std::string & xml_content, 
                             const std::string & guid);
    Note::Ptr create_note_from_template(const std::string & title, const Note::Ptr & template_note, const std::string & guid);
    /** add the note to the manager and setup signals */
    void add_note(const Note::Ptr &);
    void _common_init(const std::string & directory, const std::string & backup);

    std::string m_notes_dir;
    std::string m_backup_dir;
    Note::List m_notes;
    AddinManager   *m_addin_mgr;
    TrieController *m_trie_controller;
    std::string m_default_note_template_title;
    std::string m_start_note_uri;
    bool m_read_only;
  };


}

#endif

