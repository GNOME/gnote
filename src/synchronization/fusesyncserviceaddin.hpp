/*
 * gnote
 *
 * Copyright (C) 2012 Aurimas Cernius
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

#include <glibmm.h>

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

  virtual void shutdown();
  virtual bool initialized();
  virtual void initialize();
  virtual SyncServer::Ptr create_sync_server();
  virtual void post_sync_cleanup();
  virtual bool is_supported();
  virtual bool save_configuration();
  virtual void reset_configuration();

  virtual std::string fuse_mount_timeout_error();
  virtual std::string fuse_mount_directory_error();
protected:
  virtual bool verify_configuration() = 0;
  virtual void save_configuration_values() = 0;
  virtual void reset_configuration_values() = 0;
  virtual std::string fuse_mount_exe_name() = 0;
  virtual std::vector<std::string> get_fuse_mount_exe_args(const std::string & mountPath, bool fromStoredValues) = 0;
  virtual std::string get_fuse_mount_exe_args_for_display(const std::string & mountPath, bool fromStoredValues) = 0;
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

  std::string m_mount_path;
  utils::InterruptableTimeout m_unmount_timeout;

  std::string m_fuse_mount_exe_path;
  std::string m_fuse_unmount_exe_path;
  std::string m_mount_exe_path;
  bool m_initialized;
  bool m_enabled;
};

}
}

#endif

