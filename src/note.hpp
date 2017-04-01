/*
 * gnote
 *
 * Copyright (C) 2011-2015,2017 Aurimas Cernius
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

#include <list>
#include <queue>

#include <gtkmm/textbuffer.h>

#include "notebase.hpp"
#include "notebuffer.hpp"
#include "utils.hpp"

namespace gnote {

class NoteManager;

class NoteWindow;
class NoteTagTable;


class NoteDataBufferSynchronizer
  : public NoteDataBufferSynchronizerBase
{
public:
  // takes ownership
  NoteDataBufferSynchronizer(NoteData * _data)
    : NoteDataBufferSynchronizerBase(_data)
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
  void set_buffer(const Glib::RefPtr<NoteBuffer> & b);
  virtual const Glib::ustring & text() override;
  virtual void set_text(const Glib::ustring & t) override;

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
  typedef shared_ptr<Note> Ptr;
  typedef weak_ptr<Note> WeakPtr;
  typedef std::list<Ptr> List;

  ~Note();


  static Note::Ptr create_new_note(const Glib::ustring & title,
                                   const Glib::ustring & filename,
                                   NoteManager & manager);

  static Note::Ptr create_existing_note(NoteData *data,
                                        Glib::ustring filepath,
                                        NoteManager & manager);
  virtual void delete_note() override;
  static Note::Ptr load(const Glib::ustring &, NoteManager &);
  virtual void save() override;
  virtual void queue_save(ChangeType c) override;
  using NoteBase::remove_tag;
  virtual void remove_tag(Tag &) override;
  void add_child_widget(const Glib::RefPtr<Gtk::TextChildAnchor> & child_anchor,
                        Gtk::Widget * widget);

  using NoteBase::set_title;
  virtual void set_title(const Glib::ustring & new_title, bool from_user_action) override;
  virtual void rename_without_link_update(const Glib::ustring & newTitle) override;
  virtual void set_xml_content(const Glib::ustring & xml) override;
  Glib::ustring text_content();
  void set_text_content(const Glib::ustring & text);

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

  sigc::signal<void,Note&> & signal_opened()
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
  bool contains_text(const Glib::ustring & text);
  virtual void handle_link_rename(const Glib::ustring & old_title,
                                  const NoteBase::Ptr & renamed, bool rename) override;
  void on_buffer_changed();
  void on_buffer_tag_applied(const Glib::RefPtr<Gtk::TextTag> &tag, 
                             const Gtk::TextBuffer::iterator &, 
                             const Gtk::TextBuffer::iterator &);
  void on_buffer_tag_removed(const Glib::RefPtr<Gtk::TextTag> &tag,
                             const Gtk::TextBuffer::iterator &, 
                             const Gtk::TextBuffer::iterator &);
  void on_buffer_mark_set(const Gtk::TextBuffer::iterator & iter,
                          const Glib::RefPtr<Gtk::TextBuffer::Mark> & insert);
  void on_buffer_mark_deleted(const Glib::RefPtr<Gtk::TextBuffer::Mark> & mark);
  bool on_window_destroyed(GdkEventAny *ev);
  void on_save_timeout();
  void process_child_widget_queue();
  void process_rename_link_update(const Glib::ustring & old_title);
  void process_rename_link_update_end(int response, Gtk::Dialog *dialog,
                                      const Glib::ustring & old_title, const Note::Ptr & self);
  void on_note_window_embedded();
  void on_note_window_foregrounded();

  Note(NoteData * data, const Glib::ustring & filepath, NoteManager & manager);

  struct ChildWidgetData
  {
    ChildWidgetData(const Glib::RefPtr<Gtk::TextChildAnchor> & _anchor,
                    Gtk::Widget *_widget)
      : anchor(_anchor)
      , widget(_widget)
      {
      }
    Glib::RefPtr<Gtk::TextChildAnchor> anchor;
    Gtk::Widget *widget;
  };

  NoteDataBufferSynchronizer m_data;
  bool                       m_save_needed;
  bool                       m_is_deleting;
  bool                       m_note_window_embedded;

  Gtk::Widget               *m_focus_widget;
  NoteWindow                *m_window;
  Glib::RefPtr<NoteBuffer>   m_buffer;
  Glib::RefPtr<NoteTagTable> m_tag_table;

  utils::InterruptableTimeout *m_save_timeout;
  std::queue<ChildWidgetData> m_child_widget_queue;

  sigc::signal<void,Note&> m_signal_opened;

  sigc::connection m_mark_set_conn;
  sigc::connection m_mark_deleted_conn;
};

namespace noteutils {
  void show_deletion_dialog (const std::list<Note::Ptr> & notes, Gtk::Window * parent);
}

}

#endif
