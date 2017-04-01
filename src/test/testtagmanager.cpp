/*
 * gnote
 *
 * Copyright (C) 2014,2017 Aurimas Cernius
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


#include "testtagmanager.hpp"

namespace test {

gnote::Tag::Ptr TagManager::get_tag(const Glib::ustring & tag_name) const
{
  auto iter = m_tags.find(tag_name);
  if(iter != m_tags.end()) {
    return iter->second;
  }
  return gnote::Tag::Ptr();
}

gnote::Tag::Ptr TagManager::get_or_create_tag(const Glib::ustring & tag_name)
{
  auto iter = m_tags.find(tag_name);
  if(iter != m_tags.end()) {
    return iter->second;
  }
  gnote::Tag::Ptr tag = gnote::Tag::Ptr(new gnote::Tag(tag_name));
  m_tags[tag_name] = tag;
  return tag;
}

gnote::Tag::Ptr TagManager::get_system_tag(const Glib::ustring & name) const
{
  return get_tag("SYSTEM:" + name);
}

gnote::Tag::Ptr TagManager::get_or_create_system_tag(const Glib::ustring & name)
{
  return get_or_create_tag("SYSTEM:" + name);
}

void TagManager::remove_tag(const gnote::Tag::Ptr & tag)
{
  for(auto iter = m_tags.begin(); iter != m_tags.end(); ++iter) {
    if(iter->second == tag) {
      m_tags.erase(iter);
      break;
    }
  }
}

void TagManager::all_tags(std::list<gnote::Tag::Ptr> & list) const
{
  list.clear();
  for(auto iter = m_tags.begin(); iter != m_tags.end(); ++iter) {
    list.push_back(iter->second);
  }
}

}

