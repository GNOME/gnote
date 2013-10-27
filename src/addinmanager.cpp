/*
 * gnote
 *
 * Copyright (C) 2010-2013 Aurimas Cernius
 * Copyright (C) 2009, 2010 Debarshi Ray
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


#include <config.h>

#include <boost/bind.hpp>
#include <boost/checked_delete.hpp>

#include <glib.h>
#include <glibmm/i18n.h>

#include "sharp/map.hpp"
#include "sharp/directory.hpp"
#include "sharp/dynamicmodule.hpp"
#include "sharp/files.hpp"

#include "addinmanager.hpp"
#include "addinpreferencefactory.hpp"
#include "debug.hpp"
#include "ignote.hpp"
#include "watchers.hpp"
#include "notebooks/notebookapplicationaddin.hpp"
#include "notebooks/notebooknoteaddin.hpp"
#include "synchronization/syncserviceaddin.hpp"




namespace gnote {

#define REGISTER_BUILTIN_NOTE_ADDIN(klass) \
  do { sharp::IfaceFactoryBase *iface = new sharp::IfaceFactory<klass>; \
  m_builtin_ifaces.push_back(iface); \
  m_note_addin_infos.insert(std::make_pair(typeid(klass).name(),iface)); } while(0)

#define REGISTER_APP_ADDIN(klass) \
  m_app_addins.insert(std::make_pair(typeid(klass).name(),        \
                                     klass::create()))

  AddinManager::AddinManager(NoteManager & note_manager, const std::string & conf_dir)
    : m_note_manager(note_manager)
    , m_gnote_conf_dir(conf_dir)
  {
    m_addins_prefs_dir = Glib::build_filename(conf_dir, "addins");
    m_addins_prefs_file = Glib::build_filename(m_addins_prefs_dir,
                                               "global.ini");

    const bool is_first_run
                 = !sharp::directory_exists(m_addins_prefs_dir);

    if (is_first_run)
      g_mkdir_with_parents(m_addins_prefs_dir.c_str(), S_IRWXU);

    initialize_sharp_addins();
  }

  AddinManager::~AddinManager()
  {
    sharp::map_delete_all_second(m_app_addins);
    for(NoteAddinMap::const_iterator iter = m_note_addins.begin();
        iter != m_note_addins.end(); ++iter) {
      sharp::map_delete_all_second(iter->second);
    }
    sharp::map_delete_all_second(m_addin_prefs);
    sharp::map_delete_all_second(m_import_addins);
    for(std::list<sharp::IfaceFactoryBase*>::iterator iter = m_builtin_ifaces.begin();
        iter != m_builtin_ifaces.end(); ++iter) {
      delete *iter;
    }
  }

  void AddinManager::add_note_addin_info(const std::string & id,
                                         const sharp::DynamicModule * dmod)
  {
    {
      const IdInfoMap::const_iterator iter
                                        = m_note_addin_infos.find(id);
      if (m_note_addin_infos.end() != iter) {
        ERR_OUT(_("Note add-in info %s already present"), id.c_str());
        return;
      }
    }

    sharp::IfaceFactoryBase * const f = dmod->query_interface(
                                          NoteAddin::IFACE_NAME);
    if(!f) {
      ERR_OUT(_("%s does not implement %s"), id.c_str(), NoteAddin::IFACE_NAME);
      return;
    }

    m_note_addin_infos.insert(std::make_pair(id, f));

    {
      for(NoteAddinMap::iterator iter = m_note_addins.begin();
          iter != m_note_addins.end(); ++iter) {
        IdAddinMap & id_addin_map = iter->second;
        IdAddinMap::const_iterator it = id_addin_map.find(id);
        if (id_addin_map.end() != it) {
          ERR_OUT(_("Note add-in %s already present"), id.c_str());
          continue;
        }

        const Note::Ptr & note = iter->first;
        NoteAddin * const addin = dynamic_cast<NoteAddin *>((*f)());
        if (addin) {
         addin->initialize(note);
         id_addin_map.insert(std::make_pair(id, addin));
        }
      }
    }
  }

  void AddinManager::erase_note_addin_info(const std::string & id)
  {
    {
      const IdInfoMap::iterator iter = m_note_addin_infos.find(id);
      if (m_note_addin_infos.end() == iter) {
        ERR_OUT(_("Note add-in info %s is absent"), id.c_str());
        return;
      }

      m_note_addin_infos.erase(iter);
    }

    {
      for(NoteAddinMap::iterator iter = m_note_addins.begin();
          iter != m_note_addins.end(); ++iter) {
        IdAddinMap & id_addin_map = iter->second;
        IdAddinMap::iterator it = id_addin_map.find(id);
        if (id_addin_map.end() == it) {
          ERR_OUT(_("Note add-in %s is absent"), id.c_str());
          continue;
        }

        NoteAddin * const addin = it->second;
        if (addin) {
          addin->dispose(true);
          delete addin;
          id_addin_map.erase(it);
        }
      }
    }
  }

  void AddinManager::load_addin_infos(const std::string & global_path,
                                      const std::string & local_path)
  {
    load_addin_infos(global_path);
    load_addin_infos(local_path);
  }

  void AddinManager::load_addin_infos(const std::string & path)
  {
    std::list<std::string> files;
    sharp::directory_get_files_with_ext(path, ".desktop", files);
    for(std::list<std::string>::iterator iter = files.begin(); iter != files.end(); ++iter) {
      try {
        AddinInfo addin_info(*iter);
        std::string module = Glib::build_filename(path, addin_info.addin_module());
        if(sharp::file_exists(module + "." + G_MODULE_SUFFIX)) {
          addin_info.addin_module(module);
          m_addin_infos[addin_info.id()] = addin_info;
        }
        else {
          ERR_OUT(_("Failed to find module %s for addin %s"), addin_info.id().c_str(), module.c_str());
        }
      }
      catch(std::exception & e) {
        ERR_OUT(_("Failed to load addin info for %s: %s"), iter->c_str(), e.what());
      }
    }
  }

  void AddinManager::get_enabled_addins(std::list<std::string> & addins) const
  {
    bool global_addins_prefs_loaded = true;
    Glib::KeyFile global_addins_prefs;
    try {
      global_addins_prefs.load_from_file(m_addins_prefs_file);
    }
    catch(Glib::Error & not_loaded) {
      global_addins_prefs_loaded = false;
    }

    for(AddinInfoMap::const_iterator iter = m_addin_infos.begin(); iter != m_addin_infos.end(); ++iter) {
      if(global_addins_prefs_loaded && global_addins_prefs.has_key("Enabled", iter->first)) {
        if(global_addins_prefs.get_boolean("Enabled", iter->first)) {
          addins.push_back(iter->second.addin_module());
        }
      }
      else if(iter->second.default_enabled()) {
          addins.push_back(iter->second.addin_module());
      }
    }
  }

  void AddinManager::initialize_sharp_addins()
  {
    if (!sharp::directory_exists (m_addins_prefs_dir))
      g_mkdir_with_parents(m_addins_prefs_dir.c_str(), S_IRWXU);

    // get the factory

    REGISTER_BUILTIN_NOTE_ADDIN(NoteRenameWatcher);
    REGISTER_BUILTIN_NOTE_ADDIN(NoteSpellChecker);
    REGISTER_BUILTIN_NOTE_ADDIN(NoteUrlWatcher);
    REGISTER_BUILTIN_NOTE_ADDIN(NoteLinkWatcher);
    REGISTER_BUILTIN_NOTE_ADDIN(NoteWikiWatcher);
    REGISTER_BUILTIN_NOTE_ADDIN(MouseHandWatcher);
    REGISTER_BUILTIN_NOTE_ADDIN(NoteTagsWatcher);
    REGISTER_BUILTIN_NOTE_ADDIN(notebooks::NotebookNoteAddin);
   
    REGISTER_APP_ADDIN(notebooks::NotebookApplicationAddin);

    std::string global_path = LIBDIR "/" PACKAGE_NAME "/addins/" PACKAGE_VERSION;
    std::string local_path = m_gnote_conf_dir + "/addins";

    load_addin_infos(global_path, local_path);
    std::list<std::string> enabled_addins;
    get_enabled_addins(enabled_addins);
    m_module_manager.load_modules(enabled_addins);

    const sharp::ModuleMap & modules = m_module_manager.get_modules();
    for(sharp::ModuleMap::const_iterator iter = modules.begin();
        iter != modules.end(); ++iter) {

      const std::string & mod_id = get_info_for_module(iter->first).id();
      sharp::DynamicModule* dmod = iter->second;
      if(!dmod) {
        continue;
      }

      dmod->enabled(true); // enable all loaded modules on startup
      add_module_addins(mod_id, dmod);
    }
  }

  void AddinManager::add_module_addins(const std::string & mod_id, sharp::DynamicModule * dmod)
  {
    sharp::IfaceFactoryBase * f = dmod->query_interface(NoteAddin::IFACE_NAME);
    if(f && dmod->is_enabled()) {
      m_note_addin_infos.insert(std::make_pair(mod_id, f));
    }

    f = dmod->query_interface(AddinPreferenceFactoryBase::IFACE_NAME);
    if(f) {
      AddinPreferenceFactoryBase * factory = dynamic_cast<AddinPreferenceFactoryBase*>((*f)());
      m_addin_prefs.insert(std::make_pair(mod_id, factory));
    }

    f = dmod->query_interface(ImportAddin::IFACE_NAME);
    if(f) {
      ImportAddin * addin = dynamic_cast<ImportAddin*>((*f)());
      m_import_addins.insert(std::make_pair(mod_id, addin));
    }

    f = dmod->query_interface(ApplicationAddin::IFACE_NAME);
    if(f) {
      ApplicationAddin * addin = dynamic_cast<ApplicationAddin*>((*f)());
      addin->note_manager(m_note_manager);
      m_app_addins.insert(std::make_pair(mod_id, addin));
    }
    f = dmod->query_interface(sync::SyncServiceAddin::IFACE_NAME);
    if(f) {
      sync::SyncServiceAddin * addin = dynamic_cast<sync::SyncServiceAddin*>((*f)());
      m_sync_service_addins.insert(std::make_pair(mod_id, addin));
    }
  }

  AddinInfo AddinManager::get_info_for_module(const std::string & module) const
  {
    for(AddinInfoMap::const_iterator iter = m_addin_infos.begin();
        iter != m_addin_infos.end(); ++iter) {
      if(iter->second.addin_module() == module) {
        return iter->second;
      }
    }
    return AddinInfo();
  }

  void AddinManager::load_addins_for_note(const Note::Ptr & note)
  {
    if(m_note_addins.find(note) != m_note_addins.end()) {
      ERR_OUT(_("Trying to load addins when they are already loaded"));
      return;
    }
    IdAddinMap loaded_addins;
    m_note_addins[note] = loaded_addins;

    IdAddinMap & loaded(m_note_addins[note]); // avoid copying the whole map
    for(IdInfoMap::const_iterator iter = m_note_addin_infos.begin();
        iter != m_note_addin_infos.end(); ++iter) {

      const IdInfoMap::value_type & addin_info(*iter); 
      sharp::IInterface* iface = (*addin_info.second)();
      NoteAddin * addin = dynamic_cast<NoteAddin *>(iface);
      if(addin) {
        addin->initialize(note);
        loaded.insert(std::make_pair(addin_info.first, addin));
      }
      else {
        DBG_OUT("wrong type for the interface: %s", typeid(*iface).name());
        delete iface;
      }
    }
  }

  ApplicationAddin * AddinManager::get_application_addin(
                                     const std::string & id) const
  {
    const IdImportAddinMap::const_iterator import_iter
      = m_import_addins.find(id);

    if (m_import_addins.end() != import_iter)
      return import_iter->second;

    const AppAddinMap::const_iterator app_iter
      = m_app_addins.find(id);

    if (m_app_addins.end() != app_iter)
      return app_iter->second;

    return 0;
  }

  sync::SyncServiceAddin *AddinManager::get_sync_service_addin(const std::string & id) const
  {
    const IdSyncServiceAddinMap::const_iterator iter = m_sync_service_addins.find(id);
    if(iter != m_sync_service_addins.end()) {
      return iter->second;
    }

    return NULL;
  }

  void AddinManager::get_preference_tab_addins(std::list<PreferenceTabAddin *> &l) const
  {
    sharp::map_get_values(m_pref_tab_addins, l);
  }


  void AddinManager::get_sync_service_addins(std::list<sync::SyncServiceAddin *> &l) const
  {
    sharp::map_get_values(m_sync_service_addins, l);
  }


  void AddinManager::get_import_addins(std::list<ImportAddin*> & l) const
  {
    sharp::map_get_values(m_import_addins, l);
  }

  void AddinManager::initialize_application_addins() const
  {
    for(AppAddinMap::const_iterator iter = m_app_addins.begin();
        iter != m_app_addins.end(); ++iter) {
      ApplicationAddin * addin = iter->second;
      addin->note_manager(m_note_manager);
      const sharp::DynamicModule * dmod
        = m_module_manager.get_module(iter->first);
      if (!dmod || dmod->is_enabled()) {
        addin->initialize();
      }
    }
  }

  void AddinManager::initialize_sync_service_addins() const
  {
    for(IdSyncServiceAddinMap::const_iterator iter = m_sync_service_addins.begin();
        iter != m_sync_service_addins.end(); ++iter) {
      sync::SyncServiceAddin *addin = NULL;
      try {
        addin = iter->second;
        const sharp::DynamicModule *dmod = m_module_manager.get_module(iter->first);
        if(!dmod || dmod->is_enabled()) {
          addin->initialize();
        }
      }
      catch(std::exception & e) {
        DBG_OUT("Error calling %s.initialize (): %s", addin->id().c_str(), e.what());

        // TODO: Call something like AddinManager.Disable (addin)
      }
    }
  }

  void AddinManager::shutdown_application_addins() const
  {
    for(AppAddinMap::const_iterator iter = m_app_addins.begin();
        iter != m_app_addins.end(); ++iter) {
      ApplicationAddin * addin = iter->second;
      const sharp::DynamicModule * dmod
        = m_module_manager.get_module(iter->first);
      if (!dmod || dmod->is_enabled()) {
        try {
          addin->shutdown();
        }
        catch (const sharp::Exception & e) {
          DBG_OUT("Error calling %s.Shutdown (): %s",
                  typeid(*addin).name(), e.what());
        }
      }
    }
  }

  void AddinManager::save_addins_prefs() const
  {
    Glib::KeyFile global_addins_prefs;
    try {
      global_addins_prefs.load_from_file(m_addins_prefs_file);
    }
    catch (Glib::Error & not_loaded_ignored) {
    }

    const sharp::ModuleMap & modules = m_module_manager.get_modules();
    for(AddinInfoMap::const_iterator iter = m_addin_infos.begin();
        iter != m_addin_infos.end(); ++iter) {
      const std::string & mod_id = iter->first;
      sharp::ModuleMap::const_iterator mod_iter = modules.find(iter->second.addin_module());
      bool enabled = mod_iter != modules.end() && mod_iter->second->is_enabled();
      global_addins_prefs.set_boolean("Enabled", mod_id, enabled);
    }

    Glib::RefPtr<Gio::File> prefs_file = Gio::File::create_for_path(
                                           m_addins_prefs_file);
    Glib::RefPtr<Gio::FileOutputStream> prefs_file_stream
                                          = prefs_file->append_to();
    prefs_file_stream->truncate(0);
    prefs_file_stream->write(global_addins_prefs.to_data());
  }

  AddinInfo AddinManager::get_addin_info(const std::string & id) const
  {
    AddinInfoMap::const_iterator iter = m_addin_infos.find(id);
    if(iter != m_addin_infos.end()) {
      return iter->second;
    }
    return AddinInfo();
  }

  bool AddinManager::is_module_loaded(const std::string & id) const
  {
    AddinInfo info = get_addin_info(id);
    return m_module_manager.get_module(info.addin_module());
  }

  sharp::DynamicModule *AddinManager::get_module(const std::string & id)
  {
    AddinInfo info = get_addin_info(id);
    sharp::DynamicModule *module = m_module_manager.get_module(info.addin_module());
    if(!module) {
      module = m_module_manager.load_module(info.addin_module());
      if(module) {
        add_module_addins(id, module);
      }
    }
    return module;
  }

  Gtk::Widget * AddinManager::create_addin_preference_widget(const std::string & id)
  {
    IdAddinPrefsMap::const_iterator iter = m_addin_prefs.find(id);
    if(iter != m_addin_prefs.end()) {
      return iter->second->create_preference_widget(m_note_manager);
    }
    return NULL;
  }
}
