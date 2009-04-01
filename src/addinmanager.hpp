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
  ~AddinManager();

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

