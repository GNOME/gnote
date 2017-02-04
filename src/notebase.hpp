/*
 * gnote
 *
 * Copyright (C) 2011-2014,2017 Aurimas Cernius
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


#ifndef _NOTEBASE_HPP_
#define _NOTEBASE_HPP_

#include <map>

#include <glibmm/ustring.h>
#include <sigc++/signal.h>

#include "base/macros.hpp"
#include "base/singleton.hpp"
#include "tag.hpp"
#include "sharp/datetime.hpp"
#include "sharp/xmlreader.hpp"
#include "sharp/xmlwriter.hpp"


namespace gnote {

class NoteManagerBase;


class NoteData
{
public:
  typedef std::map<Glib::ustring, Tag::Ptr> TagMap;

  static const int s_noPosition;

  NoteData(const Glib::ustring & _uri);

  const Glib::ustring & uri() const
    {
      return m_uri;
    }
  const Glib::ustring & title() const
    {
      return m_title;
    }
  Glib::ustring & title()
    {
      return m_title;
    }
  const Glib::ustring & text() const
    { 
      return m_text;
    }
  Glib::ustring & text()
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
  int selection_bound_position() const
    {
      return m_selection_bound_pos;
    }
  void set_selection_bound_position(int pos)
    {
      m_selection_bound_pos = pos;
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
  const TagMap & tags() const
    {
      return m_tags;
    }
  TagMap & tags()
    {
      return m_tags;
    }

  void set_extent(int width, int height);
  bool has_extent();

private:
  const Glib::ustring m_uri;
  Glib::ustring     m_title;
  Glib::ustring     m_text;
  sharp::DateTime             m_create_date;
  sharp::DateTime             m_change_date;
  sharp::DateTime             m_metadata_change_date;
  int               m_cursor_pos;
  int               m_selection_bound_pos;
  int               m_width, m_height;

  TagMap m_tags;
};


class NoteDataBufferSynchronizerBase
{
public:
  NoteDataBufferSynchronizerBase(NoteData *_data)
    : m_data(_data)
    {}
  virtual ~NoteDataBufferSynchronizerBase();
  const NoteData & data() const
    {
      return *m_data;
    }
  NoteData & data()
    {
      return *m_data;
    }
  virtual const NoteData & synchronized_data() const
    {
      return *m_data;
    }
  virtual NoteData & synchronized_data()
    {
      return *m_data;
    }
  virtual const Glib::ustring & text();
  virtual void set_text(const Glib::ustring & t);
private:
  NoteData *m_data;
};


class NoteBase
  : public enable_shared_from_this<NoteBase>
  , public sigc::trackable
{
public:
  typedef shared_ptr<NoteBase> Ptr;
  typedef weak_ptr<NoteBase> WeakPtr;
  typedef std::list<Ptr> List;

  static Glib::ustring url_from_path(const Glib::ustring &);
  static void parse_tags(const xmlNodePtr tagnodes, std::list<Glib::ustring> & tags);

  NoteBase(NoteData *_data, const Glib::ustring & filepath, NoteManagerBase & manager);

  NoteManagerBase & manager()
    {
      return m_manager;
    }
  const NoteManagerBase & manager() const
    {
      return m_manager;
    }

  int get_hash_code() const;
  const Glib::ustring & uri() const;
  const Glib::ustring id() const;
  const Glib::ustring & get_title() const;
  void set_title(const Glib::ustring & new_title);
  virtual void set_title(const Glib::ustring & new_title, bool from_user_action);
  virtual void rename_without_link_update(const Glib::ustring & newTitle);

  virtual void queue_save(ChangeType c);
  virtual void save();
  void rename_links(const Glib::ustring & old_title, const Ptr & renamed);
  void remove_links(const Glib::ustring & old_title, const Ptr & renamed);
  virtual void delete_note();
  void add_tag(const Tag::Ptr &);
  virtual void remove_tag(Tag &);
  void remove_tag(const Tag::Ptr &);
  bool contains_tag(const Tag::Ptr &) const;

  const Glib::ustring & file_path() const
    {
      return m_file_path;
    }
  Glib::ustring get_complete_note_xml();
  const Glib::ustring & xml_content()
    {
      return data_synchronizer().text();
    }
  virtual void set_xml_content(const Glib::ustring & xml);
  void load_foreign_note_xml(const Glib::ustring & foreignNoteXml, ChangeType changeType);
  void get_tags(std::list<Tag::Ptr> &) const;
  const NoteData & data() const;
  NoteData & data();

  const sharp::DateTime & create_date() const;
  const sharp::DateTime & change_date() const;
  const sharp::DateTime & metadata_change_date() const;
  bool is_new() const;
  bool enabled() const
    {
      return m_enabled;
    }
  virtual void enabled(bool is_enabled);

  typedef sigc::signal<void, const NoteBase::Ptr &> SavedHandler;
  SavedHandler signal_saved;
  typedef sigc::signal<void, const NoteBase::Ptr&, const Glib::ustring& > RenamedHandler;
  RenamedHandler signal_renamed;
  typedef sigc::signal<void, const NoteBase&, const Tag::Ptr&> TagAddedHandler;
  TagAddedHandler signal_tag_added;
  typedef sigc::signal<void, const NoteBase&, const Tag &> TagRemovingHandler;  
  TagRemovingHandler signal_tag_removing;
  typedef sigc::signal<void, const NoteBase::Ptr&, const Glib::ustring&> TagRemovedHandler;
  TagRemovedHandler signal_tag_removed;
protected:
  virtual const NoteDataBufferSynchronizerBase & data_synchronizer() const = 0;
  virtual NoteDataBufferSynchronizerBase & data_synchronizer() = 0;
  virtual void process_rename_link_update(const Glib::ustring & old_title);
  void set_change_type(ChangeType c);
  virtual void handle_link_rename(const Glib::ustring & old_title, const Ptr & renamed, bool rename);
private:
  NoteManagerBase & m_manager;
  Glib::ustring m_file_path;
  bool m_enabled;
};


class NoteArchiver
  : public base::Singleton<NoteArchiver>
{
public:
  static const char *CURRENT_VERSION;

  static void read(const Glib::ustring & read_file, NoteData & data);
  static Glib::ustring write_string(const NoteData & data);
  static void write(const Glib::ustring & write_file, const NoteData & data);
  void read_file(const Glib::ustring & file, NoteData & data);
  void read(sharp::XmlReader & xml, NoteData & data);
  void write_file(const Glib::ustring & write_file, const NoteData & data);
  void write(sharp::XmlWriter & xml, const NoteData & data);

  Glib::ustring get_renamed_note_xml(const Glib::ustring &, const Glib::ustring &, const Glib::ustring &) const;
  Glib::ustring get_title_from_note_xml(const Glib::ustring & noteXml) const;
protected:
  void _read(sharp::XmlReader & xml, NoteData & data, Glib::ustring & version);

  static NoteArchiver s_obj;
};


}

#endif

