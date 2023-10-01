/*
 * gnote
 *
 * Copyright (C) 2010-2011,2013-2014,2017,2019,2023 Aurimas Cernius
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



#include <glibmm/miscutils.h>

#include "sharp/directory.hpp"
#include "sharp/files.hpp"
#include "debug.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "tomboyimportaddin.hpp"


using gnote::Note;

namespace tomboyimport {

TomboyImportModule::TomboyImportModule()
{
  ADD_INTERFACE_IMPL(TomboyImportAddin);
}



void TomboyImportAddin::initialize()
{
  m_tomboy_path =
    Glib::build_filename(Glib::get_user_data_dir(), "tomboy");

  m_initialized = true;
}


void TomboyImportAddin::shutdown()
{
  m_initialized = false;
}


bool TomboyImportAddin::want_to_run(gnote::NoteManager & )
{
  return sharp::directory_exists(m_tomboy_path);
}


bool TomboyImportAddin::first_run(gnote::NoteManager & manager)
{
  size_t imported = 0;

  DBG_OUT("import path is %s", m_tomboy_path.c_str());

  if(sharp::directory_exists(m_tomboy_path)) {
    std::vector<Glib::ustring> files = sharp::directory_get_files_with_ext(m_tomboy_path, ".note");

    for(auto file_path : files) {
      if(manager.import_note(file_path)) {
        DBG_OUT("success");
        imported++;
      }
    }

    return imported == files.size();
  }

  return true;
}


}
