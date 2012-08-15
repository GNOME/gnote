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


#ifndef _FILESYSTEM_SYNC_SERVICE_ADDIN_
#define _FILESYSTEM_SYNC_SERVICE_ADDIN_

#include <gtkmm/filechooserbutton.h>

#include "sharp/dynamicmodule.hpp"
#include "synchronization/syncserviceaddin.hpp"



namespace filesystemsyncservice {

class FileSystemSyncServiceModule
  : public sharp::DynamicModule
{
public:
  FileSystemSyncServiceModule();
  virtual const char * id() const;
  virtual const char * name() const;
  virtual const char * description() const;
  virtual const char * authors() const;
  virtual int          category() const;
  virtual const char * version() const;
};


DECLARE_MODULE(FileSystemSyncServiceModule)


class FileSystemSyncServiceAddin
  : public gnote::sync::SyncServiceAddin
{
public:
  static FileSystemSyncServiceAddin *create()
    {
      return new FileSystemSyncServiceAddin;
    }
  FileSystemSyncServiceAddin();

  virtual void initialize();
  virtual void shutdown();

  virtual gnote::sync::SyncServer::Ptr create_sync_server();
  virtual void post_sync_cleanup();
  virtual Gtk::Widget *create_preferences_control(EventHandler requiredPrefChanged);
  virtual bool save_configuration();
  virtual void reset_configuration();
  virtual bool is_configured();
  virtual std::string name();
  virtual std::string id();
  virtual bool is_supported();
  virtual bool initialized();
private:
  bool get_config_settings(std::string & syncPath);

  Gtk::FileChooserButton *m_path_button;
  std::string m_path;
  bool m_initialized;
  bool m_enabled;
};

}

#endif
