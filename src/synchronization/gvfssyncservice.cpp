/*
 * gnote
 *
 * Copyright (C) 2019-2023,2025 Aurimas Cernius
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
#include <glibmm/miscutils.h>

#include "debug.hpp"
#include "gvfssyncservice.hpp"
#include "base/monitor.hpp"
#include "sharp/directory.hpp"
#include "sharp/files.hpp"


namespace gnote
{
namespace sync
{


GvfsSyncService::GvfsSyncService()
  : m_initialized(false)
  , m_enabled(false)
{
}

void GvfsSyncService::initialize()
{
  m_initialized = true;
  m_enabled = true;
}

void GvfsSyncService::shutdown()
{
  m_enabled = false;
}

void GvfsSyncService::post_sync_cleanup()
{
  unmount_sync();
}

bool GvfsSyncService::is_supported() const
{
  return true;
}

bool GvfsSyncService::initialized()
{
  return m_initialized && m_enabled;
}

bool GvfsSyncService::test_sync_directory(const Glib::RefPtr<Gio::File> & path, const Glib::ustring & sync_uri, Glib::ustring & error)
{
  try {
    if(sharp::directory_exists(path) == false) {
      if(!sharp::directory_create(path)) {
        DBG_OUT("Could not create \"%s\"", sync_uri.c_str());
        error = _("Specified folder path does not exist, and Gnote was unable to create it.");
        return false;
      }
    }
    else {
      // Test creating/writing/deleting a file
      Glib::ustring test_path_base = Glib::build_filename(sync_uri, "test");
      Glib::RefPtr<Gio::File> test_path = Gio::File::create_for_uri(test_path_base);
      int count = 0;

      // Get unique new file name
      while(test_path->query_exists()) {
        test_path = Gio::File::create_for_uri(test_path_base + TO_STRING(++count));
      }

      // Test ability to create and write
      Glib::ustring test_line = "Testing write capabilities.";
      auto stream = test_path->create_file();
      stream->write(test_line);
      stream->close();

      if(!test_path->query_exists()) {
        error = _("Failure writing test file");
        return false;
      }

      // Test ability to delete
      if(!test_path->remove()) {
        error = _("Failure when trying to remove test file");
        return false;
      }
    }

    return true;
  }
  catch(std::exception & e) {
    error = e.what();
    return false;
  }
  catch(...) {
    error = _("Unknown error");
    return false;
  }
}

bool GvfsSyncService::mount_async(const Glib::RefPtr<Gio::File> & path, const std::function<void(bool, const Glib::ustring &)> & completed, const Glib::RefPtr<Gio::MountOperation> & op)
{
  try {
    path->find_enclosing_mount();
    return true;
  }
  catch(Gio::Error & e) {
  }

  path->mount_enclosing_volume(op, [this, path, completed](Glib::RefPtr<Gio::AsyncResult> & result) {
    Glib::ustring error;
    try {
      if(path->mount_enclosing_volume_finish(result)) {
        m_mount = path->find_enclosing_mount();
      }
    }
    catch(Glib::Error & e) {
      error = e.what();
    }
    catch(...) {
    }

    completed(bool(m_mount), error);
  });

  return false;
}

bool GvfsSyncService::mount_sync(const Glib::RefPtr<Gio::File> & path, const Glib::RefPtr<Gio::MountOperation> & op)
{
  bool ret = true;
  CompletionMonitor cond;
  {
    CompletionMonitor::WaitLock lock(cond);
    if(mount_async(path, [&ret, &cond](bool result, const Glib::ustring&) {
         CompletionMonitor::NotifyLock lock(cond);
         ret = result;
       }, op)) {
      return true;
    }
  }

  return ret;
}

void GvfsSyncService::unmount_async(const std::function<void()> & completed)
{
  if(!m_mount) {
    completed();
    return;
  }

  m_mount->unmount([this, completed](Glib::RefPtr<Gio::AsyncResult> & result) {
    try {
      m_mount->unmount_finish(result);
    }
    catch(...) {
    }

    m_mount.reset();
    completed();
  });
}

void GvfsSyncService::unmount_sync()
{
  if(!m_mount) {
    return;
  }

  CompletionMonitor cond;
  {
    CompletionMonitor::WaitLock lock(cond);
    unmount_async([this, &cond]{
      CompletionMonitor::NotifyLock lock(cond);
      m_mount.reset();
    });
  }
}


}
}

