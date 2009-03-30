


#ifndef __ADDINMANAGER_HPP__
#define __ADDINMANAGER_HPP__

#include <list>
#include <map>
#include <string>

#include <sigc++/signal.h>

#include "note.hpp"
#include "noteaddin.hpp"

namespace gnote {

  class ApplicationAddin;


class NoteAddinInfo
{
public:
  NoteAddinInfo(const sigc::slot<NoteAddin*> & f,
                const char * id)
    : addin_id(id)
    {
      factory.connect(f);
    }
  sigc::signal<NoteAddin*> factory;
  std::string              addin_id;
};

class AddinManager
{
public:
	AddinManager(const std::string & conf_dir);

	void load_addins_for_note(const Note::Ptr &);
  std::list<ApplicationAddin*> get_application_addins() const;
  /// get_preference_tab_addins();

  sigc::signal<void> & signal_application_addin_list_changed();
private:

  void initialize_sharp_addins();
    
  const std::string m_gnote_conf_dir;
  /// Key = TypeExtensionNode.Id
  typedef std::map<std::string, ApplicationAddin*> AppAddinMap;
  AppAddinMap                               m_app_addins;
  typedef std::map<Note::Ptr, std::list<NoteAddin*> > NoteAddinMap;
  NoteAddinMap                              m_note_addins;
  /// Key = TypeExtensionNode.Id
  typedef std::map<std::string, NoteAddinInfo*> IdInfoMap;
  IdInfoMap                                m_note_addin_infos;
  sigc::signal<void>         m_application_addin_list_changed;
};

}

#endif

