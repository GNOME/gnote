/*
 * gnote
 *
 * Copyright (C) 2010-2014,2017,2019,2022-2024 Aurimas Cernius
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


#include <unordered_set>

#include "notebook.hpp"
#include "tag.hpp"
#include "base/hash.hpp"


namespace gnote {

class IconManager;

namespace notebooks {


class SpecialNotebook
  : public Notebook
{
public:
  typedef Glib::RefPtr<SpecialNotebook> Ptr;

  virtual Glib::ustring get_icon_name() const = 0;
protected:
  SpecialNotebook(NoteManagerBase & m, const Glib::ustring &s)
    : Notebook(m, s, true)
    {
    }
  Tag::ORef get_tag() const override;
  Note & get_template_note() const override;
};


class AllNotesNotebook
  : public SpecialNotebook
{
public:
  typedef Glib::RefPtr<AllNotesNotebook> Ptr;
  static Ptr create(NoteManagerBase&);
  Glib::ustring get_normalized_name() const override;
  bool contains_note(const Note & note, bool include_system = false) override;
  bool add_note(Note&) override;
  Glib::ustring get_icon_name() const override;
private:
  explicit AllNotesNotebook(NoteManagerBase &);
};


class UnfiledNotesNotebook
  : public SpecialNotebook
{
public:
  typedef Glib::RefPtr<UnfiledNotesNotebook> Ptr;
  static Ptr create(NoteManagerBase&);
  Glib::ustring get_normalized_name() const override;
  bool contains_note(const Note & note, bool include_system = false) override;
  bool add_note(Note&) override;
  Glib::ustring get_icon_name() const override;
private:
  explicit UnfiledNotesNotebook(NoteManagerBase &);
};


class PinnedNotesNotebook
  : public SpecialNotebook
{
public:
  typedef Glib::RefPtr<PinnedNotesNotebook> Ptr;
  static Ptr create(NoteManagerBase&);
  Glib::ustring get_normalized_name() const override;
  bool contains_note(const Note & note, bool include_system = false) override;
  bool add_note(Note&) override;
  virtual Glib::ustring get_icon_name() const override;
private:
  explicit PinnedNotesNotebook(NoteManagerBase &);
};


class ActiveNotesNotebook
  : public SpecialNotebook
{
public:
  typedef Glib::RefPtr<ActiveNotesNotebook> Ptr;
  static Ptr create(NoteManagerBase&);
  Glib::ustring get_normalized_name() const override;
  bool contains_note(const Note & note, bool include_system = false) override;
  bool add_note(Note&) override;
  Glib::ustring get_icon_name() const override;
  bool empty();
private:
  explicit ActiveNotesNotebook(NoteManagerBase &);
  void on_note_deleted(NoteBase & note);

  std::unordered_set<Glib::ustring, Hash<Glib::ustring>> m_notes;
};


}
}

#endif

