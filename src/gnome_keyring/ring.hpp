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


#ifndef _GNOME_KEYRING_RING_HPP_
#define _GNOME_KEYRING_RING_HPP_

#include <vector>

#include <gnome-keyring.h>

#include "itemdata.hpp"


namespace gnome {
namespace keyring {

class Ring
{
public:
  static std::vector<ItemData::Ptr> find(ItemType type, const std::map<std::string, std::string> & atts);
  static std::string default_keyring();
  static int create_item(const std::string & keyring, ItemType type, const std::string & displayName,
                         const std::map<std::string, std::string> & attributes,
                         const std::string & secret, bool updateIfExists);
  static void set_item_info(const std::string & keyring, int id, ItemType type,
                            const std::string & displayName, const std::string & secret);
  static void set_item_attributes(const std::string & keyring, int id, const std::map<std::string, std::string> & atts);
private:
  static GnomeKeyringItemType keyring_item_type(ItemType type);
  static GnomeKeyringAttributeList *keyring_attributes(const std::map<std::string, std::string> & atts);
  static void transfer_attributes(ItemData::Ptr, GnomeKeyringFound*);
  static const char * keyring_c_str(const std::string &);
};

}
}

#endif
