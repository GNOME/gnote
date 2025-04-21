/*
 * gnote
 *
 * Copyright (C) 2011-2015,2017,2019-2020,2022-2025 Aurimas Cernius
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




#ifndef __NOTE_HPP_
#define __NOTE_HPP_

#include <queue>

#include <gtkmm/textbuffer.h>

#include "notebase.hpp"
#include "notebuffer.hpp"
#include "utils.hpp"

namespace gnote {

class IGnote;
class NoteManager;

class NoteWindow;
class NoteTagTable;


class NoteDataBufferSynchronizer
  : public NoteDataBufferSynchronizerBase
{
public:
  NoteDataBufferSynchronizer(std::unique_ptr<NoteData> _data)
    : NoteDataBufferSynchronizerBase(std::move(_data))
    {
    }

  virtual const NoteData & synchronized_data() const override
    {
      synchronize_text();
      return data();
    }
  virtual NoteData & synchronized_data() override
    {
      synchronize_text();
      return data();
    }
  const Glib::RefPtr<NoteBuffer> & buffer() const
    {
      return m_buffer;
    }
  void set_buffer(Glib::RefPtr<NoteBuffer> && b);
  const Glib::ustring & text() const override;
  void set_text(Glib::ustring && t) override;

private:
  void invalidate_text();
  bool is_text_invalid() const;
  void synchronize_text() const;
  void synchronize_buffer();
  void buffer_changed();
  void buffer_tag_applied(const Glib::RefPtr<Gtk::TextBuffer::Tag> &,
                          const Gtk::TextBuffer::iterator &,
                          const Gtk::TextBuffer::iterator &);
  void buffer_tag_removed(const Glib::RefPtr<Gtk::TextBuffer::Tag> &,
                          const Gtk::TextBuffer::iterator &,
                          const Gtk::TextBuffer::iterator &);

  Glib::RefPtr<NoteBuffer> m_buffer;
};


class Note 
  : public NoteBase
{
public:
  typedef Glib::RefPtr<Note> Ptr;
  typedef std::reference_wrapper<Note> Ref;
  typedef std::optional<Ref> ORef;

  ~Note();


  static Note::Ptr create_new_note(Glib::ustring && title, Glib::ustring && filename, NoteManager & manager, IGnote & g);

  static Note::Ptr create_existing_note(std::unique_ptr<NoteData> data, Glib::ustring && filepath, NoteManager & manager, IGnote & g);

  virtual void delete_note() override;
  static Note::Ptr load(Glib::ustring &&, NoteManager &, IGnote &);
  virtual void save() override;
  virtual void queue_save(ChangeType c) override;
  void add_child_widget(Glib::RefPtr<Gtk::TextChildAnchor> && child_anchor, Gtk::Widget *widget);

  using NoteBase::set_title;
  void set_title(Glib::ustring && new_title, bool from_user_action) override;
  void rename_without_link_update(Glib::ustring && newTitle) override;
  void set_xml_content(Glib::ustring && xml) override;
  virtual Glib::ustring text_content() override;
  void set_text_content(Glib::ustring && text);

  const Glib::RefPtr<NoteTagTable> & get_tag_table();
  bool has_buffer() const
    {
      return (bool)m_buffer;
    }
  const Glib::RefPtr<NoteBuffer> & get_buffer();
  bool has_window() const 
    { 
      return (m_window != NULL); 
    }
  NoteWindow * get_window()
    {
      return m_window;
    }
  NoteWindow * create_window();
  bool is_special() const;
  bool is_loaded() const
    {
      return (bool)m_buffer;
    }
  bool is_opened() const
    { 
      return (m_window != NULL); 
    }
  bool is_pinned() const;
  void set_pinned(bool pinned) const;
  using NoteBase::enabled;
  virtual void enabled(bool is_enabled) override;

  sigc::signal<void(Note&)> & signal_opened()
    { return m_signal_opened; }
protected:
  virtual const NoteDataBufferSynchronizerBase & data_synchronizer() const override
  {
    return m_data;
  }
  virtual NoteDataBufferSynchronizerBase & data_synchronizer() override
  {
    return m_data;
  }
private:
  Note(std::unique_ptr<NoteData> _data, Glib::ustring && filepath, NoteManager & manager, IGnote & g);

  bool contains_text(const Glib::ustring & text);
  void handle_link_rename(const Glib::ustring & old_title, const NoteBase & renamed, bool rename) override;
  void on_buffer_changed();
  void on_buffer_tag_applied(const Glib::RefPtr<Gtk::TextTag> &tag, 
                             const Gtk::TextBuffer::iterator &, 
                             const Gtk::TextBuffer::iterator &);
  void on_buffer_tag_removed(const Glib::RefPtr<Gtk::TextTag> &tag,
                             const Gtk::TextBuffer::iterator &, 
                             const Gtk::TextBuffer::iterator &);
  void on_buffer_mark_set(const Gtk::TextBuffer::iterator & iter,
                          const Glib::RefPtr<Gtk::TextBuffer::Mark> & insert);
  void on_window_destroyed();
  void process_child_widget_queue();
  void process_rename_link_update(const Glib::ustring & old_title);
  void on_note_window_embedded();
  void on_note_window_foregrounded();

  struct ChildWidgetData
  {
    ChildWidgetData(Glib::RefPtr<Gtk::TextChildAnchor> && _anchor, Gtk::Widget *_widget)
      : anchor(std::move(_anchor))
      , widget(_widget)
      {
      }
    Glib::RefPtr<Gtk::TextChildAnchor> anchor;
    Gtk::Widget *widget;
  };

  IGnote &                   m_gnote;
  NoteDataBufferSynchronizer m_data;
  bool                       m_save_needed;
  bool                       m_is_deleting;
  bool                       m_note_window_embedded;

  Gtk::Widget               *m_focus_widget;
  NoteWindow                *m_window;
  Glib::RefPtr<NoteBuffer>   m_buffer;
  Glib::RefPtr<NoteTagTable> m_tag_table;

  std::queue<ChildWidgetData> m_child_widget_queue;

  sigc::signal<void(Note&)> m_signal_opened;

  sigc::connection m_mark_set_conn;
  sigc::connection m_mark_deleted_conn;
};

namespace noteutils {
  void show_deletion_dialog(const std::vector<NoteBase::Ref> & notes, Gtk::Window & parent);
}

}

#endif
