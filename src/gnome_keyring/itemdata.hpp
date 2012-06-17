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


#ifndef _GNOME_KEYRING_ITEMDATA_HPP_
#define _GNOME_KEYRING_ITEMDATA_HPP_

#include <map>
#include <string>
#include <tr1/memory>


namespace gnome {
namespace keyring {

enum ItemType {
  GENERIC_SECRET,
  NETWORK_PASSWORD,
  NOTE
};

class ItemData
{
public:
  friend class Ring;
  typedef std::tr1::shared_ptr<ItemData> Ptr;

  std::string keyring;
  int item_id;
  std::string secret;
  std::map<std::string, std::string> attributes;

  virtual ItemType type() const = 0;

  virtual std::string str();
protected:
  virtual void set_values_from_attributes();

  static Ptr get_instance_from_item_type(ItemType type);
};

}
}

#endif
