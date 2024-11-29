/*
 * gnote
 *
 * Copyright (C) 2014,2017-2020,2022,2024 Aurimas Cernius
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


#include <algorithm>

#include "testtagmanager.hpp"

namespace test {

gnote::Tag::ORef TagManager::get_tag(const Glib::ustring & tag_name) const
{
  auto iter = std::find_if(m_tags.begin(), m_tags.end(), [&tag_name](const gnote::Tag::Ptr &tag) { return tag->name() == tag_name; });
  if(iter != m_tags.end()) {
    return **iter;
  }
  return gnote::Tag::ORef();
}

gnote::Tag &TagManager::get_or_create_tag(const Glib::ustring & tag_name)
{
  if(auto tag = get_tag(tag_name)) {
    return *tag;
  }
  gnote::Tag::Ptr tag = gnote::Tag::Ptr(new gnote::Tag(Glib::ustring(tag_name)));
  m_tags.push_back(tag);
  return *tag;
}

gnote::Tag::ORef TagManager::get_system_tag(const Glib::ustring & name) const
{
  if(auto tag = get_tag("SYSTEM:" + name)) {
    return *tag;
  }
  return gnote::Tag::ORef();
}

gnote::Tag &TagManager::get_or_create_system_tag(const Glib::ustring & name)
{
  return get_or_create_tag("SYSTEM:" + name);
}

void TagManager::remove_tag(gnote::Tag &tag)
{
  auto iter = std::find_if(m_tags.begin(), m_tags.end(), [&tag](const gnote::Tag::Ptr &t) { return t.get() == &tag; });
  if(iter != m_tags.end()) {
    m_tags.erase(iter);
  }
}

std::vector<gnote::Tag::Ref> TagManager::all_tags() const
{
  std::vector<gnote::Tag::Ref> tags;
  tags.reserve(m_tags.size());
  for(auto &tag : m_tags) {
    tags.emplace_back(*tag);
  }
  return tags;
}

}

