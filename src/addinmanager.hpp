/*
 * gnote
 *
 * Copyright (C) 2010,2012-2015,2017,2019-2020,2022-2023 Aurimas Cernius
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

#include <map>

#include <sigc++/signal.h>

#include "base/hash.hpp"
#include "sharp/modulemanager.hpp"
#include "addininfo.hpp"
#include "note.hpp"
#include "noteaddin.hpp"
#include "importaddin.hpp"
#include "preferences.hpp"


namespace gnote {

class ApplicationAddin;
class AddinPreferenceFactoryBase;

namespace sync {
class SyncServiceAddin;
}

typedef std::map<Glib::ustring, AddinInfo> AddinInfoMap;


class AddinManager
{
public:
  AddinManager(IGnote & g, NoteManager & note_manager, Preferences & preferences, const Glib::ustring & conf_dir);
  ~AddinManager();

  void add_note_addin_info(Glib::ustring && id, const sharp::DynamicModule * dmod);
  void erase_note_addin_info(const Glib::ustring & id);

  Glib::ustring & get_prefs_dir()
    {
      return m_addins_prefs_dir;
    }

  void load_addins_for_note(NoteBase &);
  std::vector<NoteAddin*> get_note_addins(const NoteBase &) const;
  ApplicationAddin *get_application_addin(const Glib::ustring & id) const;
  sync::SyncServiceAddin *get_sync_service_addin(const Glib::ustring & id) const;
  std::vector<sync::SyncServiceAddin*> get_sync_service_addins() const;
  std::vector<ImportAddin*> get_import_addins() const;
  void initialize_application_addins() const;
  void initialize_sync_service_addins() const;
  void shutdown_application_addins() const;
  void save_addins_prefs() const;

  const AddinInfoMap & get_addin_infos() const
    {
      return m_addin_infos;
    }
  AddinInfo get_addin_info(const Glib::ustring & id) const;
  AddinInfo get_addin_info(const AbstractAddin & addin) const;
  bool is_module_loaded(const Glib::ustring & id) const;
  sharp::DynamicModule *get_module(const Glib::ustring & id);

  Gtk::Widget *create_addin_preference_widget(const Glib::ustring & id);
private:
  void load_addin_infos(const Glib::ustring & global_path, const Glib::ustring & local_path);
  void load_addin_infos(const Glib::ustring & path);
  void load_note_addin(Glib::ustring && id, sharp::IfaceFactoryBase *const f);
  std::vector<Glib::ustring> get_enabled_addins() const;
  void initialize_sharp_addins();
  void add_module_addins(const Glib::ustring & mod_id, sharp::DynamicModule * dmod);
  AddinInfo get_info_for_module(const Glib::ustring & module) const;
  void register_addin_actions() const;
    
  IGnote & m_gnote;
  NoteManager & m_note_manager;
  Preferences & m_preferences;
  const Glib::ustring m_gnote_conf_dir;
  Glib::ustring m_addins_prefs_dir;
  Glib::ustring m_addins_prefs_file;
  sharp::ModuleManager m_module_manager;
  std::vector<std::unique_ptr<sharp::IfaceFactoryBase>> m_builtin_ifaces;
  AddinInfoMap m_addin_infos;
  /// Key = TypeExtensionNode.Id
  typedef std::map<Glib::ustring, std::unique_ptr<ApplicationAddin>> AppAddinMap;
  AppAddinMap                               m_app_addins;
  typedef std::map<Glib::ustring, std::unique_ptr<NoteAddin>> IdAddinMap;
  typedef std::unordered_map<Glib::ustring, IdAddinMap, Hash<Glib::ustring>> NoteAddinMap;
  NoteAddinMap                              m_note_addins;
  /// Key = TypeExtensionNode.Id
  /// the iface factory is not owned by the manager.
  /// TODO: make sure it is removed if the dynamic module is unloaded.
  typedef std::map<Glib::ustring, sharp::IfaceFactoryBase*> IdInfoMap;
  IdInfoMap                                m_note_addin_infos;
  typedef std::map<Glib::ustring, std::unique_ptr<sync::SyncServiceAddin>> IdSyncServiceAddinMap;
  IdSyncServiceAddinMap                    m_sync_service_addins;
  typedef std::map<Glib::ustring, std::unique_ptr<ImportAddin>> IdImportAddinMap;
  IdImportAddinMap                         m_import_addins;
  typedef std::map<Glib::ustring, std::unique_ptr<AddinPreferenceFactoryBase>> IdAddinPrefsMap;
  IdAddinPrefsMap                          m_addin_prefs;
  sigc::signal<void()> m_application_addin_list_changed;
};

}

#endif

