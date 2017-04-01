/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017 Aurimas Cernius
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

#ifndef _SYNCHRONIZATION_FUSESYNCSERVICEADDIN_HPP_
#define _SYNCHRONIZATION_FUSESYNCSERVICEADDIN_HPP_

#include <glibmm/spawn.h>

#include "base/macros.hpp"
#include "syncserviceaddin.hpp"
#include "utils.hpp"


namespace gnote {
namespace sync {

class FuseSyncServiceAddin
  : public SyncServiceAddin
{
public:
  static const int DEFAULT_MOUNT_TIMEOUT_MS;

  FuseSyncServiceAddin();

  virtual void shutdown() override;
  virtual bool initialized() override;
  virtual void initialize() override;
  virtual SyncServer::Ptr create_sync_server() override;
  virtual void post_sync_cleanup() override;
  virtual bool is_supported() override;
  virtual bool save_configuration() override;
  virtual void reset_configuration() override;

  virtual Glib::ustring fuse_mount_timeout_error();
  virtual Glib::ustring fuse_mount_directory_error();
protected:
  virtual bool verify_configuration() = 0;
  virtual void save_configuration_values() = 0;
  virtual void reset_configuration_values() = 0;
  virtual Glib::ustring fuse_mount_exe_name() = 0;
  virtual std::vector<Glib::ustring> get_fuse_mount_exe_args(const Glib::ustring & mountPath, bool fromStoredValues) = 0;
  virtual Glib::ustring get_fuse_mount_exe_args_for_display(const Glib::ustring & mountPath, bool fromStoredValues) = 0;
private:
  static void redirect_standard_error();
  static bool wait_for_exit(Glib::Pid, int timeout, int *exit_code);
  bool mount_fuse(bool useStoredValues);
  int get_timeout_ms();
  void set_up_mount_path();
  void prepare_mount_path();
  void gnote_exit_handler();
  void unmount_timeout();
  bool is_mounted();

  Glib::ustring m_mount_path;
  utils::InterruptableTimeout m_unmount_timeout;

  Glib::ustring m_fuse_mount_exe_path;
  Glib::ustring m_fuse_unmount_exe_path;
  Glib::ustring m_mount_exe_path;
  bool m_initialized;
  bool m_enabled;
};

}
}

#endif

