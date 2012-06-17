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


#ifndef _GNOME_KEYRING_NETITEMDATA_HPP_
#define _GNOME_KEYRING_NETITEMDATA_HPP_

#include "itemdata.hpp"


namespace gnome {
namespace keyring {

class NetItemData
  : public ItemData
{
public:
  static ItemData::Ptr create()
    {
      return ItemData::Ptr(new NetItemData);
    }

  std::string user;
  std::string domain;
  std::string server;
  std::string obj;
  std::string protocol;
  std::string auth_type;
  int port;

  virtual ItemType type() const
    {
      return NETWORK_PASSWORD;
    }
protected:
  virtual void set_values_from_attributes();
private:
  std::string get_attribute_value(const std::string & att_name) const;
};

}
}

#endif
