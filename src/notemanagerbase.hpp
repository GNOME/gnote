/*
 * gnote
 *
 * Copyright (C) 2010-2014 Aurimas Cernius
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


#ifndef _NOTEMANAGERBASE_HPP_
#define _NOTEMANAGERBASE_HPP_

#include "notebase.hpp"
#include "triehit.hpp"


namespace gnote {

class TrieController;

class NoteManagerBase
{
public:
  typedef sigc::signal<void, const NoteBase::Ptr &> ChangedHandler;

  static Glib::ustring sanitize_xml_content(const Glib::ustring & xml_content);
  static Glib::ustring get_note_template_content(const Glib::ustring & title);
  static Glib::ustring split_title_from_content(Glib::ustring title, Glib::ustring & body);

  NoteManagerBase(const Glib::ustring & directory);
  virtual ~NoteManagerBase();

  size_t trie_max_length();
  TrieHit<NoteBase::WeakPtr>::ListPtr find_trie_matches(const Glib::ustring &);

  void read_only(bool ro)
    {
      m_read_only = ro;
    }
  bool read_only() const
    {
      return m_read_only;
    }
  NoteBase::Ptr find(const Glib::ustring &) const;
  NoteBase::Ptr find_by_uri(const std::string &) const;
  NoteBase::List get_notes_linking_to(const Glib::ustring & title) const;
  NoteBase::Ptr create();
  NoteBase::Ptr create(const Glib::ustring & title);
  NoteBase::Ptr create(const Glib::ustring & title, const Glib::ustring & xml_content);
  NoteBase::Ptr create_note_from_template(const Glib::ustring & title, const NoteBase::Ptr & template_note);
  virtual NoteBase::Ptr get_or_create_template_note();
  NoteBase::Ptr find_template_note() const;
  Glib::ustring get_unique_name(const Glib::ustring & basename) const;
  void delete_note(const NoteBase::Ptr & note);
  // Import a note read from file_path
  // Will ensure the sanity including the unique title.
  NoteBase::Ptr import_note(const Glib::ustring & file_path);
  NoteBase::Ptr create_with_guid(const Glib::ustring & title, const std::string & guid);

  const Glib::ustring & notes_dir() const
    {
      return m_notes_dir;
    }
  const NoteBase::List & get_notes() const
    { 
      return m_notes;
    }

  const std::string & start_note_uri() const
    { 
      return m_start_note_uri; 
    }

  ChangedHandler signal_note_deleted;
  ChangedHandler signal_note_added;
  NoteBase::RenamedHandler signal_note_renamed;
  NoteBase::SavedHandler signal_note_saved;
protected:
  virtual void _common_init(const Glib::ustring & directory, const Glib::ustring & backup);
  bool first_run() const;
  virtual void post_load();
  virtual void migrate_notes(const std::string & old_note_dir);
  /** add the note to the manager and setup signals */
  void add_note(const NoteBase::Ptr &);
  void on_note_rename(const NoteBase::Ptr & note, const Glib::ustring & old_title);
  void on_note_save(const NoteBase::Ptr & note);
  virtual NoteBase::Ptr create_note_from_template(const Glib::ustring & title,
                                                  const NoteBase::Ptr & template_note,
                                                  const std::string & guid);
  virtual NoteBase::Ptr create_new_note(Glib::ustring title, const std::string & guid);
  virtual NoteBase::Ptr create_new_note(const Glib::ustring & title, const Glib::ustring & xml_content, 
                                        const std::string & guid);
  virtual NoteBase::Ptr note_create_new(const Glib::ustring & title, const Glib::ustring & file_name) = 0;
  Glib::ustring make_new_file_name() const;
  Glib::ustring make_new_file_name(const Glib::ustring & guid) const;
  virtual NoteBase::Ptr note_load(const Glib::ustring & file_name) = 0;

  NoteBase::List m_notes;
  std::string m_start_note_uri;
  Glib::ustring m_backup_dir;
  Glib::ustring m_default_note_template_title;
private:
  void create_notes_dir() const;
  bool create_directory(const Glib::ustring & directory) const;
  TrieController *create_trie_controller();

  TrieController *m_trie_controller;
  Glib::ustring m_notes_dir;
  bool m_read_only;
};

}

#endif
