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


#include "keyringexception.hpp"

namespace gnome {
namespace keyring {

KeyringException::KeyringException(GnomeKeyringResult code)
  : m_result(code)
  , m_what(get_msg(code))
{
}

const char * KeyringException::get_msg(GnomeKeyringResult code)
{
  switch(code) {
  case GNOME_KEYRING_RESULT_OK:
    return "Success";
  case GNOME_KEYRING_RESULT_DENIED:
    return "Access denied";
  case GNOME_KEYRING_RESULT_NO_KEYRING_DAEMON:
    return "The keyring daemon is not available";
  case GNOME_KEYRING_RESULT_ALREADY_UNLOCKED:
    return "Keyring was already unlocked";
  case GNOME_KEYRING_RESULT_NO_SUCH_KEYRING:
    return "No such keyring";
  case GNOME_KEYRING_RESULT_BAD_ARGUMENTS:
    return "Bad arguments";
  case GNOME_KEYRING_RESULT_IO_ERROR:
    return "I/O error";
  case GNOME_KEYRING_RESULT_CANCELLED:
    return "Operation canceled";
  case GNOME_KEYRING_RESULT_KEYRING_ALREADY_EXISTS:
    return "Item already exists";
  case GNOME_KEYRING_RESULT_NO_MATCH:
    return "No match";
  default:
    return "Unknown error";
  }
}

}
}
