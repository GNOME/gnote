/*
 * gnote
 *
 * Copyright (C) 2012-2016,2019,2021-2024 Aurimas Cernius
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




#ifndef __NOTE_ADDIN_HPP_
#define __NOTE_ADDIN_HPP_

#include <map>

#include <sigc++/connection.h>

#include "sharp/exception.hpp"
#include "abstractaddin.hpp"
#include "note.hpp"
#include "notebuffer.hpp"
#include "popoverwidgets.hpp"

namespace gnote {

/// <summary>
/// A NoteAddin extends the functionality of a note and a NoteWindow.
/// If you wish to extend Tomboy in a more broad sense, perhaps you
/// should create an ApplicationAddin.
/// <summary>
class NoteAddin
  : public AbstractAddin
{
public:
  static const char * IFACE_NAME;
  /// factory method
//  static NoteAddin *create() { return NULL; }
  void initialize(IGnote & ignote, Note::Ptr && note);

  virtual void dispose(bool) override;

  /// <summary>
  /// Called when the NoteAddin is attached to a Note
  /// </summary>
  virtual void initialize () = 0;

  /// <summary>
  /// Called when a note is deleted and also when
  /// the addin is disabled.
  /// </summary>
  virtual void shutdown () = 0;

  /// <summary>
  /// Called when the note is opened.
  /// </summary>
  virtual void on_note_opened () = 0;

  virtual void on_note_foregrounded() {}
  virtual void on_note_backgrounded() {}

  virtual std::vector<PopoverWidget> get_actions_popover_widgets() const;
  void register_main_window_action_callback(const Glib::ustring & action, sigc::slot<void(const Glib::VariantBase&)> && callback);

  Note & get_note() const
    {
      if(is_disposing() || !m_note) {
        throw sharp::Exception("Plugin is disposing already");
      }
      return *m_note;
    }
  bool has_buffer() const
    {
      return m_note->has_buffer();
    }
  const NoteBuffer::Ptr & get_buffer() const
    {
      if(is_disposing() && !has_buffer()) {
        throw sharp::Exception("Plugin is disposing already");
      }
      return m_note->get_buffer();
    }
  NoteWindow * get_window() const
    {
      if(is_disposing() && !has_buffer()) {
        throw sharp::Exception("Plugin is disposing already");
      }
      return m_note->get_window();
    }
  Gtk::Window *get_host_window() const;
  NoteManagerBase & manager() const
    {
      return m_note->manager();
    }
  void on_note_opened_event(Note & );
private:
  void on_foregrounded();
  void on_backgrounded();

  Note::Ptr                     m_note;
  sigc::connection              m_note_opened_cid;
  typedef std::pair<Glib::ustring, sigc::slot<void(const Glib::VariantBase&)>> ActionCallback;
  std::vector<ActionCallback>   m_action_callbacks;
  std::vector<sigc::connection> m_action_callbacks_cids;
};


}


#endif
