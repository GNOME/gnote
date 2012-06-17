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

#include "keyringexception.hpp"
#include "ring.hpp"


namespace gnome {
namespace keyring {

std::vector<ItemData::Ptr> Ring::find(ItemType type, const std::map<std::string, std::string> & atts)
{
  std::vector<ItemData::Ptr> res;
  GList *list = NULL;
  GnomeKeyringAttributeList *attributes = keyring_attributes(atts);
  GnomeKeyringResult result = gnome_keyring_find_items_sync(keyring_item_type(type), attributes, &list);
  gnome_keyring_attribute_list_free(attributes);
  if(result == GNOME_KEYRING_RESULT_OK) {
    for(GList *item = g_list_first(list); item; item = g_list_next(item)) {
      GnomeKeyringFound *found_item = static_cast<GnomeKeyringFound*>(item->data);
      ItemData::Ptr item_data = ItemData::get_instance_from_item_type(type);
      item_data->keyring = found_item->keyring;
      item_data->item_id = found_item->item_id;
      item_data->secret = found_item->secret;
      transfer_attributes(item_data, found_item);
      res.push_back(item_data);
    }
    gnome_keyring_found_list_free(list);
  }
  else if(result != GNOME_KEYRING_RESULT_NO_MATCH) {
    throw KeyringException(result);
  }

  return res;
}

GnomeKeyringItemType Ring::keyring_item_type(ItemType type)
{
  switch(type) {
  case GENERIC_SECRET:
    return GNOME_KEYRING_ITEM_GENERIC_SECRET;
  case NETWORK_PASSWORD:
    return GNOME_KEYRING_ITEM_NETWORK_PASSWORD;
  case NOTE:
  default:
    return GNOME_KEYRING_ITEM_NOTE;
  }
}

GnomeKeyringAttributeList *Ring::keyring_attributes(const std::map<std::string, std::string> & atts)
{
  GnomeKeyringAttributeList *list = gnome_keyring_attribute_list_new();
  for(std::map<std::string, std::string>::const_iterator iter = atts.begin(); iter != atts.end(); ++iter) {
    gnome_keyring_attribute_list_append_string(list, iter->first.c_str(), iter->second.c_str());
  }
  return list;
}

void Ring::transfer_attributes(ItemData::Ptr item_data, GnomeKeyringFound *found_item)
{
  if(!found_item->attributes) {
    return;
  }
  for(unsigned i = 0; i < found_item->attributes->len; ++i) {
    GnomeKeyringAttribute att = g_array_index(found_item->attributes, GnomeKeyringAttribute, i);
    std::string value = att.type == GNOME_KEYRING_ATTRIBUTE_TYPE_STRING
                          ? att.value.string
                          : boost::lexical_cast<std::string>(att.value.integer);
    item_data->attributes[att.name] = value;
  }
}

std::string Ring::default_keyring()
{
  char *keyring = NULL;
  GnomeKeyringResult result = gnome_keyring_get_default_keyring_sync(&keyring);
  if(result == GNOME_KEYRING_RESULT_OK) {
    std::string res;
    if(keyring) {
      res = keyring;
      g_free(keyring);
    }
    return res;
  }
  throw KeyringException(result);
}

int Ring::create_item(const std::string & keyring, ItemType type, const std::string & displayName,
                      const std::map<std::string, std::string> & attributes,
                      const std::string & secret, bool updateIfExists)
{
  GnomeKeyringAttributeList *atts = keyring_attributes(attributes);
  guint32 item_id = 0;
  GnomeKeyringResult result = gnome_keyring_item_create_sync(keyring_c_str(keyring), keyring_item_type(type),
                                                             displayName.c_str(), atts, secret.c_str(),
                                                             updateIfExists ? TRUE : FALSE, &item_id);
  gnome_keyring_attribute_list_free(atts);
  if(result == GNOME_KEYRING_RESULT_OK) {
    return item_id;
  }
  throw KeyringException(result);
}

void Ring::set_item_info(const std::string & keyring, int id, ItemType type,
                         const std::string & displayName, const std::string & secret)
{
  GnomeKeyringItemInfo *info = gnome_keyring_item_info_new();
  gnome_keyring_item_info_set_type(info, keyring_item_type(type));
  gnome_keyring_item_info_set_display_name(info, displayName.c_str());
  gnome_keyring_item_info_set_secret(info, secret.c_str());
  GnomeKeyringResult result = gnome_keyring_item_set_info_sync(keyring_c_str(keyring), id, info);
  gnome_keyring_item_info_free(info);
  if(result != GNOME_KEYRING_RESULT_OK) {
    throw KeyringException(result);
  }
}

void Ring::set_item_attributes(const std::string & keyring, int id, const std::map<std::string, std::string> & atts)
{
  GnomeKeyringAttributeList *attributes = keyring_attributes(atts);
  GnomeKeyringResult result = gnome_keyring_item_set_attributes_sync(keyring_c_str(keyring), id, attributes);
  gnome_keyring_attribute_list_free(attributes);
  if(result != GNOME_KEYRING_RESULT_OK) {
    throw KeyringException(result);
  }
}

const char * Ring::keyring_c_str(const std::string & keyring)
{
  const char *key_ring = NULL;
  if(keyring != "") {
    key_ring = keyring.c_str();
  }
  return key_ring;
}

}
}
