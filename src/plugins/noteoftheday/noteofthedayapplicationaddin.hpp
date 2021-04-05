/*
 * gnote
 *
 * Copyright (C) 2010,2013,2019 Aurimas Cernius
 * Copyright (C) 2009 Debarshi Ray
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

#ifndef __NOTE_OF_THE_DAY_APPLICATION_ADDIN_HPP_
#define __NOTE_OF_THE_DAY_APPLICATION_ADDIN_HPP_

#include <sigc++/sigc++.h>

#include "sharp/dynamicmodule.hpp"
#include "applicationaddin.hpp"

namespace gnote {

class NoteManager;

}

namespace noteoftheday {

class NoteOfTheDayModule
  : public sharp::DynamicModule
{
public:
  NoteOfTheDayModule();
};

class NoteOfTheDayApplicationAddin
  : public gnote::ApplicationAddin
{
public:
  static const char * IFACE_NAME;

  static NoteOfTheDayApplicationAddin * create()
    {
      return new NoteOfTheDayApplicationAddin;
    }

  virtual ~NoteOfTheDayApplicationAddin();
  virtual void initialize() override;
  virtual void shutdown() override;
  virtual bool initialized() override;

private:

  NoteOfTheDayApplicationAddin();
  void check_new_day() const;

  bool m_initialized;
  sigc::connection m_timeout;
};

}

#endif
