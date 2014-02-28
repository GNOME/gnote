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



#ifndef __NOTEBOOKS_SPECIALNOTEBOOKS_HPP_
#define __NOTEBOOKS_SPECIALNOTEBOOKS_HPP_


#include <set>

#include "base/macros.hpp"
#include "notebook.hpp"
#include "tag.hpp"


namespace gnote {
namespace notebooks {


class SpecialNotebook
  : public Notebook
{
public:
  typedef shared_ptr<SpecialNotebook> Ptr;

  virtual Glib::RefPtr<Gdk::Pixbuf> get_icon() = 0;
protected:
  SpecialNotebook(NoteManager & m, const std::string &s)
    : Notebook(m, s, true)
    {
    }
  virtual Tag::Ptr    get_tag() const override;
  virtual Note::Ptr   get_template_note() const override;
};


class AllNotesNotebook
  : public SpecialNotebook
{
public:
  typedef shared_ptr<AllNotesNotebook> Ptr;
  AllNotesNotebook(NoteManager &);
  virtual std::string get_normalized_name() const override;
  virtual bool        contains_note(const Note::Ptr & note, bool include_system = false) override;
  virtual bool        add_note(const Note::Ptr &) override;
  virtual Glib::RefPtr<Gdk::Pixbuf> get_icon() override;
};


class UnfiledNotesNotebook
  : public SpecialNotebook
{
public:
  typedef shared_ptr<UnfiledNotesNotebook> Ptr;
  UnfiledNotesNotebook(NoteManager &);
  virtual std::string get_normalized_name() const override;
  virtual bool        contains_note(const Note::Ptr & note, bool include_system = false) override;
  virtual bool        add_note(const Note::Ptr &) override;
  virtual Glib::RefPtr<Gdk::Pixbuf> get_icon() override;
};


class PinnedNotesNotebook
  : public SpecialNotebook
{
public:
  typedef shared_ptr<PinnedNotesNotebook> Ptr;
  PinnedNotesNotebook(NoteManager &);
  virtual std::string get_normalized_name() const override;
  virtual bool        contains_note(const Note::Ptr & note, bool include_system = false) override;
  virtual bool        add_note(const Note::Ptr &) override;
  virtual Glib::RefPtr<Gdk::Pixbuf> get_icon() override;
};


class ActiveNotesNotebook
  : public SpecialNotebook
{
public:
  typedef shared_ptr<ActiveNotesNotebook> Ptr;
  ActiveNotesNotebook(NoteManager &);
  virtual std::string get_normalized_name() const override;
  virtual bool        contains_note(const Note::Ptr & note, bool include_system = false) override;
  virtual bool        add_note(const Note::Ptr &) override;
  virtual Glib::RefPtr<Gdk::Pixbuf> get_icon() override;
  bool empty();
  sigc::signal<void> signal_size_changed;
private:
  void on_note_deleted(const NoteBase::Ptr & note);

  std::set<Note::Ptr> m_notes;
};


}
}

#endif

