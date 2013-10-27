 /*
 * gnote
 *
 * Copyright (C) 2010-2013 Aurimas Cernius
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

#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string/find.hpp>

#include <libxml/parser.h>

#include <glibmm/i18n.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

#include "mainwindow.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "noterenamedialog.hpp"
#include "notetag.hpp"
#include "notewindow.hpp"
#include "itagmanager.hpp"
#include "utils.hpp"
#include "debug.hpp"
#include "notebooks/notebookmanager.hpp"
#include "sharp/exception.hpp"
#include "sharp/fileinfo.hpp"
#include "sharp/files.hpp"
#include "sharp/map.hpp"
#include "sharp/string.hpp"
#include "sharp/xml.hpp"
#include "sharp/xmlconvert.hpp"
#include "sharp/xmlreader.hpp"
#include "sharp/xmlwriter.hpp"

#if HAVE_CXX11
  #include <functional>
  using std::hash;
#else
  #include <tr1/functional>
  using std::tr1::hash;
#endif


namespace gnote {

  namespace noteutils {

    void show_deletion_dialog (const std::list<Note::Ptr> & notes, Gtk::Window * parent)
    {
      std::string message;

      if(notes.size() == 1) {
        // TRANSLATORS: %1% will be replaced by note title
        message = str(boost::format(_("Really delete \"%1%\"?")) % notes.front()->get_title());
      }
      else {
        // TRANSLATORS: %1% is number of notes
        message = str(boost::format(ngettext("Really delete %1% note?", "Really delete %1% notes?", notes.size())) % notes.size());
      }

      utils::HIGMessageDialog dialog(parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                                     Gtk::MESSAGE_QUESTION,
                                     Gtk::BUTTONS_NONE,
                                     message,
                                     _("If you delete a note it is permanently lost."));

      Gtk::Button *button;

      button = manage(new Gtk::Button(Gtk::Stock::CANCEL));
      button->property_can_default().set_value(true);
      button->show ();
      dialog.add_action_widget(*button, Gtk::RESPONSE_CANCEL);
      dialog.set_default_response(Gtk::RESPONSE_CANCEL);

      button = manage(new Gtk::Button (Gtk::Stock::DELETE));
      button->property_can_default().set_value(true);
      button->show ();
      dialog.add_action_widget(*button, 666);

      int result = dialog.run();
      if (result == 666) {
        for(Note::List::const_iterator iter = notes.begin();
            iter != notes.end(); ++iter) {
          const Note::Ptr & note(*iter);
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

  NoteData::NoteData(const std::string & _uri)
    : m_uri(_uri)
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

  NoteDataBufferSynchronizer::~NoteDataBufferSynchronizer()
  {
    delete m_data;
  }

  void NoteDataBufferSynchronizer::set_buffer(const Glib::RefPtr<NoteBuffer> & b)
  {
    m_buffer = b;
    m_buffer->signal_changed().connect(sigc::mem_fun(*this, &NoteDataBufferSynchronizer::buffer_changed));
    m_buffer->signal_apply_tag()
      .connect(sigc::mem_fun(*this, &NoteDataBufferSynchronizer::buffer_tag_applied));
    m_buffer->signal_remove_tag()
      .connect(sigc::mem_fun(*this, &NoteDataBufferSynchronizer::buffer_tag_removed));

    synchronize_buffer();

    invalidate_text();
  }

  const std::string & NoteDataBufferSynchronizer::text()
  {
    synchronize_text();
    return m_data->text();
  }

  void NoteDataBufferSynchronizer::set_text(const std::string & t)
  {
    m_data->text() = t;
    synchronize_buffer();
  }

  void NoteDataBufferSynchronizer::invalidate_text()
  {
    m_data->text() = "";
  }

  bool NoteDataBufferSynchronizer::is_text_invalid() const
  {
    return m_data->text().empty();
  }

  void NoteDataBufferSynchronizer::synchronize_text() const
  {
    if(is_text_invalid() && m_buffer) {
      m_data->text() = NoteBufferArchiver::serialize(m_buffer);
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
                                       m_data->text());
      m_buffer->set_modified(false);

      place_cursor_and_selection(*m_data, m_buffer);

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

  Note::Note(NoteData * _data, const std::string & filepath, NoteManager & _manager)
    : m_data(_data)
    , m_filepath(filepath)
    , m_save_needed(false)
    , m_is_deleting(false)
    , m_enabled(true)
    , m_note_window_embedded(false)
    , m_focus_widget(NULL)
    , m_manager(_manager)
    , m_window(NULL)
    , m_tag_table(NULL)
  {
    for(NoteData::TagMap::const_iterator iter = _data->tags().begin();
        iter != _data->tags().end(); ++iter) {
      add_tag(iter->second);
    }
    m_save_timeout = new utils::InterruptableTimeout();
    m_save_timeout->signal_timeout.connect(sigc::mem_fun(*this, &Note::on_save_timeout));
  }

  Note::~Note()
  {
    delete m_save_timeout;
    delete m_window;
  }

  /// <summary>
  /// Returns a Tomboy URL from the given path.
  /// </summary>
  /// <param name="filepath">
  /// A <see cref="System.String"/>
  /// </param>
  /// <returns>
  /// A <see cref="System.String"/>
  /// </returns>
  std::string Note::url_from_path(const std::string & filepath)
  {
    return "note://gnote/" + sharp::file_basename(filepath);
  }

  int Note::get_hash_code() const
  {
    hash<std::string> h;
    return h(get_title());
  }

  /// <summary>
  /// Creates a New Note with the given values.
  /// </summary>
  /// <param name="title">
  /// A <see cref="System.String"/>
  /// </param>
  /// <param name="filepath">
  /// A <see cref="System.String"/>
  /// </param>
  /// <param name="manager">
  /// A <see cref="NoteManager"/>
  /// </param>
  /// <returns>
  /// A <see cref="Note"/>
  /// </returns>
  Note::Ptr Note::create_new_note(const std::string & title,
                                  const std::string & filename,
                                  NoteManager & manager)
  {
    NoteData * note_data = new NoteData(url_from_path(filename));
    note_data->title() = title;
    sharp::DateTime date(sharp::DateTime::now());
    note_data->create_date() = date;
    note_data->set_change_date(date);
      
    return Note::Ptr(new Note(note_data, filename, manager));
  }

  Note::Ptr Note::create_existing_note(NoteData *data,
                                 std::string filepath,
                                 NoteManager & manager)
  {
    if (!data->change_date().is_valid()) {
      sharp::DateTime d(sharp::file_modification_time(filepath));
      data->set_change_date(d);
    }
    if (!data->create_date().is_valid()) {
      if(data->change_date().is_valid()) {
        data->create_date() = data->change_date();
      }
      else {
        sharp::DateTime d(sharp::file_modification_time(filepath));
        data->create_date() = d;
      }
    }
    return Note::Ptr(new Note(data, filepath, manager));
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
        MainWindow *win = dynamic_cast<MainWindow*>(m_window->host());
        bool close_host = win ? win->close_on_escape() : false;
        m_window->host()->unembed_widget(*m_window);
        if(close_host) {
          win->close_window();
        }
      }
      delete m_window; 
      m_window = NULL;
    }
      
    // Remove note URI from GConf entry menu_pinned_notes
    set_pinned(false);
  }

  
  Note::Ptr Note::load(const std::string & read_file, NoteManager & manager)
  {
    NoteData *data = NoteArchiver::read(read_file, url_from_path(read_file));
    return create_existing_note (data, read_file, manager);
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

    DBG_OUT("Saving '%s'...", m_data.data().title().c_str());

    try {
      NoteArchiver::write(m_filepath, m_data.synchronized_data());
    } 
    catch (const sharp::Exception & e) {
      // Probably IOException or UnauthorizedAccessException?
      ERR_OUT(_("Exception while saving note: %s"), e.what());
      show_io_error_dialog(dynamic_cast<Gtk::Window*>(m_window->host()));
    }

    m_signal_saved(shared_from_this());
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
    Gtk::TextIter start, end;
    if(m_buffer->get_selection_bounds(start, end)) {
      m_data.data().set_cursor_position(start.get_offset());
      m_data.data().set_selection_bound_position(end.get_offset());
    }
    else if(insert->get_name() == "insert") {
      m_data.data().set_cursor_position(iter.get_offset());
    }
    else {
      return;
    }

    DBG_OUT("OnBufferSetMark queueing save");
    queue_save(NO_CHANGE);
  }

  void Note::on_buffer_mark_deleted(const Glib::RefPtr<Gtk::TextBuffer::Mark> &)
  {
    Gtk::TextIter start, end;
    if(m_data.data().selection_bound_position() != m_data.data().cursor_position()
       && !m_buffer->get_selection_bounds(start, end)) {
      DBG_OUT("selection removed");
      m_data.data().set_cursor_position(m_buffer->get_insert()->get_iter().get_offset());
      m_data.data().set_selection_bound_position(NoteData::s_noPosition);
      queue_save(NO_CHANGE);
    }
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
      
    switch (changeType)
    {
    case CONTENT_CHANGED:
      // NOTE: Updating ChangeDate automatically updates MetdataChangeDate to match.
      m_data.data().set_change_date(sharp::DateTime::now());
      break;
    case OTHER_DATA_CHANGED:
      // Only update MetadataChangeDate.  Used by sync/etc
      // to know when non-content note data has changed,
      // but order of notes in menu and search UI is
      // unaffected.
      m_data.data().metadata_change_date() = sharp::DateTime::now();
      break;
    default:
      break;
    }
  }

  void Note::on_save_timeout()
  {
    try {
      save();
      m_save_needed = false;
    }
    catch(const sharp::Exception &e) 
    {
      ERR_OUT(_("Error while saving: %s"), e.what());
    }
  }

  void Note::add_tag(const Tag::Ptr & tag)
  {
    if(!tag) {
      throw sharp::Exception ("note::add_tag() called with a NULL tag.");
    }
    tag->add_note (*this);

    NoteData::TagMap & thetags(m_data.data().tags());
    if (thetags.find(tag->normalized_name()) == thetags.end()) {
      thetags[tag->normalized_name()] = tag;

      m_signal_tag_added(*this, tag);

      DBG_OUT ("Tag added, queueing save");
      queue_save(OTHER_DATA_CHANGED);
    }
  }

  void Note::remove_tag(Tag & tag)
  {
    std::string tag_name = tag.normalized_name();
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

    m_signal_tag_removing(*this, tag);

    // don't erase the tag if we are deleting the note. 
    // This will invalidate the iterator.
    // see bug 579839.
    if(!m_is_deleting) {
      thetags.erase(iter);
    }
    tag.remove_note(*this);

    m_signal_tag_removed(shared_from_this(), tag_name);

    DBG_OUT("Tag removed, queueing save");
    queue_save(OTHER_DATA_CHANGED);
  }


  void Note::remove_tag(const Tag::Ptr & tag)
  {
    if (!tag)
      throw sharp::Exception ("Note.RemoveTag () called with a null tag.");
    remove_tag(*tag);
  }
    
  bool Note::contains_tag(const Tag::Ptr & tag) const
  {
    if(!tag) {
      return false;
    }
    const NoteData::TagMap & thetags(m_data.data().tags());
    return (thetags.find(tag->normalized_name()) != thetags.end());
  }

  void Note::add_child_widget(const Glib::RefPtr<Gtk::TextChildAnchor> & child_anchor,
                              Gtk::Widget * widget)
  {
    m_child_widget_queue.push(ChildWidgetData(child_anchor, widget));
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

  const std::string & Note::uri() const
  {
    return m_data.data().uri();
  }

  const std::string Note::id() const
  {
    // TODO: Store on Note instantiation
    return sharp::string_replace_first(m_data.data().uri(), "note://gnote/","");
  }


  const std::string & Note::get_title() const
  {
    return m_data.data().title();
  }


  void Note::set_title(const std::string & new_title)
  {
    set_title(new_title, false);
  }


  void Note::set_title(const std::string & new_title,
                       bool from_user_action)
  {
    if (m_data.data().title() != new_title) {
      if (m_window) {
        m_window->set_name(new_title);
      }

      std::string old_title = m_data.data().title();
      m_data.data().title() = new_title;

      if (from_user_action) {
        process_rename_link_update(old_title);
      }
      else {
        m_signal_renamed(shared_from_this(), old_title);
        queue_save(CONTENT_CHANGED);
      }
    }
  }


  void Note::process_rename_link_update(const std::string & old_title)
  {
    Note::List linking_notes;
    const Note::List & manager_notes = m_manager.get_notes();
    const Note::Ptr self = shared_from_this();

    for (Note::List::const_iterator iter = manager_notes.begin();
         manager_notes.end() != iter;
         iter++) {
      // Technically, containing text does not imply linking,
      // but this is less work
      const Note::Ptr note = *iter;
      if (note != self && note->contains_text(old_title))
        linking_notes.push_back(note);
    }

    if (!linking_notes.empty()) {
      Glib::RefPtr<Gio::Settings> settings = Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE);
      const NoteRenameBehavior behavior
        = static_cast<NoteRenameBehavior>(settings->get_int(Preferences::NOTE_RENAME_BEHAVIOR));

      if (NOTE_RENAME_ALWAYS_SHOW_DIALOG == behavior) {
        NoteRenameDialog *dlg = new NoteRenameDialog(linking_notes, old_title, self);
        dlg->signal_response().connect(
          boost::bind(sigc::mem_fun(*this, &Note::process_rename_link_update_end), _1, dlg, old_title, self));
        dlg->present();
        get_window()->editor()->set_editable(false);
      }
      else if (NOTE_RENAME_ALWAYS_REMOVE_LINKS == behavior) {
        for (Note::List::const_iterator iter = linking_notes.begin();
             linking_notes.end() != iter;
             iter++) {
          (*iter)->remove_links(old_title, self);
          process_rename_link_update_end(Gtk::RESPONSE_NO, NULL, old_title, self);
        }
      }
      else if (NOTE_RENAME_ALWAYS_RENAME_LINKS == behavior) {
        for (Note::List::const_iterator iter = linking_notes.begin();
             linking_notes.end() != iter;
             iter++) {
          (*iter)->rename_links(old_title, self);
          process_rename_link_update_end(Gtk::RESPONSE_NO, NULL, old_title, self);
        }
      }
    }
  }

  void Note::process_rename_link_update_end(int response, Gtk::Dialog *dialog,
                                            const std::string & old_title, const Note::Ptr & self)
  {
    if(dialog) {
      NoteRenameDialog *dlg = static_cast<NoteRenameDialog*>(dialog);
      const NoteRenameBehavior selected_behavior = dlg->get_selected_behavior();
      if(Gtk::RESPONSE_CANCEL != response && NOTE_RENAME_ALWAYS_SHOW_DIALOG != selected_behavior) {
        Glib::RefPtr<Gio::Settings> settings = Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE);
        settings->set_int(Preferences::NOTE_RENAME_BEHAVIOR, selected_behavior);
      }

      const NoteRenameDialog::MapPtr notes = dlg->get_notes();

      for(std::map<Note::Ptr, bool>::const_iterator iter = notes->begin();
          notes->end() != iter; iter++) {
        const std::pair<Note::Ptr, bool> p = *iter;
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

    m_signal_renamed(shared_from_this(), old_title);
    queue_save(CONTENT_CHANGED);
  }


  bool Note::contains_text(const std::string & text)
  {
    const std::string text_lower = sharp::string_to_lower(text);
    const std::string text_content_lower
                        = sharp::string_to_lower(text_content());
    return sharp::string_index_of(text_content_lower, text_lower) > -1;
  }


  void Note::rename_links(const std::string & old_title,
                          const Ptr & renamed)
  {
    handle_link_rename(old_title, renamed, true);
  }


  void Note::remove_links(const std::string & old_title,
                          const Ptr & renamed)
  {
    handle_link_rename(old_title, renamed, false);
  }


  void Note::handle_link_rename(const std::string & old_title,
                                const Ptr & renamed,
                                bool rename)
  {
    // Check again, things may have changed
    if (!contains_text(old_title))
      return;

    const std::string old_title_lower
                        = sharp::string_to_lower(old_title);

    const NoteTag::Ptr link_tag = m_tag_table->get_link_tag();

    // Replace existing links with the new title.
    utils::TextTagEnumerator enumerator(m_buffer, link_tag);
    while (enumerator.move_next()) {
      const utils::TextRange & range(enumerator.current());
      if (sharp::string_to_lower(range.text()) != old_title_lower)
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


  void Note::rename_without_link_update(const std::string & newTitle)
  {
    if (m_data.data().title() != newTitle) {
      if (m_window) {
        m_window->set_name(newTitle);
      }

      m_data.data().title() = newTitle;

      // HACK:
      m_signal_renamed(shared_from_this(), newTitle);

      queue_save(CONTENT_CHANGED); // TODO: Right place for this?
    }
  }

  void Note::set_xml_content(const std::string & xml)
  {
    if (m_buffer) {
      m_buffer->set_text("");
      NoteBufferArchiver::deserialize(m_buffer, xml);
    } 
    else {
      m_data.set_text(xml);
    }
  }

  std::string Note::get_complete_note_xml()
  {
    return NoteArchiver::write_string(m_data.synchronized_data());
  }

  void Note::load_foreign_note_xml(const std::string & foreignNoteXml, ChangeType changeType)
  {
    if (foreignNoteXml.empty())
      throw sharp::Exception ("foreignNoteXml");

    // Arguments to this method cannot be trusted.  If this method
    // were to throw an XmlException in the middle of processing,
    // a note could be damaged.  Therefore, we check for parseability
    // ahead of time, and throw early.
    xmlDocPtr doc = xmlParseDoc((const xmlChar *)foreignNoteXml.c_str());

    if(!doc) {
      throw sharp::Exception("invalid XML in foreignNoteXml");
    }
    xmlFreeDoc(doc);

    sharp::XmlReader xml;
    xml.load_buffer(foreignNoteXml);

    // Remove tags now, since a note with no tags has
    // no "tags" element in the XML
    std::list<Tag::Ptr> new_tags;
    std::string name;

    while (xml.read()) {
      switch (xml.get_node_type()) {
      case XML_READER_TYPE_ELEMENT:
        name = xml.get_name();
        if (name == "title") {
          set_title(xml.read_string());
        }
        else if (name == "text") {
          set_xml_content(xml.read_inner_xml());
        }
        else if (name == "last-change-date") {
          m_data.data().set_change_date(
            sharp::XmlConvert::to_date_time(xml.read_string()));
        }
        else if(name == "last-metadata-change-date") {
          m_data.data().metadata_change_date() =
            sharp::XmlConvert::to_date_time(xml.read_string());
        }
        else if(name == "create-date") {
          m_data.data().create_date() =
            sharp::XmlConvert::to_date_time(xml.read_string ());
        }
        else if(name == "tags") {
          xmlDocPtr doc2 = xmlParseDoc((const xmlChar*)xml.read_outer_xml().c_str());
          if(doc2) {
            std::list<std::string> tag_strings;
            parse_tags (doc2->children, tag_strings);
            for(std::list<std::string>::const_iterator iter = tag_strings.begin();
                iter != tag_strings.end(); ++iter) {
              Tag::Ptr tag = ITagManager::obj().get_or_create_tag(*iter);
              new_tags.push_back(tag);
            }
            xmlFreeDoc(doc2);
          }
          else {
            DBG_OUT("loading tag subtree failed");
          }
        }
        break;
      default:
        break;
      }
    }

    xml.close ();

    std::list<Tag::Ptr> tag_list;
    get_tags(tag_list);
    
    for(std::list<Tag::Ptr>::const_iterator iter = tag_list.begin();
        iter != tag_list.end(); ++iter) {
      if(find(new_tags.begin(), new_tags.end(), *iter) == new_tags.end()) {
        remove_tag(*iter);
      }
    }
    for(std::list<Tag::Ptr>::const_iterator iter = new_tags.begin();
        iter != new_tags.end(); ++iter) {
      add_tag(*iter);
    }
    
    // Allow method caller to specify ChangeType (mostly needed by sync)
    queue_save (changeType);
  }


  void Note::parse_tags(const xmlNodePtr tagnodes, std::list<std::string> & tags)
  {
    sharp::XmlNodeSet nodes = sharp::xml_node_xpath_find(tagnodes, "//*");
    
    if(nodes.empty()) {
      return;
    }
    for(sharp::XmlNodeSet::const_iterator iter = nodes.begin();
        iter != nodes.end(); ++iter) {

      const xmlNodePtr node = *iter;
      if(xmlStrEqual(node->name, (const xmlChar*)"tag") && (node->type == XML_ELEMENT_NODE)) {
        xmlChar * content = xmlNodeGetContent(node);
        if(content) {
          DBG_OUT("found tag %s", content);
          tags.push_back((const char*)content);
          xmlFree(content);
        }
      }
    }
  }

  std::string Note::text_content()
  {
    if(!m_buffer) {
      get_buffer();
    }
    return m_buffer->get_slice(m_buffer->begin(), m_buffer->end());
  }

  void Note::set_text_content(const std::string & text)
  {
    if(m_buffer) {
      m_buffer->set_text(text);
    }
    else {
      ERR_OUT(_("Setting text content for closed notes not supported"));
    }
  }

  const NoteData & Note::data() const
  {
    return m_data.synchronized_data();
  }

  NoteData & Note::data()
  {
    return m_data.synchronized_data();
  }

  const sharp::DateTime & Note::create_date() const
  {
    return m_data.data().create_date();
  }

  const sharp::DateTime & Note::change_date() const
  {
    return m_data.data().change_date();
  }

  const sharp::DateTime & Note::metadata_change_date() const
  {
    return m_data.data().metadata_change_date();
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
      m_buffer = NoteBuffer::create(get_tag_table(), *this);
      m_data.set_buffer(m_buffer);

      m_buffer->signal_changed().connect(
        sigc::mem_fun(*this, &Note::on_buffer_changed));
      m_buffer->signal_apply_tag().connect(
        sigc::mem_fun(*this, &Note::on_buffer_tag_applied));
      m_buffer->signal_remove_tag().connect(
        sigc::mem_fun(*this, &Note::on_buffer_tag_removed));
      m_mark_set_conn = m_buffer->signal_mark_set().connect(
        sigc::mem_fun(*this, &Note::on_buffer_mark_set));
      m_mark_deleted_conn = m_buffer->signal_mark_deleted().connect(
        sigc::mem_fun(*this, &Note::on_buffer_mark_deleted));
    }
    return m_buffer;
  }


  NoteWindow * Note::get_window()
  {
    if(!m_window) {
      m_window = new NoteWindow(*this);
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

    notebooks::NotebookManager::obj().active_notes_notebook()->add_note(shared_from_this());
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
    return (m_manager.start_note_uri() == m_data.data().uri());
  }


  bool Note::is_new() const
  {
    return m_data.data().create_date().is_valid() && (m_data.data().create_date() > sharp::DateTime::now().add_hours(-24));
  }

  bool Note::is_pinned() const
  {
    std::string pinned_uris = Preferences::obj()
      .get_schema_settings(Preferences::SCHEMA_GNOTE)->get_string(Preferences::MENU_PINNED_NOTES);
    return (boost::find_first(pinned_uris, uri()));
  }


  void Note::set_pinned(bool pinned) const
  {
    std::string new_pinned;
    Glib::RefPtr<Gio::Settings> settings = Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE);
    std::string old_pinned = settings->get_string(Preferences::MENU_PINNED_NOTES);
    bool is_currently_pinned = (boost::find_first(old_pinned, uri()));

    if (pinned == is_currently_pinned)
      return;

    if (pinned) {
      new_pinned = uri() + " " + old_pinned;
    } 
    else {
      std::vector<std::string> pinned_split;
      sharp::string_split(pinned_split, old_pinned, " \t\n");
      for(std::vector<std::string>::const_iterator iter = pinned_split.begin();
          iter != pinned_split.end(); ++iter) {
        const std::string & pin(*iter);
        if (!pin.empty() && (pin != uri())) {
          new_pinned += pin + " ";
        }
      }
    }
    settings->set_string(Preferences::MENU_PINNED_NOTES, new_pinned);
    notebooks::NotebookManager::obj().signal_note_pin_status_changed(*this, pinned);
  }

  void Note::get_tags(std::list<Tag::Ptr> & l) const
  {
    sharp::map_get_values(m_data.data().tags(), l);
  }

  void Note::enabled(bool is_enabled)
  {
    m_enabled = is_enabled;
    if(m_window) {
      Gtk::Window *window = dynamic_cast<Gtk::Window*>(m_window->host());
      if(window) {
        if(!m_enabled) {
          m_focus_widget = window->get_focus();
        }
        m_window->enabled(m_enabled);
        if(m_enabled) {
          window->set_focus(*m_focus_widget);
        }
      }
    }
  }

  const char *NoteArchiver::CURRENT_VERSION = "0.3";
//  const char *NoteArchiver::DATE_TIME_FORMAT = "%Y-%m-%dT%T.@7f@%z"; //"yyyy-MM-ddTHH:mm:ss.fffffffzzz";

  //instance
  NoteArchiver NoteArchiver::s_obj;


  NoteData *NoteArchiver::read(const std::string & read_file, const std::string & uri)
  {
    return obj().read_file(read_file, uri);
  }


  NoteData *NoteArchiver::read_file(const std::string & file, const std::string & uri)
  {
    std::string version;
    sharp::XmlReader xml(file);
    NoteData *data = _read(xml, uri, version);
    if(version != NoteArchiver::CURRENT_VERSION) {
      try {
        // Note has old format, so rewrite it.  No need
        // to reread, since we are not adding anything.
        DBG_OUT("Updating note XML from %s to newest format...", version.c_str());
        NoteArchiver::write(file, *data);
      }
      catch(sharp::Exception & e) {
        // write failure, but not critical
        ERR_OUT(_("Failed to update note format: %s"), e.what());
      }
    }
    return data;
  }


  NoteData *NoteArchiver::read(sharp::XmlReader & xml, const std::string & uri)
  {
    std::string version; // discarded
    return _read(xml, uri, version);
  }


  NoteData *NoteArchiver::_read(sharp::XmlReader & xml, const std::string & uri, std::string & version)
  {
    NoteData *note = new NoteData(uri);

    std::string name;

    while (xml.read ()) {
      switch (xml.get_node_type()) {
      case XML_READER_TYPE_ELEMENT:
        name = xml.get_name();
        
        if(name == "note") {
          version = xml.get_attribute("version");
        }
        else if(name == "title") {
          note->title() = xml.read_string();
        } 
        else if(name == "text") {
          // <text> is just a wrapper around <note-content>
          // NOTE: Use .text here to avoid triggering a save.
          note->text() = xml.read_inner_xml();
        }
        else if(name == "last-change-date") {
          note->set_change_date(
            sharp::XmlConvert::to_date_time (xml.read_string()));
        }
        else if(name == "last-metadata-change-date") {
          note->metadata_change_date() =
            sharp::XmlConvert::to_date_time(xml.read_string());
        }
        else if(name == "create-date") {
          note->create_date() =
            sharp::XmlConvert::to_date_time (xml.read_string());
        }
        else if(name == "cursor-position") {
          note->set_cursor_position(STRING_TO_INT(xml.read_string()));
        }
        else if(name == "selection-bound-position") {
          note->set_selection_bound_position(STRING_TO_INT(xml.read_string()));
        }
        else if(name == "width") {
          note->width() = STRING_TO_INT(xml.read_string());
        }
        else if(name == "height") {
          note->height() = STRING_TO_INT(xml.read_string());
        }
        else if(name == "tags") {
          xmlDocPtr doc2 = xmlParseDoc((const xmlChar*)xml.read_outer_xml().c_str());

          if(doc2) {
            std::list<std::string> tag_strings;
            Note::parse_tags(doc2->children, tag_strings);
            for(std::list<std::string>::const_iterator iter = tag_strings.begin();
                iter != tag_strings.end(); ++iter) {
              Tag::Ptr tag = ITagManager::obj().get_or_create_tag(*iter);
              note->tags()[tag->normalized_name()] = tag;
            }
            xmlFreeDoc(doc2);
          }
          else {
            DBG_OUT("loading tag subtree failed");
          }
        }
        break;

      default:
        break;
      }
    }
    xml.close ();

    return note;
  }

  std::string NoteArchiver::write_string(const NoteData & note)
  {
    std::string str;
    sharp::XmlWriter xml;
    obj().write(xml, note);
    xml.close();
    str = xml.to_string();
    return str;
  }
  

  void NoteArchiver::write(const std::string & write_file, const NoteData & data)
  {
    obj().write_file(write_file, data);
  }

  void NoteArchiver::write_file(const std::string & _write_file, const NoteData & note)
  {
    try {
      std::string tmp_file = _write_file + ".tmp";
      // TODO Xml doc settings
      sharp::XmlWriter xml(tmp_file); //, XmlEncoder::DocumentSettings);
      write(xml, note);
      xml.close();

      if (sharp::file_exists(_write_file)) {
        std::string backup_path = _write_file + "~";
        if (sharp::file_exists(backup_path)) {
          sharp::file_delete(backup_path);
        }
      
        // Backup the to a ~ file, just in case
        sharp::file_move(_write_file, backup_path);
      
        // Move the temp file to write_file
        sharp::file_move(tmp_file, _write_file);

        // Delete the ~ file
        sharp::file_delete(backup_path);
      } 
      else {
        // Move the temp file to write_file
        sharp::file_move(tmp_file, _write_file);
      }
    }
    catch(const std::exception & e)
    {
      ERR_OUT(_("Filesystem error: %s"), e.what());
    }
  }

  void NoteArchiver::write(sharp::XmlWriter & xml, const NoteData & note)
  {
    xml.write_start_document();
    xml.write_start_element("", "note", "http://beatniksoftware.com/tomboy");
    xml.write_attribute_string("",
                             "version",
                             "",
                             CURRENT_VERSION);
    xml.write_attribute_string("xmlns",
                             "link",
                             "",
                             "http://beatniksoftware.com/tomboy/link");
    xml.write_attribute_string("xmlns",
                             "size",
                             "",
                             "http://beatniksoftware.com/tomboy/size");

    xml.write_start_element ("", "title", "");
    xml.write_string (note.title());
    xml.write_end_element ();

    xml.write_start_element ("", "text", "");
    xml.write_attribute_string ("xml", "space", "", "preserve");
    // Insert <note-content> blob...
    xml.write_raw (note.text());
    xml.write_end_element ();

    xml.write_start_element ("", "last-change-date", "");
    xml.write_string (
      sharp::XmlConvert::to_string (note.change_date()));
    xml.write_end_element ();

    xml.write_start_element ("", "last-metadata-change-date", "");
    xml.write_string (
      sharp::XmlConvert::to_string (note.metadata_change_date()));
    xml.write_end_element ();

    if (note.create_date().is_valid()) {
      xml.write_start_element ("", "create-date", "");
      xml.write_string (
        sharp::XmlConvert::to_string (note.create_date()));
      xml.write_end_element ();
    }

    xml.write_start_element ("", "cursor-position", "");
    xml.write_string(TO_STRING(note.cursor_position()));
    xml.write_end_element ();

    xml.write_start_element("", "selection-bound-position", "");
    xml.write_string(TO_STRING(note.selection_bound_position()));
    xml.write_end_element();

    xml.write_start_element ("", "width", "");
    xml.write_string(TO_STRING(note.width()));
    xml.write_end_element ();

    xml.write_start_element("", "height", "");
    xml.write_string(TO_STRING(note.height()));
    xml.write_end_element();

    if (note.tags().size() > 0) {
      xml.write_start_element ("", "tags", "");
      for(NoteData::TagMap::const_iterator iter = note.tags().begin();
          iter != note.tags().end(); ++iter) {
        xml.write_start_element("", "tag", "");
        xml.write_string(iter->second->name());
        xml.write_end_element();
      }
      xml.write_end_element();
    }

    xml.write_end_element(); // Note
    xml.write_end_document();

  }
  
  std::string NoteArchiver::get_renamed_note_xml(const std::string & note_xml, 
                                                 const std::string & old_title,
                                                 const std::string & new_title) const
  {
    std::string updated_xml;
    // Replace occurences of oldTitle with newTitle in noteXml
    std::string titleTagPattern =  
      str(boost::format("<title>%1%</title>") % old_title);
    std::string titleTagReplacement =
      str(boost::format("<title>%1%</title>") % new_title);
    updated_xml = sharp::string_replace_regex(note_xml, titleTagPattern, titleTagReplacement);

    std::string titleContentPattern =
      str(boost::format("<note-content([^>]*)>\\s*%1%") % old_title);
    std::string titleContentReplacement =
      str(boost::format("<note-content\\1>%1%") % new_title);
    std::string updated_xml2 = sharp::string_replace_regex(updated_xml, titleContentPattern, 
                                                           titleContentReplacement);

    return updated_xml2;

  }

  std::string NoteArchiver::get_title_from_note_xml(const std::string & noteXml) const
  {
    if (!noteXml.empty()) {
      sharp::XmlReader xml;

      xml.load_buffer(noteXml);

      while (xml.read ()) {
        switch (xml.get_node_type()) {
        case XML_READER_TYPE_ELEMENT:
          if (xml.get_name() == "title") {
            return xml.read_string ();
          }
          break;
        default:
          break;
        }
      }
    }

    return "";
  }
  

}
