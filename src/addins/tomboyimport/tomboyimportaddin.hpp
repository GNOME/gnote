/*
 * gnote
 *
 * Copyright (C) 2010,2013,2017,2019 Aurimas Cernius
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



#ifndef __TOMBOY_IMPORT_ADDIN_HPP_
#define __TOMBOY_IMPORT_ADDIN_HPP_

#include "sharp/dynamicmodule.hpp"
#include "importaddin.hpp"


namespace tomboyimport {


class TomboyImportModule
  : public sharp::DynamicModule
{
public:
  TomboyImportModule();
};


DECLARE_MODULE(TomboyImportModule);

class TomboyImportAddin
  : public gnote::ImportAddin
{
public:

  static TomboyImportAddin * create()
    {
      return new TomboyImportAddin;
    }
  virtual void initialize() override;
  virtual void shutdown() override;
  virtual bool want_to_run(gnote::NoteManager & manager) override;
  virtual bool first_run(gnote::NoteManager & manager) override;

private:
  Glib::ustring m_tomboy_path;
};


}


#endif

