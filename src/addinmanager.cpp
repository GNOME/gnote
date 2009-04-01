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



#include <boost/bind.hpp>

#include "addinmanager.hpp"
#include "watchers.hpp"
#include "notebooks/notebookapplicationaddin.hpp"
#include "notebooks/notebooknoteaddin.hpp"
#include "sharp/foreach.hpp"

namespace gnote {

#define REGISTER_NOTE_ADDIN(klass) \
  m_note_addin_infos.insert(std::make_pair(typeid(klass).name(),        \
  new NoteAddinInfo(sigc::ptr_fun(&klass::create), typeid(klass).name())))

#define REGISTER_APP_ADDIN(klass) \
  m_app_addins.insert(std::make_pair(typeid(klass).name(),        \
                                     klass::create()))

  AddinManager::AddinManager(const std::string & conf_dir)
    : m_gnote_conf_dir(conf_dir)
  {
    initialize_sharp_addins();
  }

  AddinManager::~AddinManager()
  {
    foreach(const AppAddinMap::value_type & value, m_app_addins) {
      delete value.second;
    }
    foreach(const NoteAddinMap::value_type & value, m_note_addins) {
      std::for_each(value.second.begin(), value.second.end(), 
                    boost::bind(&boost::checked_delete<NoteAddin>, _1));
    }
    foreach(const IdInfoMap::value_type & value, m_note_addin_infos) {
      delete value.second;
    }
  }

  void AddinManager::initialize_sharp_addins()
  {
    // get the factory

    REGISTER_NOTE_ADDIN(NoteRenameWatcher);
    REGISTER_NOTE_ADDIN(NoteSpellChecker);
    REGISTER_NOTE_ADDIN(NoteUrlWatcher);
    REGISTER_NOTE_ADDIN(NoteLinkWatcher);
    REGISTER_NOTE_ADDIN(NoteWikiWatcher);
    REGISTER_NOTE_ADDIN(MouseHandWatcher);
    REGISTER_NOTE_ADDIN(NoteTagsWatcher);
    REGISTER_NOTE_ADDIN(notebooks::NotebookNoteAddin);
   
    REGISTER_APP_ADDIN(notebooks::NotebookApplicationAddin);
  }

  void AddinManager::load_addins_for_note(const Note::Ptr & note)
  {
    std::list<NoteAddin *> loaded_addins;
    m_note_addins[note] = loaded_addins;

    std::list<NoteAddin *> & loaded(m_note_addins[note]); // avoid copying the whole list
    foreach(const IdInfoMap::value_type & addin_info, m_note_addin_infos) {
      NoteAddin * addin = addin_info.second->factory();
      addin->initialize(note);
      loaded.push_back(addin);
    }
  }


  std::list<ApplicationAddin*> AddinManager::get_application_addins() const
  {
    std::list<ApplicationAddin*> l;
    foreach(const AppAddinMap::value_type & value, m_app_addins) {
      l.push_back(value.second);
    }
    return l;
  }

}
