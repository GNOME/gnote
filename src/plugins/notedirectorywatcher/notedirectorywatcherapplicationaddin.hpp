/*
 * gnote
 *
 * Copyright (C) 2012-2014,2017,2019-2021,2023 Aurimas Cernius
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

#ifndef _NOTE_DIRECTORY_WATCHER_APPLICATION_ADDIN_
#define _NOTE_DIRECTORY_WATCHER_APPLICATION_ADDIN_


#include <map>

#include <giomm/filemonitor.h>
#include <giomm/settings.h>

#include "applicationaddin.hpp"
#include "note.hpp"
#include "sharp/dynamicmodule.hpp"


namespace notedirectorywatcher {

class NoteDirectoryWatcherModule
  : public sharp::DynamicModule
{
public:
  NoteDirectoryWatcherModule();
};


DECLARE_MODULE(NoteDirectoryWatcherModule);

struct NoteFileChangeRecord
{
  Glib::DateTime last_change;
  bool deleted;
  bool changed;
};


class NoteDirectoryWatcherApplicationAddin
  : public gnote::ApplicationAddin
{
public:
  static NoteDirectoryWatcherApplicationAddin *create()
    {
      return new NoteDirectoryWatcherApplicationAddin;
    }
  virtual void initialize() override;
  virtual void shutdown() override;
  virtual bool initialized() override;
private:
  static Glib::ustring get_id(const Glib::ustring & path);
  static Glib::ustring make_uri(const Glib::ustring & note_id);

  NoteDirectoryWatcherApplicationAddin();
  void handle_note_saved(gnote::NoteBase &);
  void handle_file_system_change_event(const Glib::RefPtr<Gio::File> & file,
                                       const Glib::RefPtr<Gio::File> & other_file,
                                       Gio::FileMonitor::Event event_type);
  bool handle_timeout();
  void delete_note(const Glib::ustring & note_id);
  void add_or_update_note(const Glib::ustring & note_id);
  void on_settings_changed(const Glib::ustring & key);
  void sanitize_check_interval(const Glib::RefPtr<Gio::Settings> & settings);

  Glib::RefPtr<Gio::FileMonitor> m_file_system_watcher;

  std::map<Glib::ustring, NoteFileChangeRecord> m_file_change_records;
  std::map<Glib::ustring, Glib::DateTime> m_note_save_times;
  sigc::connection m_signal_note_saved_cid;
  sigc::connection m_signal_changed_cid;
  sigc::connection m_signal_settings_changed_cid;
  bool m_initialized;
  int m_check_interval;
  std::mutex m_lock;
};

}

#endif
