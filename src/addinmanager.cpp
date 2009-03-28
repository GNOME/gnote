



#include "addinmanager.hpp"
#include "watchers.hpp"
#include "sharp/foreach.hpp"

namespace gnote {

#define REGISTER_NOTE_ADDIN(klass) \
  m_note_addin_infos.insert(std::make_pair(typeid(klass).name(),        \
  new NoteAddinInfo(sigc::ptr_fun(&klass::create), typeid(klass).name())))

  AddinManager::AddinManager(const std::string & conf_dir)
    : m_gnote_conf_dir(conf_dir)
  {
    initialize_sharp_addins();
  }


  void AddinManager::initialize_sharp_addins()
  {
    // get the factory

    REGISTER_NOTE_ADDIN(NoteRenameWatcher);
    REGISTER_NOTE_ADDIN(NoteSpellChecker);
    REGISTER_NOTE_ADDIN(NoteUrlWatcher);
    REGISTER_NOTE_ADDIN(NoteLinkWatcher);
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
}
