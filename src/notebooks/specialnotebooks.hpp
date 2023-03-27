/*
 * gnote
 *
 * Copyright (C) 2010-2014,2017,2019,2022 Aurimas Cernius
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

#include "notebook.hpp"
#include "tag.hpp"


namespace gnote {

class IconManager;

namespace notebooks {


class SpecialNotebook
  : public Notebook
{
public:
  typedef std::shared_ptr<SpecialNotebook> Ptr;

  virtual Glib::ustring get_icon_name() const = 0;
protected:
  SpecialNotebook(NoteManagerBase & m, const Glib::ustring &s)
    : Notebook(m, s, true)
    {
    }
  Tag::Ptr get_tag() const override;
  Note::Ptr get_template_note() const override;
};


class AllNotesNotebook
  : public SpecialNotebook
{
public:
  typedef std::shared_ptr<AllNotesNotebook> Ptr;
  AllNotesNotebook(NoteManagerBase &);
  Glib::ustring get_normalized_name() const override;
  bool contains_note(const Note::Ptr & note, bool include_system = false) override;
  bool add_note(const Note::Ptr &) override;
  Glib::ustring get_icon_name() const override;
};


class UnfiledNotesNotebook
  : public SpecialNotebook
{
public:
  typedef std::shared_ptr<UnfiledNotesNotebook> Ptr;
  UnfiledNotesNotebook(NoteManagerBase &);
  Glib::ustring get_normalized_name() const override;
  bool contains_note(const Note::Ptr & note, bool include_system = false) override;
  bool add_note(const Note::Ptr &) override;
  Glib::ustring get_icon_name() const override;
};


class PinnedNotesNotebook
  : public SpecialNotebook
{
public:
  typedef std::shared_ptr<PinnedNotesNotebook> Ptr;
  PinnedNotesNotebook(NoteManagerBase &);
  Glib::ustring get_normalized_name() const override;
  bool contains_note(const Note::Ptr & note, bool include_system = false) override;
  bool add_note(const Note::Ptr &) override;
  virtual Glib::ustring get_icon_name() const override;
};


class ActiveNotesNotebook
  : public SpecialNotebook
{
public:
  typedef std::shared_ptr<ActiveNotesNotebook> Ptr;
  ActiveNotesNotebook(NoteManagerBase &);
  Glib::ustring get_normalized_name() const override;
  bool contains_note(const Note::Ptr & note, bool include_system = false) override;
  bool add_note(const Note::Ptr &) override;
  Glib::ustring get_icon_name() const override;
  bool empty();
  sigc::signal<void()> signal_size_changed;
private:
  void on_note_deleted(const NoteBase::Ptr & note);

  std::set<Note::Ptr> m_notes;
};


}
}

#endif

