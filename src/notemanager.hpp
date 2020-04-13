/*
 * gnote
 *
 * Copyright (C) 2010-2014,2017,2019-2020 Aurimas Cernius
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

#include "notemanagerbase.hpp"
#include "note.hpp"
#include "preferences.hpp"
#include "tagmanager.hpp"
#include "notebooks/notebookmanager.hpp"

namespace gnote {

  class AddinManager;

  class NoteManager 
    : public NoteManagerBase
  {
  public:
    typedef sigc::slot<void, const Note::Ptr &> NoteChangedSlot;
    
    NoteManager(IGnote & g);
    virtual ~NoteManager();

    void init(const Glib::ustring &);

    virtual notebooks::NotebookManager & notebook_manager() override
      {
        return m_notebook_manager;
      }
    virtual NoteArchiver & note_archiver() override
      {
        return m_note_archiver;
      }
    virtual const ITagManager & tag_manager() const override
      {
        return m_tag_manager;
      }
    virtual ITagManager & tag_manager() override
      {
        return m_tag_manager;
      }

    void on_setting_changed(const Glib::ustring & key);

    AddinManager & get_addin_manager()
      {
        return *m_addin_mgr;
      }

    virtual NoteBase::Ptr get_or_create_template_note() override;

    ChangedHandler signal_note_buffer_changed;

    using NoteManagerBase::create_note_from_template;
  protected:
    virtual void post_load() override;
    virtual void migrate_notes(const Glib::ustring & old_note_dir) override;
    virtual NoteBase::Ptr create_note_from_template(const Glib::ustring & title,
                                                    const NoteBase::Ptr & template_note,
                                                    const Glib::ustring & guid) override;
    virtual NoteBase::Ptr create_note(Glib::ustring title, Glib::ustring body) override;
    virtual NoteBase::Ptr create_new_note(Glib::ustring title, const Glib::ustring & guid) override;
    virtual NoteBase::Ptr create_new_note(const Glib::ustring & title, const Glib::ustring & xml_content,
                                          const Glib::ustring & guid) override;
    virtual NoteBase::Ptr note_create_new(const Glib::ustring & title, const Glib::ustring & file_name) override;
    virtual NoteBase::Ptr note_load(const Glib::ustring & file_name) override;
  private:
    AddinManager *create_addin_manager();
    void create_start_notes();
    void load_notes();
    void on_exiting_event();

    Preferences & m_preferences;
    notebooks::NotebookManager m_notebook_manager;
    AddinManager   *m_addin_mgr;
    NoteArchiver m_note_archiver;
    TagManager m_tag_manager;
  };


}

#endif

