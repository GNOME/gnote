/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017,2019,2022-2023 Aurimas Cernius
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
    typedef sigc::slot<void()> EventHandler;
    static const char * IFACE_NAME;

    SyncServiceAddin()
      : m_sync_manager(NULL)
    {}

    void initialize(IGnote & ignote, ISyncManager & sync_manager)
      {
        AbstractAddin::initialize(ignote);
        m_sync_manager = &sync_manager;
        initialize();
      }

    virtual ISyncManager & sync_manager()
      {
        return *m_sync_manager;
      }
    virtual SyncServer *create_sync_server() = 0;
    virtual void post_sync_cleanup() = 0;
    virtual Gtk::Widget *create_preferences_control(Gtk::Window & parent, EventHandler requiredPrefChanged) = 0;
    virtual bool save_configuration(const sigc::slot<void(bool, Glib::ustring)> & on_saved) = 0;
    virtual void reset_configuration() = 0;
    virtual bool is_configured() const = 0;
    virtual bool are_settings_valid() const
      {
        return true;
      }
    virtual Glib::ustring name() const = 0;
    virtual Glib::ustring id() const = 0;
    virtual bool is_supported() const = 0;
    virtual void shutdown () = 0;
    virtual bool initialized () = 0;
  protected:
    virtual void initialize () = 0;
  private:
    ISyncManager *m_sync_manager;
  };

}
}


#endif
