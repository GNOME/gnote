/*
 * gnote
 *
 * Copyright (C) 2010-2014,2017,2019-2024 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>

#include "applicationaddin.hpp"
#include "debug.hpp"
#include "notemanager.hpp"
#include "addinmanager.hpp"
#include "ignote.hpp"
#include "itagmanager.hpp"
#include "sharp/directory.hpp"
#include "sharp/dynamicmodule.hpp"

namespace gnote {

  NoteManager::NoteManager(IGnote & g)
    : NoteManagerBase(g)
    , m_preferences(g.preferences())
    , m_notebook_manager(*this)
    , m_addin_mgr(NULL)
    , m_note_archiver(*this)
    , m_save_timeout(0)
  {
  }


  void NoteManager::init(const Glib::ustring & directory)
  {
    Glib::ustring backup = directory + "/Backup";
    bool is_first_run = NoteManagerBase::init(directory, backup);

    m_addin_mgr = create_addin_manager ();
    NoteTagTable::setup_instance(m_preferences);

    if (is_first_run) {
      std::vector<ImportAddin*> l = m_addin_mgr->get_import_addins();
      bool has_imported = false;

      if(l.empty()) {
        DBG_OUT("no import plugins");
      }

      for(auto import_addin : l) {
        DBG_OUT("importing");
        import_addin->initialize();
        if(import_addin->want_to_run(*this)) {
          has_imported |= import_addin->first_run(*this);
        }
        AddinInfo addin_info = m_addin_mgr->get_addin_info(*import_addin);
        if(addin_info.get_attribute("AutoDisable") == "true") {
          import_addin->shutdown();
          m_addin_mgr->get_module(addin_info.id())->enabled(false);
        }
      }
      m_addin_mgr->save_addins_prefs(); // we probably disabled some import plugins
      // we MUST call this after import
      post_load();

      // First run. Create "Start Here" notes.
      create_start_notes ();
    } 
    else {
      load_notes ();
      auto keep_backups_since = Glib::DateTime::create_now_utc().add_days(-30);
      delete_old_backups(backup, keep_backups_since);
    }

    m_notebook_manager.init();
    gnote().signal_quit.connect(sigc::mem_fun(*this, &NoteManager::on_exiting_event));
  }

  NoteManager::~NoteManager()
  {
    delete m_addin_mgr;
  }

  AddinManager *NoteManager::create_addin_manager()
  {
    return new AddinManager(gnote(), *this, m_preferences, IGnote::conf_dir());
  }

  void NoteManager::create_start_notes ()
  {
    // FIXME: Delay the creation of the start notes so the panel/tray
    // icon has enough time to appear so that Tomboy.TrayIconShowing
    // is valid.  Then, we'll be able to instruct the user where to
    // find the Tomboy icon.
    //string icon_str = Tomboy.TrayIconShowing ?
    //     Catalog.GetString ("System Tray Icon area") :
    //     Catalog.GetString ("GNOME Panel");

    // for some reason I have to set the xmlns -- Hub
    Glib::ustring start_note_content =
      _("<note-content xmlns:link=\"http://beatniksoftware.com/tomboy/link\">"
        "Start Here\n\n"
        "<bold>Welcome to Gnote!</bold>\n\n"
        "Use this \"Start Here\" note to begin organizing "
        "your ideas and thoughts.\n\n"
        "You can create new notes to hold your ideas by "
        "selecting the \"Create New Note\" item from the "
        "Gnote menu in your GNOME Panel. "
        "Your note will be saved automatically.\n\n"
        "Then organize the notes you create by linking "
        "related notes and ideas together!\n\n"
        "We've created a note called "
        "<link:internal>Using Links in Gnote</link:internal>.  "
        "Notice how each time we type <link:internal>Using "
        "Links in Gnote</link:internal> it automatically "
        "gets underlined?  Click on the link to open the note."
        "</note-content>");

    Glib::ustring links_note_content =
      _("<note-content>"
        "Using Links in Gnote\n\n"
        "Notes in Gnote can be linked together by "
        "highlighting text in the current note and clicking"
        " the <bold>Link</bold> button above in the toolbar.  "
        "Doing so will create a new note and also underline "
        "the note's title in the current note.\n\n"
        "Changing the title of a note will update links "
        "present in other notes.  This prevents broken links "
        "from occurring when a note is renamed.\n\n"
        "Also, if you type the name of another note in your "
        "current note, it will automatically be linked for you."
        "</note-content>");

    try {
      auto & start_note = create(_("Start Here"), std::move(start_note_content));
      start_note.queue_save(CONTENT_CHANGED);
      m_preferences.start_note_uri(start_note.uri());

      auto & links_note = create(_("Using Links in Gnote"), std::move(links_note_content));
      links_note.queue_save(CONTENT_CHANGED);
    } 
    catch (const std::exception & e) {
      ERR_OUT(_("Error creating start notes: %s"), e.what());
    }
  }

  void NoteManager::load_notes()
  {
    std::vector<Glib::ustring> files = sharp::directory_get_files_with_ext(notes_dir(), ".note");

    for(auto & file_path : files) {
      try {
        Note::Ptr note = Note::load(std::move(file_path), *this, gnote());
        add_note(note);
      } 
      catch (const std::exception & e) {
        /* TRANSLATORS: first %s is file, second is error */
        ERR_OUT(_("Error parsing note XML, skipping \"%s\": %s"),
                file_path.c_str(), e.what());
      }
    }
    post_load();
    // Make sure that a Start Note Uri is set in the preferences, and
    // make sure that the Uri is valid to prevent bug #508982. This
    // has to be done here for long-time Tomboy users who won't go
    // through the create_start_notes () process.
    auto start_note_uri = m_preferences.start_note_uri();
    if (start_note_uri.empty() || !find_by_uri(start_note_uri)) {
      // Attempt to find an existing Start Here note
      auto start_note = find(_("Start Here"));
      if(start_note) {
        m_preferences.start_note_uri(start_note.value().get().uri());
      }
    }
  }


  void NoteManager::post_load()
  {
    NoteManagerBase::post_load();

    // Load all the addins for our notes.
    // Iterating through copy of notes list, because list may be
    // changed when loading addins.
    decltype(m_notes) notesCopy(m_notes);
    for(const NoteBase::Ptr & iter : notesCopy) {
      m_addin_mgr->load_addins_for_note(*iter);
    }
  }

  void NoteManager::migrate_notes(const Glib::ustring & old_note_dir)
  {
    std::vector<Glib::ustring> files = sharp::directory_get_files_with_ext(old_note_dir, ".note");

    for(auto file : files) {
      const Glib::RefPtr<Gio::File> src = Gio::File::create_for_path(file);
      const Glib::ustring dest_path = Glib::build_filename(notes_dir(), Glib::path_get_basename(file.c_str()));
      const Glib::RefPtr<Gio::File> dest = Gio::File::create_for_path(dest_path);
      src->copy(dest, Gio::File::CopyFlags::NONE);
    }

    const Glib::ustring old_backup_dir = Glib::build_filename(old_note_dir, "Backup");
    files = sharp::directory_get_files_with_ext(old_backup_dir, ".note");

    for(auto file : files) {
      const Glib::RefPtr<Gio::File> src = Gio::File::create_for_path(file);
      const Glib::ustring dest_path = Glib::build_filename(m_backup_dir, Glib::path_get_basename(file.c_str()));
      const Glib::RefPtr<Gio::File> dest = Gio::File::create_for_path(dest_path);
      src->copy(dest, Gio::File::CopyFlags::NONE);
    }
  }

  void NoteManager::on_exiting_event()
  {
    m_addin_mgr->shutdown_application_addins();

    DBG_OUT("Saving unsaved notes...");
      
    // Use a copy of the notes to prevent bug #510442 (crash on exit
    // when iterating the notes to save them.
    decltype(m_notes) notesCopy(m_notes);
    for(const NoteBase::Ptr & note : notesCopy) {
      note->save();
    }
  }

  NoteBase::Ptr NoteManager::note_load(Glib::ustring && file_name)
  {
    return Note::load(std::move(file_name), *this, gnote());
  }


  Note & NoteManager::create_note(Glib::ustring && title, Glib::ustring && body, Glib::ustring && guid)
  {
    bool select_body = body.empty();
    auto & new_note = static_cast<Note&>(NoteManagerBase::create_note(std::move(title), std::move(body), std::move(guid)));
    if(select_body) {
      // Select the inital text so typing will overwrite the body text
      new_note.get_buffer()->select_note_body();
    }
    return new_note;
  }

  // Create a new note with the specified Xml content
  Note & NoteManager::create_new_note(Glib::ustring && title, Glib::ustring && xml_content, Glib::ustring && guid)
  {
    auto & new_note = static_cast<Note&>(NoteManagerBase::create_new_note(std::move(title), std::move(xml_content), std::move(guid)));

    // Load all the addins for the new note
    m_addin_mgr->load_addins_for_note(new_note);

    return new_note;
  }

  NoteBase::Ptr NoteManager::note_create_new(Glib::ustring && title, Glib::ustring && file_name)
  {
    return Note::create_new_note(std::move(title), std::move(file_name), *this, gnote());
  }

  Note & NoteManager::get_or_create_template_note()
  {
    Note & note = static_cast<Note&>(NoteManagerBase::get_or_create_template_note());

    // Select the initial text
    Glib::RefPtr<NoteBuffer> buffer = note.get_buffer();
    buffer->select_note_body();
    return note;
  }

  // Creates a new note with the given title and guid with body based on
  // the template note.
  Note & NoteManager::create_note_from_template(Glib::ustring && title, const NoteBase & template_note, Glib::ustring && guid)
  {
    auto title_size = title.size();
    auto & new_note = static_cast<Note&>(NoteManagerBase::create_note_from_template(std::move(title), template_note, std::move(guid)));

    // Copy template note's properties
    Glib::RefPtr<Gtk::TextBuffer> buffer = new_note.get_buffer();
    Gtk::TextIter cursor, selection;
    auto &template_save_selection = m_tag_manager.get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SAVE_SELECTION_SYSTEM_TAG);
    if(template_note.contains_tag(template_save_selection)) {
      Glib::ustring template_title = template_note.get_title();
      int cursor_pos = template_note.data().cursor_position();
      int selection_bound = template_note.data().selection_bound_position();
      if(cursor_pos == 0) {
        // the created note has different title from template, take that into account
        // selection starts at the beginning of title
        // if it ends at the end of title, select entire title
        // if it ends after the title, keep the same offset from the end selected
        // otherwise select nothing, we don't know, what the selected part of title means anyway
        cursor = buffer->get_iter_at_offset(0);
        selection = cursor;
        if(selection_bound == int(template_title.size())) {
          selection.forward_to_line_end();
        }
        else if(selection_bound > int(template_title.size())) {
          selection.forward_to_line_end();
          selection.forward_chars(selection_bound - template_title.size());
        }
      }
      else if(cursor_pos <= int(template_title.size())) {
        cursor = buffer->get_iter_at_line(1);
        selection = cursor;
        selection.forward_chars(selection_bound - template_title.size() - 1); // skip title and new line
      }
      else {
        cursor = buffer->get_iter_at_offset(cursor_pos - template_title.size() + title_size - 1);
        selection = buffer->get_iter_at_offset(selection_bound - template_title.size() + title_size - 1);
      }
    }
    else {
      // move cursor to the start of first word after the title
      cursor = buffer->get_iter_at_line(1);
      while(!cursor.starts_word() && cursor.forward_char());
      selection = cursor;
    }

    buffer->place_cursor(cursor);
    if(selection != cursor) {
      buffer->move_mark(buffer->get_selection_bound(), selection);
    }

    return new_note;
  }

  void NoteManager::queue_save(NoteBase & note)
  {
    const auto & uri = note.uri();
    for(const auto & to_save : m_queued_saves) {
      if(to_save == uri) {
        return;
      }
    }

    m_queued_saves.push_back(uri);
    if(m_save_timeout) {
      return;
    }

    auto save_callback = [](gpointer data) -> gboolean {
      auto & manager = *static_cast<NoteManager*>(data);
      manager.save_notes();
      if(manager.m_queued_saves.size()) {
        return TRUE;
      }

      manager.m_save_timeout = 0;
      return FALSE;
    };
    m_save_timeout = g_timeout_add_seconds(4, save_callback, this);
  }

  void NoteManager::save_notes()
  {
    if(m_queued_saves.size() == 0) {
      return;
    }

    // make copy, just incase saving causes new queues
    decltype(m_queued_saves) to_save;
    std::swap(m_queued_saves, to_save);

    for(const auto & uri : to_save) {
      bool did_find = find_by_uri(uri, [](NoteBase & note) {
        try {
          note.save();
        }
        catch(const sharp::Exception &e) {
          ERR_OUT(_("Error while saving: %s"), e.what());
        }
      });

      if(!did_find) {
        ERR_OUT(_("Did not find note with uri '%s', cannot save"), uri.c_str());
      }
    }
  }

}
