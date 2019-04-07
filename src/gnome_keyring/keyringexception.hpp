/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017,2019 Aurimas Cernius
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


#ifndef _GNOME_KEYRING_KEYRINGEXCEPTION_HPP_
#define _GNOME_KEYRING_KEYRINGEXCEPTION_HPP_

#include <stdexcept>

#include <glibmm/ustring.h>


namespace gnome {
namespace keyring {

class KeyringException
  : public std::exception
{
public:
  KeyringException(const Glib::ustring & msg)
    : m_what(msg)
    {}
  virtual ~KeyringException() noexcept override {}
  virtual const char *what() const noexcept override
    {
      return m_what.c_str();
    }
private:
  Glib::ustring m_what;
};

}
}

#endif
