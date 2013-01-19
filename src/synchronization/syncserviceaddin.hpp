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


#ifndef _SYNCHRONIZATION_SYNCSERVICEADDIN_HPP_
#define _SYNCHRONIZATION_SYNCSERVICEADDIN_HPP_


#include <gtkmm/widget.h>

#include "abstractaddin.hpp"
#include "isyncmanager.hpp"



namespace gnote {
namespace sync {

  class SyncServiceAddin
    : public AbstractAddin
  {
  public:
    typedef sigc::slot<void> EventHandler;
    static const char * IFACE_NAME;

    virtual SyncServer::Ptr create_sync_server() = 0;
    virtual void post_sync_cleanup() = 0;
    virtual Gtk::Widget *create_preferences_control(EventHandler requiredPrefChanged) = 0;
    virtual bool save_configuration() = 0;
    virtual void reset_configuration() = 0;
    virtual bool is_configured() = 0;
    virtual bool are_settings_valid()
      {
        return true;
      }
    virtual std::string name() = 0;
    virtual std::string id() = 0;
    virtual bool is_supported() = 0;
    virtual void initialize () = 0;
    virtual void shutdown () = 0;
    virtual bool initialized () = 0;
  };

}
}


#endif
