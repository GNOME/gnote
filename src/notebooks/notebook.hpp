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



#ifndef __NOTEBOOKS_NOTEBOOK_HPP_
#define __NOTEBOOKS_NOTEBOOK_HPP_

#include <set>
#include <string>
#include <tr1/memory>

#include "tag.hpp"
#include "note.hpp"

namespace gnote {
namespace notebooks {


/// <summary>
/// An object that represents a notebook in Tomboy
/// </summary>
class Notebook 
  : public std::tr1::enable_shared_from_this<Notebook>
{
public:
  typedef std::tr1::shared_ptr<Notebook> Ptr;
  static const char * NOTEBOOK_TAG_PREFIX;
  Notebook(NoteManager &, const std::string &, bool is_special = false);
  Notebook(NoteManager &, const Tag::Ptr &);
  std::string get_name() const
    { return m_name; }
  void set_name(const std::string &);
  virtual std::string get_normalized_name() const;
  virtual Tag::Ptr    get_tag() const;
  Note::Ptr find_template_note() const;
  virtual Note::Ptr   get_template_note() const;
  Note::Ptr create_notebook_note();
  virtual bool contains_note(const Note::Ptr &);
  virtual bool add_note(const Note::Ptr &);
  virtual Glib::RefPtr<Gdk::Pixbuf> get_icon();
  static std::string normalize(const std::string & s);
////
  virtual ~Notebook()
    {}
protected:
  NoteManager & m_note_manager;
private:
  Notebook(const Notebook &);
  Notebook & operator=(const Notebook &);
  std::string m_name;
  std::string m_normalized_name;
  std::string m_default_template_note_title;
  Tag::Ptr    m_tag;
};


/// <summary>
/// A notebook of this type is special in the sense that it
/// will not normally be displayed to the user as a notebook
/// but it's used in the Search All Notes Window for special
/// filtering of the notes.
/// </summary>
class SpecialNotebook
  : public Notebook
{
public:
  typedef std::tr1::shared_ptr<SpecialNotebook> Ptr;
protected:
  SpecialNotebook(NoteManager & m, const std::string &s)
    : Notebook(m, s, true)
    {
    }
  virtual Tag::Ptr    get_tag() const;
  virtual Note::Ptr   get_template_note() const;
};


/// <summary>
/// A special notebook that represents really "no notebook" as
/// being selected.  This notebook is used in the Search All
/// Notes Window to allow users to select it at the top of the
/// list so that all notes are shown.
/// </summary>
class AllNotesNotebook
  : public SpecialNotebook
{
public:
  typedef std::tr1::shared_ptr<AllNotesNotebook> Ptr;
  AllNotesNotebook(NoteManager &);
  virtual std::string get_normalized_name() const;
  virtual bool        contains_note(const Note::Ptr &);
  virtual bool        add_note(const Note::Ptr &);
  virtual Glib::RefPtr<Gdk::Pixbuf> get_icon();
};


/// <summary>
/// A special notebook that represents a notebook with notes
/// that are not filed.  This is used in the Search All Notes
/// Window to filter notes that are not placed in any notebook.
/// </summary>
class UnfiledNotesNotebook
  : public SpecialNotebook
{
public:
  typedef std::tr1::shared_ptr<UnfiledNotesNotebook> Ptr;
  UnfiledNotesNotebook(NoteManager &);
  virtual std::string get_normalized_name() const;
  virtual bool        contains_note(const Note::Ptr &);
  virtual bool        add_note(const Note::Ptr &);
  virtual Glib::RefPtr<Gdk::Pixbuf> get_icon();
};


class PinnedNotesNotebook
  : public SpecialNotebook
{
public:
  typedef std::tr1::shared_ptr<PinnedNotesNotebook> Ptr;
  PinnedNotesNotebook(NoteManager &);
  virtual std::string get_normalized_name() const;
  virtual bool        contains_note(const Note::Ptr &);
  virtual bool        add_note(const Note::Ptr &);
  virtual Glib::RefPtr<Gdk::Pixbuf> get_icon();
};


class ActiveNotesNotebook
  : public SpecialNotebook
{
public:
  typedef std::tr1::shared_ptr<ActiveNotesNotebook> Ptr;
  ActiveNotesNotebook(NoteManager &);
  virtual std::string get_normalized_name() const;
  virtual bool        contains_note(const Note::Ptr &);
  virtual bool        add_note(const Note::Ptr &);
  virtual Glib::RefPtr<Gdk::Pixbuf> get_icon();
  bool empty()
    {
      return m_notes.size() == 0;
    }
  sigc::signal<void> signal_size_changed;
private:
  void on_note_deleted(const Note::Ptr & note);

  std::set<Note::Ptr> m_notes;
};


}
}

#endif
