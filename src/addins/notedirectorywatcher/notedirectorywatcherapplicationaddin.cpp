/*
 * gnote
 *
 * Copyright (C) 2012-2013 Aurimas Cernius
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


#include <fstream>

#include <glibmm/i18n.h>

#include "debug.hpp"
#include "notedirectorywatcherapplicationaddin.hpp"
#include "notemanager.hpp"
#include "sharp/files.hpp"
#include "sharp/string.hpp"


namespace notedirectorywatcher {

NoteDirectoryWatcherModule::NoteDirectoryWatcherModule()
{
  ADD_INTERFACE_IMPL(NoteDirectoryWatcherApplicationAddin);
}




NoteDirectoryWatcherApplicationAddin::NoteDirectoryWatcherApplicationAddin()
  : m_initialized(false)
{
}

void NoteDirectoryWatcherApplicationAddin::initialize()
{
  gnote::NoteManager & manager(note_manager());
  std::string note_path = manager.get_notes_dir();
  manager.signal_note_saved
    .connect(sigc::mem_fun(*this, &NoteDirectoryWatcherApplicationAddin::handle_note_saved));

  Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(note_path);
  m_file_system_watcher = file->monitor_directory();

  m_file_system_watcher->signal_changed()
    .connect(sigc::mem_fun(*this, &NoteDirectoryWatcherApplicationAddin::handle_file_system_change_event));

  m_initialized = true;
}

void NoteDirectoryWatcherApplicationAddin::shutdown()
{
  m_file_system_watcher->cancel();
  m_initialized = false;
}

bool NoteDirectoryWatcherApplicationAddin::initialized()
{
  return m_initialized;
}

void NoteDirectoryWatcherApplicationAddin::handle_note_saved(const gnote::Note::Ptr & note)
{
  m_note_save_times[note->id()] = sharp::DateTime::now();
}

void NoteDirectoryWatcherApplicationAddin::handle_file_system_change_event(
  const Glib::RefPtr<Gio::File> & file, const Glib::RefPtr<Gio::File> &, Gio::FileMonitorEvent event_type)
{
  switch(event_type) {
  case Gio::FILE_MONITOR_EVENT_CHANGED:
  case Gio::FILE_MONITOR_EVENT_DELETED:
  case Gio::FILE_MONITOR_EVENT_CREATED:
  case Gio::FILE_MONITOR_EVENT_MOVED:
    break;
  default:
    return;
  }

  std::string note_id = get_id(file->get_path());

  DBG_OUT("NoteDirectoryWatcher: %s has %d (note_id=%s)", file->get_path().c_str(), int(event_type), note_id.c_str());

  // Record that the file has been added/changed/deleted.  Adds/changes trump
  // deletes.  Record the date.
  m_lock.lock();
  try {
    std::map<std::string, NoteFileChangeRecord>::iterator record = m_file_change_records.find(note_id);
    if(record == m_file_change_records.end()) {
      m_file_change_records[note_id] = NoteFileChangeRecord();
      record = m_file_change_records.find(note_id);
    }

    if(event_type == Gio::FILE_MONITOR_EVENT_CHANGED) {
      record->second.changed = true;
      record->second.deleted = false;
    }
    else if(event_type == Gio::FILE_MONITOR_EVENT_CREATED) {
      record->second.changed = true;
      record->second.deleted = false;
    }
    else if(event_type == Gio::FILE_MONITOR_EVENT_MOVED) {
      record->second.changed = true;
      record->second.deleted = false;
    }
    else if(event_type == Gio::FILE_MONITOR_EVENT_DELETED) {
      if(!record->second.changed) {
	record->second.deleted = true;
      }
    }

    record->second.last_change = sharp::DateTime::now();
  }
  catch(...)
  {}
  m_lock.unlock();

  Glib::RefPtr<Glib::TimeoutSource> timeout = Glib::TimeoutSource::create(5000);
  timeout->connect(sigc::mem_fun(*this, &NoteDirectoryWatcherApplicationAddin::handle_timeout));
  timeout->attach();
}

std::string NoteDirectoryWatcherApplicationAddin::get_id(const std::string & path)
{
  std::string dir_separator;
  dir_separator += G_DIR_SEPARATOR;
  int last_slash = sharp::string_last_index_of(path, std::string(dir_separator));
  int first_period = sharp::string_index_of(path, ".", last_slash);

  return path.substr(last_slash + 1, first_period - last_slash - 1);
}

bool NoteDirectoryWatcherApplicationAddin::handle_timeout()
{
  m_lock.lock();
  try {
    std::vector<std::string> keysToRemove(m_file_change_records.size());

    for(std::map<std::string, NoteFileChangeRecord>::iterator iter = m_file_change_records.begin();
        iter != m_file_change_records.end(); ++iter) {
      DBG_OUT("NoteDirectoryWatcher: Handling (timeout) %s", iter->first.c_str());

      // Check that Note.Saved event didn't occur within 3 seconds of last write
      if(m_note_save_times.find(iter->first) != m_note_save_times.end() &&
          std::abs((m_note_save_times[iter->first] - iter->second.last_change).total_seconds()) <= 3) {
        DBG_OUT("NoteDirectoryWatcher: Ignoring (timeout) because it was probably a Gnote write");
        keysToRemove.push_back(iter->first);
        continue;
      }
      // TODO: Take some actions to clear note_save_times? Not a large structure...

      sharp::DateTime last_change(iter->second.last_change);
      if(sharp::DateTime::now() > last_change.add_seconds(4)) {
        if(iter->second.deleted) {
          delete_note(iter->first);
        }
        else {
          add_or_update_note(iter->first);
        }

        keysToRemove.push_back(iter->first);
      }
    }

    for(std::vector<std::string>::iterator note_id = keysToRemove.begin(); note_id != keysToRemove.end(); ++note_id) {
      m_file_change_records.erase(*note_id);
    }
  }
  catch(...)
  {}
  m_lock.unlock();

  return false;
}

void NoteDirectoryWatcherApplicationAddin::delete_note(const std::string & note_id)
{
  DBG_OUT("NoteDirectoryWatcher: deleting %s because file deleted.", note_id.c_str());

  std::string note_uri = make_uri(note_id);

  gnote::Note::Ptr note_to_delete = note_manager().find_by_uri(note_uri);
  if(note_to_delete != 0) {
    note_manager().delete_note(note_to_delete);
  }
  else {
    DBG_OUT("notedirectorywatcher: did not delete %s because note not found.", note_id.c_str());
  }
}

void NoteDirectoryWatcherApplicationAddin::add_or_update_note(const std::string & note_id)
{
  std::string note_path = Glib::build_filename(
    note_manager().get_notes_dir(), note_id + ".note");
  if (!sharp::file_exists(note_path)) {
    DBG_OUT("NoteDirectoryWatcher: Not processing update of %s because file does not exist.", note_path.c_str());
    return;
  }

  std::string noteXml;
  try {
    std::ifstream reader;
    reader.exceptions(std::ios_base::badbit);
    reader.open(note_path.c_str());
    std::string line;
    while(std::getline(reader, line)) {
      noteXml += line + '\n';
    }
    reader.close();
  }
  catch(std::ios::failure & e) {
    /* TRANSLATORS: first %s is file name, second is error */
    ERR_OUT(_("NoteDirectoryWatcher: Update aborted, error reading %s: %s"), note_path.c_str(), e.what());
    return;
  }

  if(noteXml == "") {
    DBG_OUT("NoteDirectoryWatcher: Update aborted, %s had no contents.", note_path.c_str());
    return;
  }

  std::string note_uri = make_uri(note_id);

  gnote::Note::Ptr note = note_manager().find_by_uri(note_uri);

  bool is_new_note = false;

  if(note == 0) {
    is_new_note = true;
    DBG_OUT("NoteDirectoryWatcher: Adding %s because file changed.", note_id.c_str());

    std::string title;
    Glib::RefPtr<Glib::Regex> regex = Glib::Regex::create("<title>([^<]+)</title>", Glib::REGEX_MULTILINE);
    Glib::MatchInfo match_info;
    if(regex->match(noteXml, match_info)) {
      title = match_info.fetch(1);
    }
    else {
      /* TRANSLATORS: %s is file */
      ERR_OUT(_("NoteDirectoryWatcher: Error reading note title from %s"), note_path.c_str());
      return;
    }

    try {
      note = note_manager().create_with_guid(title, note_id);
      if(note == 0) {
        /* TRANSLATORS: %s is file */
        ERR_OUT(_("NoteDirectoryWatcher: Unknown error creating note from %s"), note_path.c_str());
        return;
      }
    }
    catch(std::exception & e) {
      /* TRANSLATORS: first %s is file, second is error */
      ERR_OUT(_("NoteDirectoryWatcher: Error creating note from %s: %s"), note_path.c_str(), e.what());
      return;
    }
  }

  if(is_new_note) {
    DBG_OUT("NoteDirectoryWatcher: Updating %s because file changed.", note_id.c_str());
  }
  try {
    note->load_foreign_note_xml(noteXml, gnote::CONTENT_CHANGED);
  }
  catch(std::exception & e) {
    /* TRANSLATORS: first %s is file, second is error */
    ERR_OUT(_("NoteDirectoryWatcher: Update aborted, error parsing %s: %s"), note_path.c_str(), e.what());
    if(is_new_note) {
      note_manager().delete_note(note);
    }
  }
}

std::string NoteDirectoryWatcherApplicationAddin::make_uri(const std::string & note_id)
{
  return "note://gnote/" + note_id;
}


}

