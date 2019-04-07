/*
 * gnote
 *
 * Copyright (C) 2013,2019 Aurimas Cernius
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

#ifndef __SPECIAL_NOTES_APPLICATION_ADDIN_HPP_
#define __SPECIAL_NOTES_APPLICATION_ADDIN_HPP_

#include "notebooks/notebook.hpp"
#include "sharp/dynamicmodule.hpp"
#include "applicationaddin.hpp"

namespace specialnotes {

class SpecialNotesModule
  : public sharp::DynamicModule
{
public:
  SpecialNotesModule();
};


class SpecialNotesApplicationAddin
  : public gnote::ApplicationAddin
{
public:
  static const char * IFACE_NAME;

  static SpecialNotesApplicationAddin *create()
    {
      return new SpecialNotesApplicationAddin;
    }

  virtual ~SpecialNotesApplicationAddin();
  virtual void initialize() override;
  virtual void shutdown() override;
  virtual bool initialized() override;
private:
  SpecialNotesApplicationAddin();

  bool m_initialized;
  gnote::notebooks::Notebook::Ptr m_notebook;
};

}

#endif
