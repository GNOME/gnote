/*
 * gnote
 *
 * Copyright (C) 2012-2014,2017,2021-2023 Aurimas Cernius
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


#include <glibmm/i18n.h>
#include <glibmm/main.h>
#include <glibmm/miscutils.h>
#include <glibmm/regex.h>

#include "debug.hpp"
#include "ignote.hpp"
#include "notedirectorywatcherapplicationaddin.hpp"
#include "notedirectorywatcherpreferencesfactory.hpp"
#include "notemanager.hpp"
#include "sharp/files.hpp"
#include "sharp/string.hpp"


namespace notedirectorywatcher {

NoteDirectoryWatcherModule::NoteDirectoryWatcherModule()
{
  ADD_INTERFACE_IMPL(NoteDirectoryWatcherApplicationAddin);
  ADD_INTERFACE_IMPL(NoteDirectoryWatcherPreferencesFactory);
}




NoteDirectoryWatcherApplicationAddin::NoteDirectoryWatcherApplicationAddin()
  : m_initialized(false)
{
}

void NoteDirectoryWatcherApplicationAddin::initialize()
{
  gnote::NoteManager & manager(note_manager());
  const Glib::ustring & note_path = manager.notes_dir();
  m_signal_note_saved_cid = manager.signal_note_saved
    .connect(sigc::mem_fun(*this, &NoteDirectoryWatcherApplicationAddin::handle_note_saved));

  Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(note_path);
  m_file_system_watcher = file->monitor_directory();

  m_signal_changed_cid = m_file_system_watcher->signal_changed()
    .connect(sigc::mem_fun(*this, &NoteDirectoryWatcherApplicationAddin::handle_file_system_change_event));

  m_signal_settings_changed_cid = NoteDirectoryWatcherPreferences::settings()->signal_changed(CHECK_INTERVAL)
    .connect(sigc::mem_fun(*this, &NoteDirectoryWatcherApplicationAddin::on_settings_changed));
  m_check_interval = NoteDirectoryWatcherPreferences::settings()->get_int(CHECK_INTERVAL);
  sanitize_check_interval(NoteDirectoryWatcherPreferences::settings());

  m_initialized = true;
}

void NoteDirectoryWatcherApplicationAddin::shutdown()
{
  m_file_system_watcher->cancel();
  m_signal_note_saved_cid.disconnect();
  m_signal_changed_cid.disconnect();
  m_signal_settings_changed_cid.disconnect();
  m_initialized = false;
}

bool NoteDirectoryWatcherApplicationAddin::initialized()
{
  return m_initialized;
}

void NoteDirectoryWatcherApplicationAddin::handle_note_saved(gnote::NoteBase & note)
{
  m_note_save_times[note.id()] = Glib::DateTime::create_now_utc();
}

void NoteDirectoryWatcherApplicationAddin::handle_file_system_change_event(
  const Glib::RefPtr<Gio::File> & file, const Glib::RefPtr<Gio::File> &, Gio::FileMonitor::Event event_type)
{
  switch(event_type) {
  case Gio::FileMonitor::Event::CHANGED:
  case Gio::FileMonitor::Event::DELETED:
  case Gio::FileMonitor::Event::CREATED:
  case Gio::FileMonitor::Event::MOVED:
    break;
  default:
    return;
  }

  Glib::ustring note_id = get_id(file->get_path());

  DBG_OUT("NoteDirectoryWatcher: %s has %d (note_id=%s)", file->get_path().c_str(), int(event_type), note_id.c_str());

  // Record that the file has been added/changed/deleted.  Adds/changes trump
  // deletes.  Record the date.
  try {
    std::lock_guard<std::mutex> lock(m_lock);
    auto record = m_file_change_records.find(note_id);
    if(record == m_file_change_records.end()) {
      m_file_change_records[note_id] = NoteFileChangeRecord();
      record = m_file_change_records.find(note_id);
    }

    if(event_type == Gio::FileMonitor::Event::CHANGED) {
      record->second.changed = true;
      record->second.deleted = false;
    }
    else if(event_type == Gio::FileMonitor::Event::CREATED) {
      record->second.changed = true;
      record->second.deleted = false;
    }
    else if(event_type == Gio::FileMonitor::Event::MOVED) {
      record->second.changed = true;
      record->second.deleted = false;
    }
    else if(event_type == Gio::FileMonitor::Event::DELETED) {
      if(!record->second.changed) {
	record->second.deleted = true;
      }
    }

    record->second.last_change = Glib::DateTime::create_now_utc();
  }
  catch(...)
  {}

  Glib::RefPtr<Glib::TimeoutSource> timeout = Glib::TimeoutSource::create(m_check_interval * 1000);
  timeout->connect(sigc::mem_fun(*this, &NoteDirectoryWatcherApplicationAddin::handle_timeout));
  timeout->attach();
}

Glib::ustring NoteDirectoryWatcherApplicationAddin::get_id(const Glib::ustring & path)
{
  Glib::ustring dir_separator;
  dir_separator += G_DIR_SEPARATOR;
  int last_slash = sharp::string_last_index_of(path, dir_separator);
  int first_period = path.find(".", last_slash);

  return path.substr(last_slash + 1, first_period - last_slash - 1);
}

bool NoteDirectoryWatcherApplicationAddin::handle_timeout()
{
  m_lock.lock();
  try {
    std::vector<Glib::ustring> keysToRemove(m_file_change_records.size());

    for(auto iter : m_file_change_records) {
      DBG_OUT("NoteDirectoryWatcher: Handling (timeout) %s", iter.first.c_str());

      // Check that Note.Saved event didn't occur within (check-interval -2) seconds of last write
      if(m_note_save_times.find(iter.first) != m_note_save_times.end() &&
          std::abs(sharp::time_span_total_seconds(m_note_save_times[iter.first].difference(iter.second.last_change))) <= (m_check_interval - 2)) {
        DBG_OUT("NoteDirectoryWatcher: Ignoring (timeout) because it was probably a Gnote write");
        keysToRemove.push_back(iter.first);
        continue;
      }
      // TODO: Take some actions to clear note_save_times? Not a large structure...

      Glib::DateTime last_change(iter.second.last_change);
      if(Glib::DateTime::create_now_utc() > last_change.add_seconds(4)) {
        if(iter.second.deleted) {
          delete_note(iter.first);
        }
        else {
          add_or_update_note(iter.first);
        }

        keysToRemove.push_back(iter.first);
      }
    }

    for(auto note_id : keysToRemove) {
      m_file_change_records.erase(note_id);
    }
  }
  catch(...)
  {}
  m_lock.unlock();

  return false;
}

void NoteDirectoryWatcherApplicationAddin::delete_note(const Glib::ustring & note_id)
{
  DBG_OUT("NoteDirectoryWatcher: deleting %s because file deleted.", note_id.c_str());

  Glib::ustring note_uri = make_uri(note_id);

  if(!note_manager().find_by_uri(note_uri, [this](gnote::NoteBase & note_to_delete) {
    note_manager().delete_note(note_to_delete);
  })) {
    DBG_OUT("notedirectorywatcher: did not delete %s because note not found.", note_id.c_str());
  }
}

void NoteDirectoryWatcherApplicationAddin::add_or_update_note(const Glib::ustring & note_id)
{
  const Glib::ustring & note_path = Glib::build_filename(note_manager().notes_dir(), note_id + ".note");
  if (!sharp::file_exists(note_path)) {
    DBG_OUT("NoteDirectoryWatcher: Not processing update of %s because file does not exist.", note_path.c_str());
    return;
  }

  Glib::ustring noteXml;
  try {
    noteXml = sharp::file_read_all_text(note_path);
  }
  catch(sharp::Exception & e) {
    /* TRANSLATORS: first %s is file name, second is error */
    ERR_OUT(_("NoteDirectoryWatcher: Update aborted, error reading %s: %s"), note_path.c_str(), e.what());
    return;
  }

  if(noteXml == "") {
    DBG_OUT("NoteDirectoryWatcher: Update aborted, %s had no contents.", note_path.c_str());
    return;
  }

  Glib::ustring note_uri = make_uri(note_id);

  auto note = note_manager().find_by_uri(note_uri);

  bool is_new_note = false;

  if(!note) {
    is_new_note = true;
    DBG_OUT("NoteDirectoryWatcher: Adding %s because file changed.", note_id.c_str());

    Glib::ustring title;
    Glib::RefPtr<Glib::Regex> regex = Glib::Regex::create("<title>([^<]+)</title>", Glib::Regex::CompileFlags::MULTILINE);
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
      auto & n = note_manager().create_with_guid(std::move(title), Glib::ustring(note_id));
      note = gnote::NoteBase::Ref(std::ref(n));
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
    note.value().get().load_foreign_note_xml(noteXml, gnote::CONTENT_CHANGED);
  }
  catch(std::exception & e) {
    /* TRANSLATORS: first %s is file, second is error */
    ERR_OUT(_("NoteDirectoryWatcher: Update aborted, error parsing %s: %s"), note_path.c_str(), e.what());
    if(is_new_note) {
      note_manager().delete_note(note.value());
    }
  }
}

Glib::ustring NoteDirectoryWatcherApplicationAddin::make_uri(const Glib::ustring & note_id)
{
  return "note://gnote/" + note_id;
}

void NoteDirectoryWatcherApplicationAddin::on_settings_changed(const Glib::ustring & key)
{
  m_check_interval = NoteDirectoryWatcherPreferences::settings()->get_int(key);
  sanitize_check_interval(NoteDirectoryWatcherPreferences::settings());
}

void NoteDirectoryWatcherApplicationAddin::sanitize_check_interval(const Glib::RefPtr<Gio::Settings> & settings)
{
  if(m_check_interval < 5) {
    m_check_interval = 5;
    settings->set_int(CHECK_INTERVAL, m_check_interval);
  }
}

}

