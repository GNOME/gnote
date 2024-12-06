/*
 * gnote
 *
 * Copyright (C) 2013,2017,2019,2021-2022,2024 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
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




#ifndef __TAG_MANAGER_HPP_
#define __TAG_MANAGER_HPP_


#include <mutex>
#include <sigc++/signal.h>

#include "itagmanager.hpp"
#include "tag.hpp"


namespace gnote {

class TagManager
  : public ITagManager
{
public:
  TagManager();

  Tag::ORef get_tag(const Glib::ustring & tag_name) const override;
  Tag &get_or_create_tag(const Glib::ustring &) override;
  Tag::ORef get_system_tag(const Glib::ustring & tag_name) const override;
  Tag &get_or_create_system_tag(const Glib::ustring & name) override;
  void remove_tag(Tag &tag) override;
  std::vector<Tag::Ref> all_tags() const override;
private:
  typedef std::unique_ptr<Tag> TagPtr;
  std::vector<TagPtr> m_tags;
  typedef std::map<Glib::ustring, TagPtr> InternalMap;
  InternalMap                      m_internal_tags;
  mutable std::mutex               m_locker;
};

}

#endif
