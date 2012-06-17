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


#include <boost/lexical_cast.hpp>
#include "netitemdata.hpp"

namespace gnome {
namespace keyring {

void NetItemData::set_values_from_attributes()
{
  user = get_attribute_value("user");
  domain = get_attribute_value("domain");
  server = get_attribute_value("server");
  obj = get_attribute_value("object");
  protocol = get_attribute_value("protocol");
  auth_type = get_attribute_value("authtype");
  std::string port_num = get_attribute_value("port");
  if(port_num != "") {
    port = boost::lexical_cast<int>(port_num);
  }

  ItemData::set_values_from_attributes();
}


std::string NetItemData::get_attribute_value(const std::string & att_name) const
{
  std::map<std::string, std::string>::const_iterator iter = attributes.find(att_name);
  if(iter == attributes.end()) {
    return "";
  }

  return iter->second;
}

}
}
