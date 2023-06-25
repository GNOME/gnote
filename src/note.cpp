 /*
 * gnote
 *
 * Copyright (C) 2010-2017,2019-2023 Aurimas Cernius
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



#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/i18n.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

#include "ignote.hpp"
#include "mainwindow.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "noterenamedialog.hpp"
#include "notetag.hpp"
#include "notewindow.hpp"
#include "utils.hpp"
#include "debug.hpp"
#include "notebooks/notebookmanager.hpp"
#include "sharp/exception.hpp"
#include "sharp/fileinfo.hpp"
#include "sharp/string.hpp"


namespace gnote {

  namespace noteutils {

    void show_deletion_dialog(const Note::List & notes, Gtk::Window * parent)
    {
      Glib::ustring message;

      if(notes.size() == 1) {
        // TRANSLATORS: %1 will be replaced by note title
        message = Glib::ustring::compose(_("Really delete \"%1\"?"), notes.front()->get_title());
      }
      else {
        // TRANSLATORS: %1 is number of notes
        message = Glib::ustring::compose(ngettext("Really delete %1 note?", "Really delete %1 notes?", notes.size()), notes.size());
      }

      utils::HIGMessageDialog dialog(parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                                     Gtk::MESSAGE_QUESTION,
                                     Gtk::BUTTONS_NONE,
                                     message,
                                     _("If you delete a note it is permanently lost."));

      Gtk::Button *button;

      button = manage(new Gtk::Button(_("_Cancel"), true));
      button->property_can_default().set_value(true);
      button->show ();
      dialog.add_action_widget(*button, Gtk::RESPONSE_CANCEL);
      dialog.set_default_response(Gtk::RESPONSE_CANCEL);

      button = manage(new Gtk::Button(_("_Delete"), true));
      button->property_can_default().set_value(true);
      button->get_style_context()->add_class("destructive-action");
      button->show ();
      dialog.add_action_widget(*button, 666);

      int result = dialog.run();
      if (result == 666) {
        for(auto & note : notes) {
          note->manager().delete_note(note);
        }
      }
    }

  }

  namespace {
    
    void show_io_error_dialog (Gtk::Window * parent)
    {
      utils::HIGMessageDialog dialog(
                              parent,
                              GTK_DIALOG_DESTROY_WITH_PARENT,
                              Gtk::MESSAGE_ERROR,
                              Gtk::BUTTONS_OK,
                              _("Error saving note data."),
                              _("An error occurred while saving your notes. "
                                "Please check that you have sufficient disk "
                                "space, and that you have appropriate rights "
                                "on ~/.local/share/gnote. Error details can be found in "
                                "~/.gnote.log."));
      dialog.run();
    }

    void place_cursor_and_selection(const NoteData & data, const Glib::RefPtr<NoteBuffer> & buffer)
    {
      Gtk::TextIter cursor;
      bool select = false;
      if(data.cursor_position() >= 0) {
        // Move cursor to last-saved position
        cursor = buffer->get_iter_at_offset(data.cursor_position());
        select = true;
      } 
      else {
        // Avoid title line
        cursor = buffer->get_iter_at_line(2);
      }
      buffer->place_cursor(cursor);

      if(select && data.selection_bound_position() >= 0) {
        // Move selection bound to last-saved position
        Gtk::TextIter selection_bound;
        selection_bound = buffer->get_iter_at_offset(data.selection_bound_position());
        buffer->move_mark(buffer->get_selection_bound(), selection_bound);
      }
    }

  }


  const int  NoteData::s_noPosition = -1;

  NoteData::NoteData(Glib::ustring && _uri)
    : m_uri(std::move(_uri))
    , m_cursor_pos(s_noPosition)
    , m_selection_bound_pos(s_noPosition)
    , m_width(0)
    , m_height(0)
  {
  }


  void NoteData::set_extent(int _width, int _height)
  {
    if (_width <= 0 || _height <= 0)
      return;

    m_width = _width;
    m_height = _height;
  }

  bool NoteData::has_extent()
  {
    return (m_width != 0) && (m_height != 0);
  }

  void NoteDataBufferSynchronizer::set_buffer(Glib::RefPtr<NoteBuffer> && b)
  {
    m_buffer = std::move(b);
    m_buffer->signal_changed().connect(sigc::mem_fun(*this, &NoteDataBufferSynchronizer::buffer_changed));
    m_buffer->signal_apply_tag()
      .connect(sigc::mem_fun(*this, &NoteDataBufferSynchronizer::buffer_tag_applied));
    m_buffer->signal_remove_tag()
      .connect(sigc::mem_fun(*this, &NoteDataBufferSynchronizer::buffer_tag_removed));

    synchronize_buffer();

    invalidate_text();
  }

  const Glib::ustring & NoteDataBufferSynchronizer::text()
  {
    synchronize_text();
    return data().text();
  }

  void NoteDataBufferSynchronizer::set_text(Glib::ustring && t)
  {
    data().text() = std::move(t);
    synchronize_buffer();
  }

  void NoteDataBufferSynchronizer::invalidate_text()
  {
    data().text() = "";
  }

  bool NoteDataBufferSynchronizer::is_text_invalid() const
  {
    return data().text().empty();
  }

  void NoteDataBufferSynchronizer::synchronize_text() const
  {
    if(is_text_invalid() && m_buffer) {
      const_cast<NoteData&>(data()).text() = NoteBufferArchiver::serialize(m_buffer);
    }
  }

  void NoteDataBufferSynchronizer::synchronize_buffer()
  {
    if(!is_text_invalid() && m_buffer) {
      // Don't create Undo actions during load
      m_buffer->undoer().freeze_undo ();

      m_buffer->erase(m_buffer->begin(), m_buffer->end());

      // Load the stored xml text
      NoteBufferArchiver::deserialize (m_buffer,
                                       m_buffer->begin(),
                                       data().text());
      m_buffer->set_modified(false);

      place_cursor_and_selection(data(), m_buffer);

      // New events should create Undo actions
      m_buffer->undoer().thaw_undo ();
    }
  }

  void NoteDataBufferSynchronizer::buffer_changed()
  {
    invalidate_text();
  }

  void NoteDataBufferSynchronizer::buffer_tag_applied(const Glib::RefPtr<Gtk::TextBuffer::Tag> & tag,
                                                      const Gtk::TextBuffer::iterator &,
                                                      const Gtk::TextBuffer::iterator &)
  {
    if(NoteTagTable::tag_is_serializable(tag)) {
      invalidate_text();
    }
  }

  void NoteDataBufferSynchronizer::buffer_tag_removed(const Glib::RefPtr<Gtk::TextBuffer::Tag> & tag,
                                                      const Gtk::TextBuffer::iterator &,
                                                      const Gtk::TextBuffer::iterator &)
  {
    if(NoteTagTable::tag_is_serializable(tag)) {
      invalidate_text();
    }
  }

  Note::Note(std::unique_ptr<NoteData> _data, Glib::ustring && filepath, NoteManager & _manager, IGnote & g)
    : NoteBase(std::move(filepath), _manager)
    , m_gnote(g)
    , m_data(std::move(_data))
    , m_save_needed(false)
    , m_is_deleting(false)
    , m_note_window_embedded(false)
    , m_focus_widget(NULL)
    , m_window(NULL)
    , m_tag_table(NULL)
  {
    for(const auto & iter : m_data.data().tags()) {
      add_tag(iter.second);
    }
    m_save_timeout = new utils::InterruptableTimeout();
    m_save_timeout->signal_timeout.connect(sigc::mem_fun(*this, &Note::on_save_timeout));
  }

  Note::~Note()
  {
    delete m_save_timeout;
    delete m_window;
  }

  Note::Ptr Note::create_new_note(Glib::ustring && title, Glib::ustring && filename, NoteManager & manager, IGnote & g)
  {
    auto note_data = std::make_unique<NoteData>(url_from_path(filename));
    note_data->title() = std::move(title);
    auto date(Glib::DateTime::create_now_local());
    note_data->create_date() = date;
    note_data->set_change_date(date);
      
    return std::make_shared<Note>(std::move(note_data), std::move(filename), manager, g);
  }

  Note::Ptr Note::create_existing_note(std::unique_ptr<NoteData> data, Glib::ustring && filepath, NoteManager & manager, IGnote & g)
  {
    if (!data->change_date()) {
      auto d(sharp::file_modification_time(filepath));
      data->set_change_date(d);
    }
    if (!data->create_date()) {
      if(data->change_date()) {
        data->create_date() = data->change_date();
      }
      else {
        auto d(sharp::file_modification_time(filepath));
        data->create_date() = d;
      }
    }
    return std::make_shared<Note>(std::move(data), std::move(filepath), manager, g);
  }

  void Note::delete_note()
  {
    m_is_deleting = true;
    m_save_timeout->cancel ();
    
    // Remove the note from all the tags
    for(NoteData::TagMap::const_iterator iter = m_data.data().tags().begin();
        iter != m_data.data().tags().end(); ++iter) {
      remove_tag(iter->second);
    }

    if (m_window) {
      if(m_window->host()) {
        m_window->host()->unembed_widget(*m_window);
      }
      delete m_window; 
      m_window = NULL;
    }
      
    // Remove note URI from GConf entry menu_pinned_notes
    set_pinned(false);
  }

  
  Note::Ptr Note::load(Glib::ustring && read_file, NoteManager & manager, IGnote & g)
  {
    auto data = std::make_unique<NoteData>(url_from_path(read_file));
    manager.note_archiver().read_file(read_file, *data);
    return create_existing_note(std::move(data), std::move(read_file), manager, g);
  }

  
  void Note::save()
  {
    // Prevent any other condition forcing a save on the note
    // if Delete has been called.
    if (m_is_deleting)
      return;
      
    // Do nothing if we don't need to save.  Avoids unneccessary saves
    // e.g on forced quit when we call save for every note.
    if (!m_save_needed)
      return;
    m_save_needed = false;

    DBG_OUT("Saving '%s'...", m_data.data().title().c_str());

    try {
      manager().note_archiver().write_file(file_path(), m_data.synchronized_data());
    } 
    catch (const sharp::Exception & e) {
      // Probably IOException or UnauthorizedAccessException?
      ERR_OUT(_("Exception while saving note: %s"), e.what());
      show_io_error_dialog(dynamic_cast<Gtk::Window*>(m_window->host()));
    }

    signal_saved(shared_from_this());
  }

  
  void Note::on_buffer_changed()
  {
    DBG_OUT("on_buffer_changed queuein save");
    queue_save(CONTENT_CHANGED);
  }

  void Note::on_buffer_tag_applied(const Glib::RefPtr<Gtk::TextTag> &tag,
                                   const Gtk::TextBuffer::iterator &, 
                                   const Gtk::TextBuffer::iterator &)
  {
    if(NoteTagTable::tag_is_serializable(tag)) {
      DBG_OUT("BufferTagApplied queueing save: %s", tag->property_name().get_value().c_str());
      queue_save(get_tag_table()->get_change_type(tag));
    }
  }

  void Note::on_buffer_tag_removed(const Glib::RefPtr<Gtk::TextTag> &tag,
                                   const Gtk::TextBuffer::iterator &,
                                   const Gtk::TextBuffer::iterator &)
  {
    if(NoteTagTable::tag_is_serializable(tag)) {
      DBG_OUT("BufferTagRemoved queueing save: %s", tag->property_name().get_value().c_str());
      queue_save(get_tag_table()->get_change_type(tag));
    }
  }

  void Note::on_buffer_mark_set(const Gtk::TextBuffer::iterator & iter,
                                const Glib::RefPtr<Gtk::TextBuffer::Mark> & insert)
  {
    auto buffer = get_buffer();
    auto insert_mark = buffer->get_insert();
    auto selection_mark = buffer->get_selection_bound();
    if(!(insert == insert_mark || insert == selection_mark)) {
      return;
    }

    Gtk::TextIter start, end;
    if(m_buffer->get_selection_bounds(start, end)) {
      m_data.data().set_cursor_position(start.get_offset());
      m_data.data().set_selection_bound_position(end.get_offset());
    }
    else {
      auto cursor_pos = insert_mark->get_iter().get_offset();
      if(m_data.data().cursor_position() == cursor_pos && m_data.data().selection_bound_position() == NoteData::s_noPosition) {
        // no change, do not queue save
        return;
      }
      m_data.data().set_cursor_position(cursor_pos);
      m_data.data().set_selection_bound_position(NoteData::s_noPosition);
    }

    DBG_OUT("OnBufferSetMark queueing save");
    queue_save(NO_CHANGE);
  }

  bool Note::on_window_destroyed(GdkEventAny * /*ev*/)
  {
    m_window = NULL;
    return false;
  }

  void Note::queue_save (ChangeType changeType)
  {
    DBG_OUT("Got QueueSave");

    // Replace the existing save timeout.  Wait 4 seconds
    // before saving...
    m_save_timeout->reset(4000);
    if (!m_is_deleting)
      m_save_needed = true;
    set_change_type(changeType);
  }

  void Note::on_save_timeout()
  {
    try {
      save();
    }
    catch(const sharp::Exception &e) 
    {
      ERR_OUT(_("Error while saving: %s"), e.what());
    }
  }

  void Note::remove_tag(Tag & tag)
  {
    Glib::ustring tag_name = tag.normalized_name();
    NoteData::TagMap & thetags(m_data.data().tags());
    NoteData::TagMap::iterator iter;

    // if we are deleting the note, no need to check for the tag, we 
    // know it is there.
    if(!m_is_deleting) {
      iter = thetags.find(tag_name);
      if (iter == thetags.end())  {
        return;
      }
    }

    signal_tag_removing(*this, tag);

    // don't erase the tag if we are deleting the note. 
    // This will invalidate the iterator.
    // see bug 579839.
    if(!m_is_deleting) {
      thetags.erase(iter);
    }
    tag.remove_note(*this);

    signal_tag_removed(shared_from_this(), tag_name);

    DBG_OUT("Tag removed, queueing save");
    queue_save(OTHER_DATA_CHANGED);
  }

  void Note::add_child_widget(Glib::RefPtr<Gtk::TextChildAnchor> && child_anchor, Gtk::Widget *widget)
  {
    m_child_widget_queue.push(ChildWidgetData(std::move(child_anchor), widget));
    if(has_window()) {
      process_child_widget_queue();
    }
  }

  void Note::process_child_widget_queue()
  {
    // Insert widgets in the childWidgetQueue into the NoteEditor
    if (!has_window())
      return; // can't do anything without a window

    while(!m_child_widget_queue.empty()) {
      ChildWidgetData & qdata(m_child_widget_queue.front());
      qdata.widget->show();
      m_window->editor()->add_child_at_anchor(*qdata.widget, qdata.anchor);
      m_child_widget_queue.pop();
    }
  }

  void Note::set_title(Glib::ustring && new_title, bool from_user_action)
  {
    if (m_data.data().title() != new_title) {
      if (m_window) {
        m_window->set_name(Glib::ustring(new_title));
      }

      Glib::ustring old_title = std::move(m_data.data().title());
      m_data.data().title() = std::move(new_title);

      if (from_user_action) {
        process_rename_link_update(old_title);
      }
      else {
        signal_renamed(shared_from_this(), old_title);
        queue_save(CONTENT_CHANGED);
      }
    }
  }


  void Note::process_rename_link_update(const Glib::ustring & old_title)
  {
    NoteBase::List linking_notes = manager().get_notes_linking_to(old_title);
    const Note::Ptr self = std::static_pointer_cast<Note>(shared_from_this());

    if (!linking_notes.empty()) {
      const NoteRenameBehavior behavior = static_cast<NoteRenameBehavior>(m_gnote.preferences().note_rename_behavior());

      if (NOTE_RENAME_ALWAYS_SHOW_DIALOG == behavior) {
        NoteRenameDialog *dlg = new NoteRenameDialog(linking_notes, old_title, self, m_gnote);
        dlg->signal_response().connect([this, dlg, old_title, self](int response) {
          process_rename_link_update_end(response, dlg, old_title, self);
        });
        dlg->present();
        get_window()->editor()->set_editable(false);
      }
      else if (NOTE_RENAME_ALWAYS_REMOVE_LINKS == behavior) {
        for(NoteBase::Ptr & iter : linking_notes) {
          iter->remove_links(old_title, self);
          process_rename_link_update_end(Gtk::RESPONSE_NO, NULL, old_title, self);
        }
      }
      else if (NOTE_RENAME_ALWAYS_RENAME_LINKS == behavior) {
        for(NoteBase::Ptr & iter : linking_notes) {
          iter->rename_links(old_title, self);
          process_rename_link_update_end(Gtk::RESPONSE_NO, NULL, old_title, self);
        }
      }
    }
    else {
      signal_renamed(shared_from_this(), old_title);
      queue_save(CONTENT_CHANGED);
    }
  }

  void Note::process_rename_link_update_end(int response, Gtk::Dialog *dialog,
                                            const Glib::ustring & old_title, const Note::Ptr & self)
  {
    if(dialog) {
      NoteRenameDialog *dlg = static_cast<NoteRenameDialog*>(dialog);
      const NoteRenameBehavior selected_behavior = dlg->get_selected_behavior();
      if(Gtk::RESPONSE_CANCEL != response && NOTE_RENAME_ALWAYS_SHOW_DIALOG != selected_behavior) {
        m_gnote.preferences().note_rename_behavior(selected_behavior);
      }

      const NoteRenameDialog::MapPtr notes = dlg->get_notes();

      for(std::map<NoteBase::Ptr, bool>::const_iterator iter = notes->begin();
          notes->end() != iter; iter++) {
        const std::pair<NoteBase::Ptr, bool> p = *iter;
        if(p.second && response == Gtk::RESPONSE_YES) { // Rename
          p.first->rename_links(old_title, self);
        }
        else {
          p.first->remove_links(old_title, self);
        }
      }
      delete dialog;
      get_window()->editor()->set_editable(true);
    }

    signal_renamed(shared_from_this(), old_title);
    queue_save(CONTENT_CHANGED);
  }


  bool Note::contains_text(const Glib::ustring & text)
  {
    const Glib::ustring text_lower = text.lowercase();
    const Glib::ustring text_content_lower = text_content().lowercase();
    return text_content_lower.find(text_lower) != Glib::ustring::npos;
  }


  void Note::handle_link_rename(const Glib::ustring & old_title,
                                const NoteBase::Ptr & renamed,
                                bool rename)
  {
    // Check again, things may have changed
    if (!contains_text(old_title))
      return;

    const Glib::ustring old_title_lower = old_title.lowercase();

    const NoteTag::Ptr link_tag = m_tag_table->get_link_tag();

    // Replace existing links with the new title.
    utils::TextTagEnumerator enumerator(get_buffer(), link_tag);
    while (enumerator.move_next()) {
      const utils::TextRange & range(enumerator.current());
      if (range.text().lowercase() != old_title_lower)
        continue;

      if (!rename) {
        DBG_OUT("Removing link tag from text %s",
                range.text().c_str());
        m_buffer->remove_tag(link_tag, range.start(), range.end());
      }
      else {
        DBG_OUT("Replacing %s with %s",
                range.text().c_str(),
                renamed->get_title().c_str());
        const Gtk::TextIter start_iter = range.start();
        const Gtk::TextIter end_iter = range.end();
        m_buffer->erase(start_iter, end_iter);
        m_buffer->insert_with_tag(range.start(),
                                  renamed->get_title(),
                                  link_tag);
      }
    }
  }

  void Note::rename_without_link_update(Glib::ustring && newTitle)
  {
    if(data_synchronizer().data().title() != newTitle) {
      if(m_window) {
        m_window->set_name(Glib::ustring(newTitle));
      }
    }
    NoteBase::rename_without_link_update(std::move(newTitle));
  }

  void Note::set_xml_content(Glib::ustring && xml)
  {
    if (m_buffer) {
      m_buffer->set_text("");
      NoteBufferArchiver::deserialize(m_buffer, std::move(xml));
    } 
    else {
      NoteBase::set_xml_content(std::move(xml));
    }
  }

  Glib::ustring Note::text_content()
  {
    if(!m_buffer) {
      return NoteBase::text_content();
    }
    return m_buffer->get_slice(m_buffer->begin(), m_buffer->end());
  }

  void Note::set_text_content(Glib::ustring && text)
  {
    if(m_buffer) {
      m_buffer->set_text(std::move(text));
    }
    else {
      ERR_OUT(_("Setting text content for closed notes not supported"));
    }
  }

  const Glib::RefPtr<NoteTagTable> & Note::get_tag_table()
  {
    if (!m_tag_table) {
      // NOTE: Sharing the same TagTable means
      // that formatting is duplicated between
      // buffers.
      m_tag_table = NoteTagTable::instance();
    }
    return m_tag_table;
  }

  const Glib::RefPtr<NoteBuffer> & Note::get_buffer()
  {
    if(!m_buffer) {
      DBG_OUT("Creating buffer for %s", m_data.data().title().c_str());
      m_buffer = NoteBuffer::create(get_tag_table(), *this, m_gnote.preferences());
      m_data.set_buffer(Glib::RefPtr<NoteBuffer>(m_buffer));

      m_buffer->signal_changed().connect(
        sigc::mem_fun(*this, &Note::on_buffer_changed));
      m_buffer->signal_apply_tag().connect(
        sigc::mem_fun(*this, &Note::on_buffer_tag_applied));
      m_buffer->signal_remove_tag().connect(
        sigc::mem_fun(*this, &Note::on_buffer_tag_removed));
      m_mark_set_conn = m_buffer->signal_mark_set().connect(
        sigc::mem_fun(*this, &Note::on_buffer_mark_set));
    }
    return m_buffer;
  }


  NoteWindow * Note::create_window()
  {
    if(!m_window) {
      m_window = new NoteWindow(*this, m_gnote);
      m_window->signal_delete_event().connect(
        sigc::mem_fun(*this, &Note::on_window_destroyed));

      m_window->editor()->set_sensitive(enabled());
      if(m_data.data().has_extent()) {
        m_window->set_size(m_data.data().width(), m_data.data().height());
      }

      m_window->signal_embedded.connect(sigc::mem_fun(*this, &Note::on_note_window_embedded));
      m_window->signal_foregrounded.connect(sigc::mem_fun(*this, &Note::on_note_window_foregrounded));
    }
    return m_window;
  }

  void Note::on_note_window_embedded()
  {
    if(!m_note_window_embedded) {
      // This is here because emiting inside
      // OnRealized causes segfaults.
      m_signal_opened(*this);

      // Add any child widgets if any exist now that
      // the window is showing.
      process_child_widget_queue();
      m_note_window_embedded = true;
    }

    m_gnote.notebook_manager().active_notes_notebook()->add_note(std::static_pointer_cast<Note>(shared_from_this()));
  }

  void Note::on_note_window_foregrounded()
  {
    m_mark_set_conn.block();
    m_mark_deleted_conn.block();

    place_cursor_and_selection(m_data.data(), m_buffer);

    m_mark_set_conn.unblock();
    m_mark_deleted_conn.unblock();
  }

  bool Note::is_special() const
  {
    return m_gnote.preferences().start_note_uri() == m_data.data().uri();
  }

  bool Note::is_pinned() const
  {
    auto pinned_uris = m_gnote.preferences().menu_pinned_notes();
    return pinned_uris.find(uri()) != Glib::ustring::npos;
  }


  void Note::set_pinned(bool pinned) const
  {
    Glib::ustring new_pinned;
    auto old_pinned = m_gnote.preferences().menu_pinned_notes();
    bool is_currently_pinned = old_pinned.find(uri()) != Glib::ustring::npos;

    if (pinned == is_currently_pinned)
      return;

    if (pinned) {
      new_pinned = uri() + " " + old_pinned;
    } 
    else {
      std::vector<Glib::ustring> pinned_split;
      sharp::string_split(pinned_split, old_pinned, " \t\n");
      for(auto pin : pinned_split) {
        if (!pin.empty() && (pin != uri())) {
          new_pinned += pin + " ";
        }
      }
    }
    m_gnote.preferences().menu_pinned_notes(new_pinned);
    m_gnote.notebook_manager().signal_note_pin_status_changed(*this, pinned);
  }

  void Note::enabled(bool is_enabled)
  {
    NoteBase::enabled(is_enabled);
    if(m_window) {
      Gtk::Window *window = dynamic_cast<Gtk::Window*>(m_window->host());
      if(window) {
        if(!enabled()) {
          m_focus_widget = window->get_focus();
        }
        m_window->host()->enabled(enabled());
        m_window->enabled(enabled());
        if(enabled() && m_focus_widget) {
          window->set_focus(*m_focus_widget);
        }
      }
    }
  }

}
