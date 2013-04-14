/*
 * gnote
 *
 * Copyright (C) 2010,2012-2013 Aurimas Cernius
 * Copyright (C) 2009 Debarshi Ray
 * Copyright (C) 2009 Hubert Figuiere
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


#ifndef __ADDINMANAGER_HPP__
#define __ADDINMANAGER_HPP__

#include <list>
#include <map>
#include <string>

#include <sigc++/signal.h>

#include "sharp/modulemanager.hpp"
#include "addininfo.hpp"
#include "note.hpp"
#include "noteaddin.hpp"
#include "importaddin.hpp"


namespace gnote {

class ApplicationAddin;
class PreferenceTabAddin;
class AddinPreferenceFactoryBase;

namespace sync {
class SyncServiceAddin;
}

typedef std::map<std::string, AddinInfo> AddinInfoMap;


class AddinManager
{
public:
  AddinManager(NoteManager & note_manager, const std::string & conf_dir);
  ~AddinManager();

  void add_note_addin_info(const std::string & id, const sharp::DynamicModule * dmod);
  void erase_note_addin_info(const std::string & id);

  std::string & get_prefs_dir()
    {
      return m_addins_prefs_dir;
    }

  void load_addins_for_note(const Note::Ptr &);
  ApplicationAddin *get_application_addin(const std::string & id) const;
  sync::SyncServiceAddin *get_sync_service_addin(const std::string & id) const;
  void get_preference_tab_addins(std::list<PreferenceTabAddin *> &) const;
  void get_sync_service_addins(std::list<sync::SyncServiceAddin *> &) const;
  void get_import_addins(std::list<ImportAddin*> &) const;
  void initialize_application_addins() const;
  void initialize_sync_service_addins() const;
  void shutdown_application_addins() const;
  void save_addins_prefs() const;

  const AddinInfoMap & get_addin_infos() const
    {
      return m_addin_infos;
    }
  AddinInfo get_addin_info(const std::string & id) const;
  bool is_module_loaded(const std::string & id) const;
  sharp::DynamicModule *get_module(const std::string & id);

  Gtk::Widget * create_addin_preference_widget(const std::string & id);
private:
  void load_addin_infos(const std::string & global_path, const std::string & local_path);
  void load_addin_infos(const std::string & path);
  void get_enabled_addins(std::list<std::string> & addins) const;
  void initialize_sharp_addins();
  void add_module_addins(const std::string & mod_id, sharp::DynamicModule * dmod);
  AddinInfo get_info_for_module(const std::string & module) const;
    
  NoteManager & m_note_manager;
  const std::string m_gnote_conf_dir;
  std::string m_addins_prefs_dir;
  std::string m_addins_prefs_file;
  sharp::ModuleManager m_module_manager;
  std::list<sharp::IfaceFactoryBase*> m_builtin_ifaces;
  AddinInfoMap m_addin_infos;
  /// Key = TypeExtensionNode.Id
  typedef std::map<std::string, ApplicationAddin*> AppAddinMap;
  AppAddinMap                               m_app_addins;
  typedef std::map<std::string, NoteAddin *> IdAddinMap;
  typedef std::map<Note::Ptr, IdAddinMap> NoteAddinMap;
  NoteAddinMap                              m_note_addins;
  /// Key = TypeExtensionNode.Id
  /// the iface factory is not owned by the manager.
  /// TODO: make sure it is removed if the dynamic module is unloaded.
  typedef std::map<std::string, sharp::IfaceFactoryBase*> IdInfoMap;
  IdInfoMap                                m_note_addin_infos;
  typedef std::map<std::string, PreferenceTabAddin*> IdPrefTabAddinMap;
  IdPrefTabAddinMap                        m_pref_tab_addins;
  typedef std::map<std::string, sync::SyncServiceAddin*> IdSyncServiceAddinMap;
  IdSyncServiceAddinMap                    m_sync_service_addins;
  typedef std::map<std::string, ImportAddin *> IdImportAddinMap;
  IdImportAddinMap                         m_import_addins;
  typedef std::map<std::string, AddinPreferenceFactoryBase*> IdAddinPrefsMap;
  IdAddinPrefsMap                          m_addin_prefs;
  sigc::signal<void>         m_application_addin_list_changed;
};

}

#endif

