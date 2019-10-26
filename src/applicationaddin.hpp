/*
 * gnote
 *
 * Copyright (C) 2013,2019 Aurimas Cernius
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



#ifndef __APPLICATION_ADDIN_HPP_
#define __APPLICATION_ADDIN_HPP_


#include "abstractaddin.hpp"
#include "notemanager.hpp"


namespace gnote {

class ApplicationAddin
  : public AbstractAddin
{
public:
  static const char * IFACE_NAME;

  ApplicationAddin()
    : m_note_manager(NULL)
  {}

  void initialize(IGnote & ignote, NoteManager & note_manager)
    {
      AbstractAddin::initialize(ignote);
      m_note_manager = &note_manager;
      initialize();
    }

  /// <summary>
  /// Called when Gnote has started up and is nearly 100% initialized.
  /// </summary>
  virtual void initialize () = 0;

  /// <summary>
  /// Called just before Gnote shuts down for good.
  /// </summary>
  virtual void shutdown () = 0;

  /// <summary>
  /// Return true if the addin is initialized
  /// </summary>
  virtual bool initialized () = 0;

  NoteManager & note_manager() const
    {
      return *m_note_manager;
    }
private:
  NoteManager *m_note_manager;
};


}

#endif

