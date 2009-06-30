/*
 * gnote
 *
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

#include "sharp/map.hpp"
#include "sharp/directory.hpp"
#include "sharp/dynamicmodule.hpp"

#include "addinmanager.hpp"
#include "addinpreferencefactory.hpp"
#include "debug.hpp"
#include "watchers.hpp"
#include "notebooks/notebookapplicationaddin.hpp"
#include "notebooks/notebooknoteaddin.hpp"


#if 1

// HACK to make sure we link the modules needed only for addins
#include "base/inifile.hpp"
#include "sharp/fileinfo.hpp"
#include "sharp/streamwriter.hpp"
#include "sharp/xsltransform.hpp"
#include "sharp/xsltargumentlist.hpp"

bool links[] = {
  base::IniFileLink_,
  sharp::FileInfoLink_,
  sharp::StreamWriterLink_,
  sharp::XslTransformLink_,
  sharp::XsltArgumentListLink_,
  false
};


#endif


namespace gnote {

#define REGISTER_BUILTIN_NOTE_ADDIN(klass) \
  do { sharp::IfaceFactoryBase *iface = new sharp::IfaceFactory<klass>; \
  m_builtin_ifaces.push_back(iface); \
  m_note_addin_infos.insert(std::make_pair(typeid(klass).name(),iface)); } while(0)

#define REGISTER_APP_ADDIN(klass) \
  m_app_addins.insert(std::make_pair(typeid(klass).name(),        \
                                     klass::create()))

  AddinManager::AddinManager(const std::string & conf_dir)
    : m_gnote_conf_dir(conf_dir)
  {
    m_addins_prefs_dir = Glib::build_filename(conf_dir, "addins");
    initialize_sharp_addins();
  }

  AddinManager::~AddinManager()
  {
    sharp::map_delete_all_second(m_app_addins);
    for(NoteAddinMap::const_iterator iter = m_note_addins.begin();
        iter != m_note_addins.end(); ++iter) {
      std::for_each(iter->second.begin(), iter->second.end(), 
                    boost::bind(&boost::checked_delete<NoteAddin>, _1));
    }
    sharp::map_delete_all_second(m_addin_prefs);
    sharp::map_delete_all_second(m_import_addins);
    for(std::list<sharp::IfaceFactoryBase*>::iterator iter = m_builtin_ifaces.begin();
        iter != m_builtin_ifaces.end(); ++iter) {
      delete *iter;
    }
  }

  void AddinManager::initialize_sharp_addins()
  {

    try {
      if(!sharp::directory_exists(m_addins_prefs_dir)) {
        sharp::directory_create(m_addins_prefs_dir);
      }
    }
    catch(...) {

    }
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

    m_module_manager.add_path(LIBDIR"/"PACKAGE_NAME"/addins/"PACKAGE_VERSION);
    m_module_manager.add_path(m_gnote_conf_dir);

    m_module_manager.load_modules();

    const sharp::ModuleList & list = m_module_manager.get_modules();
    for(sharp::ModuleList::const_iterator iter = list.begin();
        iter != list.end(); ++iter) {

      sharp::DynamicModule* dmod = *iter;
      if(!dmod) {
        continue;
      }

      sharp::IfaceFactoryBase * f = dmod->query_interface(NoteAddin::IFACE_NAME);
      if(f) {
        m_note_addin_infos.insert(std::make_pair(typeid(*f).name(), f));
      }

      f = dmod->query_interface(AddinPreferenceFactoryBase::IFACE_NAME);
      if(f) {
        AddinPreferenceFactoryBase * factory = dynamic_cast<AddinPreferenceFactoryBase*>((*f)());
        m_addin_prefs.insert(std::make_pair((*iter)->id(), factory));
      }

      f = dmod->query_interface(ImportAddin::IFACE_NAME);
      if(f) {
        ImportAddin * factory = dynamic_cast<ImportAddin*>((*f)());
        m_import_addins.insert(std::make_pair((*iter)->id(), factory));
      }
    }
  }

  void AddinManager::load_addins_for_note(const Note::Ptr & note)
  {
    if(m_note_addins.find(note) != m_note_addins.end()) {
      ERR_OUT("trying to load addins when they are already loaded");
      return;
    }
    std::list<NoteAddin *> loaded_addins;
    m_note_addins[note] = loaded_addins;

    std::list<NoteAddin *> & loaded(m_note_addins[note]); // avoid copying the whole list
    for(IdInfoMap::const_iterator iter = m_note_addin_infos.begin();
        iter != m_note_addin_infos.end(); ++iter) {

      const IdInfoMap::value_type & addin_info(*iter); 
      sharp::IInterface* iface = (*addin_info.second)();
      NoteAddin * addin = dynamic_cast<NoteAddin *>(iface);
      if(addin) {
        addin->initialize(note);
        loaded.push_back(addin);
      }
      else {
        DBG_OUT("wrong type for the interface: %s", typeid(*iface).name());
        delete iface;
      }
    }
  }


  void AddinManager::get_application_addins(std::list<ApplicationAddin*> & l) const
  {
    sharp::map_get_values(m_app_addins, l);
  }


  void AddinManager::get_preference_tab_addins(std::list<PreferenceTabAddin *> &l) const
  {
    sharp::map_get_values(m_pref_tab_addins, l);
  }


  void AddinManager::get_import_addins(std::list<ImportAddin*> & l) const
  {
    sharp::map_get_values(m_import_addins, l);
  }


  Gtk::Widget * AddinManager::create_addin_preference_widget(const std::string & id)
  {
    IdAddinPrefsMap::const_iterator iter = m_addin_prefs.find(id);
    if(iter != m_addin_prefs.end()) {
      return iter->second->create_preference_widget();
    }
    return NULL;
  }
}
