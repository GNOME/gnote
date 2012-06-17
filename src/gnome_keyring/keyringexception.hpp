/*
 * gnote
 *
 * Copyright (C) 2012 Aurimas Cernius
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

#include <gnome-keyring.h>



namespace gnome {
namespace keyring {

class KeyringException
  : public std::exception
{
public:
  KeyringException(GnomeKeyringResult);
  GnomeKeyringResult result() const
    {
      return m_result;
    }
  virtual const char * what() const throw()
    {
      return m_what;
    }
private:
  static const char * get_msg(GnomeKeyringResult);

  GnomeKeyringResult m_result;
  const char * m_what;
};

}
}

#endif
