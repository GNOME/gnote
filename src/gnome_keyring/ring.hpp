/*
 * gnote
 *
 * Copyright (C) 2012,2017 Aurimas Cernius
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


#ifndef _GNOME_KEYRING_RING_HPP_
#define _GNOME_KEYRING_RING_HPP_

#include <vector>
#include <map>

#include <glibmm/ustring.h>
#include <libsecret/secret.h>



namespace gnome {
namespace keyring {

class Ring
{
public:
  static Glib::ustring find_password(const std::map<Glib::ustring, Glib::ustring> & atts);
  static Glib::ustring default_keyring();
  static void create_password(const Glib::ustring & keyring, const Glib::ustring & displayName,
                              const std::map<Glib::ustring, Glib::ustring> & attributes,
                              const Glib::ustring & secret);
  static void clear_password(const std::map<Glib::ustring, Glib::ustring> & attributes);
private:
  static GHashTable *keyring_attributes(const std::map<Glib::ustring, Glib::ustring> & atts);

  static SecretSchema s_schema;
};

}
}

#endif
