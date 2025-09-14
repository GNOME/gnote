/*
 * gnote
 *
 * Copyright (C) 2010-2014,2017,2019-2020,2022-2025 Aurimas Cernius
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

#include <unordered_set>

#include "itagmanager.hpp"
#include "notebase.hpp"
#include "triehit.hpp"


namespace gnote {

namespace notebooks {

class NotebookManager;

}

class IGnote;
class TrieController;

class NoteManagerBase
{
public:
  typedef sigc::signal<void(NoteBase&)> ChangedHandler;

  static Glib::ustring sanitize_xml_content(const Glib::ustring & xml_content);
  static Glib::ustring get_note_template_content(const Glib::ustring & title);
  static Glib::ustring get_note_content(const Glib::ustring & title, const Glib::ustring & body);
  static Glib::ustring split_title_from_content(Glib::ustring title, Glib::ustring & body);

  NoteManagerBase(IGnote & g);
  virtual ~NoteManagerBase();

  IGnote& gnote()
    {
      return m_gnote;
    }
  virtual notebooks::NotebookManager & notebook_manager() = 0;
  size_t trie_max_length();
  TrieHit<Glib::ustring>::List find_trie_matches(const Glib::ustring &);

  virtual NoteArchiver & note_archiver() = 0;
  virtual const ITagManager & tag_manager() const = 0;
  virtual ITagManager & tag_manager() = 0;

  void read_only(bool ro)
    {
      m_read_only = ro;
    }
  bool read_only() const
    {
      return m_read_only;
    }
  NoteBase::ORef find(const Glib::ustring &) const;
  NoteBase::ORef find_by_uri(const Glib::ustring &) const;
  template <typename F>
  bool find_by_uri(const Glib::ustring & uri, const F & func) const
    {
      if(auto note = find_by_uri(uri)) {
        func(*note);
        return true;
      }
      return false;
    }
  std::vector<NoteBase::Ref> get_notes_linking_to(const Glib::ustring & title) const;
  NoteBase & create();
  NoteBase & create(Glib::ustring && title);
  NoteBase & create(Glib::ustring && title, Glib::ustring && xml_content);
  NoteBase & create_note_from_template(Glib::ustring && title, const NoteBase & template_note);
  virtual NoteBase & get_or_create_template_note();
  NoteBase::ORef find_template_note() const;
  Glib::ustring get_unique_name(const Glib::ustring & basename) const;
  void delete_note(NoteBase & note);
  // Import a note read from file_path
  // Will ensure the sanity including the unique title.
  NoteBase::ORef import_note(const Glib::ustring & file_path);
  NoteBase & create_with_guid(Glib::ustring && title, Glib::ustring && guid);

  const Glib::ustring & notes_dir() const
    {
      return m_notes_dir;
    }
  std::size_t note_count() const
    {
      return m_notes.size();
    }

  template <typename F>
  void for_each(const F & func) const
    {
      for(const auto & note : m_notes) {
        func(*note);
      }
    }

  template <typename RetT, typename FuncT>
  RetT search(const FuncT & func, const RetT & default_ret) const
    {
      RetT ret{default_ret};
      for(const auto & note : m_notes) {
        if(!func(*note, ret)) {
          break;
        }
      }
      return ret;
    }

  template <typename C, typename F>
  void copy_to(C & container, const F & func) const
    {
      for(const auto & note : m_notes) {
        func(container, note);
      }
    }

  ChangedHandler signal_note_deleted;
  ChangedHandler signal_note_added;
  NoteBase::RenamedHandler signal_note_renamed;
  NoteBase::SavedHandler signal_note_saved;
protected:
  static void delete_old_backups(const Glib::ustring &backup, const Glib::DateTime &keep_since);

  bool init(const Glib::ustring & directory, const Glib::ustring & backup);
  virtual void post_load();
  virtual void migrate_notes(const Glib::ustring & old_note_dir);
  /** add the note to the manager and setup signals */
  void add_note(NoteBase::Ptr);
  void on_note_rename(const NoteBase & note, const Glib::ustring & old_title);
  void on_note_save(NoteBase & note);
  virtual NoteBase & create_note_from_template(Glib::ustring && title, const NoteBase & template_note, Glib::ustring && guid);
  virtual NoteBase & create_note(Glib::ustring && title, Glib::ustring && body, Glib::ustring && guid = Glib::ustring());
  virtual NoteBase & create_new_note(Glib::ustring && title, Glib::ustring && xml_content, Glib::ustring && guid);
  virtual NoteBase::Ptr note_create_new(Glib::ustring && title, Glib::ustring && file_name) = 0;
  Glib::ustring make_new_file_name() const;
  Glib::ustring make_new_file_name(const Glib::ustring & guid) const;
  virtual NoteBase::Ptr note_load(Glib::ustring && file_name) = 0;

  struct NoteHash
  {
    std::size_t operator()(const NoteBase::Ptr &) const noexcept;
  };

  std::unordered_set<NoteBase::Ptr, NoteHash> m_notes;
  Glib::ustring m_backup_dir;
  Glib::ustring m_default_note_template_title;
private:
  bool first_run() const;
  void create_notes_dir() const;
  bool create_directory(const Glib::ustring & directory) const;
  TrieController *create_trie_controller();

  IGnote & m_gnote;
  TrieController *m_trie_controller;
  Glib::ustring m_notes_dir;
  bool m_read_only;
};

}

#endif
