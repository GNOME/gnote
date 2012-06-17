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


#include <stdexcept>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include "itemdata.hpp"
#include "genericitemdata.hpp"
#include "netitemdata.hpp"
#include "noteitemdata.hpp"


namespace gnome {
namespace keyring {


std::string ItemData::str()
{
  std::string sb;
  for(std::map<std::string, std::string>::iterator iter = attributes.begin();
      iter != attributes.end(); ++iter) {
    sb += boost::str(boost::format("%1%: %2%\n") % iter->first % iter->second);
  }
  return boost::str(boost::format("Keyring: %1% ItemID: %2% Secret: %3%\n%4%")
                    % keyring % item_id % secret % sb);
}

void ItemData::set_values_from_attributes()
{
}

ItemData::Ptr ItemData::get_instance_from_item_type(ItemType type)
{
  switch(type) {
  case GENERIC_SECRET:
    return GenericItemData::create();
  case NETWORK_PASSWORD:
    return NetItemData::create();
  case NOTE:
    return NoteItemData::create();
  default:
    throw std::invalid_argument("Unknown type: " + boost::lexical_cast<std::string>(type));
  }
}

}
}
