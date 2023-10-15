/*
 * gnote
 *
 * Copyright (C) 2013,2017,2019,2022-2023 Aurimas Cernius
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

#include <memory>
#include <utility>

#include <glibmm/ustring.h>

namespace gnote {

template<class value_t>
class TrieHit
{
public:
  typedef std::vector<TrieHit> List;

  TrieHit(int s, int e, Glib::ustring && k, value_t && v)
    : m_start(s)
    , m_end(e)
    , m_key(std::forward<Glib::ustring>(k))
    , m_value(std::forward<value_t>(v))
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
