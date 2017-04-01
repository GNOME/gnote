/*
 * gnote
 *
 * Copyright (C) 2012-2014,2017 Aurimas Cernius
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


#ifndef _SYNCHRONIZATION_SYNCDIALOG_HPP_
#define _SYNCHRONIZATION_SYNCDIALOG_HPP_


#include <gtkmm/dialog.h>
#include <gtkmm/expander.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeviewcolumn.h>

#include "base/macros.hpp"
#include "syncui.hpp"


namespace gnote {
namespace sync {

  class SyncDialog
    : public Gtk::Dialog
    , public SyncUI
  {
  public:
    typedef shared_ptr<SyncDialog> Ptr;

    static Ptr create(NoteManagerBase &);

    virtual void sync_state_changed(SyncState state) override;
    virtual void note_synchronized(const Glib::ustring & noteTitle, NoteSyncType type) override;
    virtual void note_conflict_detected(const Note::Ptr & localConflictNote,
                                        NoteUpdate remoteNote,
                                        const std::list<Glib::ustring> & noteUpdateTitles) override;
    virtual void present_ui() override;
    void header_text(const Glib::ustring &);
    void message_text(const Glib::ustring &);
    Glib::ustring progress_text() const;
    void progress_text(const Glib::ustring &);
    void add_update_item(const Glib::ustring & title, Glib::ustring & status);
  protected:
    virtual void on_realize() override;
  private:
    static void on_expander_activated(GtkExpander*, gpointer);
    void note_conflict_detected_(const Note::Ptr & localConflictNote,
                                 NoteUpdate remoteNote,
                                 const std::list<Glib::ustring> & noteUpdateTitles,
                                 SyncTitleConflictResolution savedBehavior,
                                 SyncTitleConflictResolution resolution,
                                 std::exception **mainThreadException);

    explicit SyncDialog(NoteManagerBase &);
    bool on_pulse_progress_bar();
    void on_row_activated(const Gtk::TreeModel::Path & path, Gtk::TreeViewColumn *column);
    void treeview_col1_data_func(Gtk::CellRenderer *renderer, const Gtk::TreeIter & iter);
    void treeview_col2_data_func(Gtk::CellRenderer *renderer, const Gtk::TreeIter & iter);
    void sync_state_changed_(SyncState state);
    void rename_note(const Note::Ptr & note, const Glib::ustring & newTitle, bool updateReferencingNotes);
    void present_note(const Note::Ptr &);

    Gtk::Image *m_image;
    Gtk::Label *m_header_label;
    Gtk::Label *m_message_label;
    Gtk::ProgressBar *m_progress_bar;
    Gtk::Label *m_progress_label;

    Gtk::Expander *m_expander;
    Gtk::Button *m_close_button;
    unsigned m_progress_bar_timeout_id;

    Glib::RefPtr<Gtk::TreeStore> m_model;
  };

}
}


#endif
