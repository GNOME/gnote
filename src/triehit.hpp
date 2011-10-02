/*
 * gnote
 *
 * Copyright (C) 2011 Debarshi Ray
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

#ifndef __TRIEHIT_HPP_
#define __TRIEHIT_HPP_

#include <list>
#include <tr1/memory>

#include <glibmm.h>

namespace gnote {

template<class value_t>
class TrieHit
{
public:
  typedef std::tr1::shared_ptr<TrieHit> Ptr;
  typedef std::list<Ptr> List;
  typedef std::tr1::shared_ptr<List> ListPtr;

  TrieHit(int s, int e, const Glib::ustring & k, const value_t & v)
    : m_start(s)
    , m_end(e)
    , m_key(k)
    , m_value(v)
    {
    }

  int start() const
  {
    return m_start;
  }

  int end() const
  {
    return m_end;
  }

  Glib::ustring key() const
  {
    return m_key;
  }

  value_t value() const
  {
    return m_value;
  }

private:

  int           m_start;
  int           m_end;
  Glib::ustring m_key;
  value_t       m_value;
};

}

#endif
