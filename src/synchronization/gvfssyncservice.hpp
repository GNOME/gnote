/*
 * gnote
 *
 * Copyright (C) 2019-2023 Aurimas Cernius
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


#include <functional>
#include <giomm/mount.h>
#include "synchronization/syncserviceaddin.hpp"


namespace gnote
{
namespace sync
{

class GvfsSyncService
  : public SyncServiceAddin
{
public:
  GvfsSyncService();

  virtual void initialize() override;
  virtual void shutdown() override;

  virtual void post_sync_cleanup() override;
  virtual bool is_supported() const override;
  virtual bool initialized() override;
protected:
  static bool test_sync_directory(const Glib::RefPtr<Gio::File> & path, const Glib::ustring & sync_uri, Glib::ustring & error);

  bool mount_async(const Glib::RefPtr<Gio::File> & path, const std::function<void(bool, const Glib::ustring &)> & completed,
    const Glib::RefPtr<Gio::MountOperation> & op = Glib::RefPtr<Gio::MountOperation>());
  bool mount_sync(const Glib::RefPtr<Gio::File> & path, const Glib::RefPtr<Gio::MountOperation> & op = Glib::RefPtr<Gio::MountOperation>());
  void unmount_async(const std::function<void()> & completed);
  void unmount_sync();

  bool m_initialized;
  bool m_enabled;
  Glib::ustring m_uri;
  Glib::RefPtr<Gio::Mount> m_mount;
};

}
}

