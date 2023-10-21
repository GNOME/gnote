/*
 * gnote
 *
 * Copyright (C) 2023 Aurimas Cernius
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


#ifndef __HASH_HPP__
#define __HASH_HPP__

#include <string_view>

#include <glibmm/ustring.h>


namespace gnote {

template <typename T>
class Hash
{
};

template <>
class Hash<Glib::ustring>
{
public:
  std::size_t operator()(const Glib::ustring & s) const noexcept
  {
    std::string_view sv(s.c_str(), s.bytes());
    return m_hash(sv);
  }
private:
  std::hash<std::string_view> m_hash;
};

}

#endif

