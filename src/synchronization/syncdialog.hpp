/*
 * gnote
 *
 * Copyright (C) 2012-2014,2017,2019-2020,2022-2023,2025 Aurimas Cernius
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


#include <giomm/liststore.h>
#include <gtkmm/dialog.h>
#include <gtkmm/expander.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/progressbar.h>

#include "syncui.hpp"
#include "base/monitor.hpp"


namespace gnote {
namespace sync {

  class SyncDialog
    : public Gtk::Dialog
    , public SyncUI
  {
  public:
    typedef std::shared_ptr<SyncDialog> Ptr;

    static Ptr create(IGnote &, NoteManagerBase &);

    SyncDialog(IGnote &, NoteManagerBase &);
    virtual void sync_state_changed(SyncState state) override;
    virtual void note_synchronized(const Glib::ustring & noteTitle, NoteSyncType type) override;
    virtual void note_conflict_detected(NoteBase & localConflictNote,
                                        NoteUpdate remoteNote,
                                        const std::vector<Glib::ustring> & noteUpdateTitles) override;
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
    void note_conflict_detected_(Note & localConflictNote,
                                 NoteUpdate remoteNote,
                                 const std::vector<Glib::ustring> & noteUpdateTitles,
                                 SyncTitleConflictResolution savedBehavior,
                                 SyncTitleConflictResolution resolution,
                                 CompletionMonitor &wait);
    void conflict_dialog_response(
      Gtk::Dialog *dialog,
      const Glib::ustring & localConflictNote,
      NoteUpdate remoteNote,
      SyncTitleConflictResolution savedBehavior,
      SyncTitleConflictResolution resolution,
      bool noteSyncBitsMatch,
      Gtk::ResponseType response);

    bool on_pulse_progress_bar();
    void on_row_activated(guint idx);
    void sync_state_changed_(SyncState state);
    void rename_note(const Glib::ustring & note_uri, Glib::ustring && newTitle, bool updateReferencingNotes);
    void present_note(Note &);

    Gtk::Image *m_image;
    Gtk::Label *m_header_label;
    Gtk::Label *m_message_label;
    Gtk::ProgressBar *m_progress_bar;
    Gtk::Label *m_progress_label;

    Gtk::Expander *m_expander;
    Gtk::Button *m_close_button;
    unsigned m_progress_bar_timeout_id;

    Glib::RefPtr<Gio::ListStoreBase> m_model;
  };

}
}


#endif
