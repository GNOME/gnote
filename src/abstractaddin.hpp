/*
 * gnote
 *
 * Copyright (C) 2010,2012-2013,2019 Aurimas Cernius
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




#ifndef __ABSTRACT_ADDIN_HPP_
#define __ABSTRACT_ADDIN_HPP_

#include <sigc++/trackable.h>

#include "sharp/modulefactory.hpp"

namespace gnote {

class IGnote;


class AbstractAddin
  : public sharp::IInterface
  , public sigc::trackable
{
public:
  AbstractAddin();
  virtual ~AbstractAddin();

  void initialize(IGnote & ignote)
    {
      m_gnote = &ignote;
    }
  void dispose();
  bool is_disposing() const
    { return m_disposing; }

  IGnote & ignote() const
    {
      return const_cast<IGnote&>(*m_gnote);
    }
protected:
  virtual void dispose(bool disposing);

private:
  IGnote *m_gnote;
  bool m_disposing;
};

}



#endif
