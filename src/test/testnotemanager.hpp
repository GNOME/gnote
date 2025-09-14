/*
 * gnote
 *
 * Copyright (C) 2014,2019-2020,2022,2025 Aurimas Cernius
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

#include "ignote.hpp"
#include "notemanagerbase.hpp"
#include "notebooks/notebookmanager.hpp"
#include "test/testtagmanager.hpp"

namespace test {

void remove_dir(const Glib::ustring &dir);

class NoteManager
  : public gnote::NoteManagerBase
{
public:
  static Glib::ustring test_notes_dir();

  NoteManager(const Glib::ustring & notes_dir, gnote::IGnote & g);
  virtual ~NoteManager();

  virtual gnote::notebooks::NotebookManager & notebook_manager() override
    {
      return m_notebook_manager;
    }
  virtual gnote::NoteArchiver & note_archiver() override
    {
      return m_note_archiver;
    }
  virtual const gnote::ITagManager & tag_manager() const override
    {
      return m_tag_manager;
    }
  virtual gnote::ITagManager & tag_manager() override
    {
      return m_tag_manager;
    }

  using gnote::NoteManagerBase::delete_old_backups;
protected:
  virtual gnote::NoteBase::Ptr note_create_new(Glib::ustring && title, Glib::ustring && file_name) override;
  gnote::NoteBase::Ptr note_load(Glib::ustring && file_name) override;
private:
  gnote::notebooks::NotebookManager m_notebook_manager;
  gnote::NoteArchiver m_note_archiver;
  TagManager m_tag_manager;
};

}
