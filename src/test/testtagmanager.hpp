/*
 * gnote
 *
 * Copyright (C) 2014,2017-2019,2024 Aurimas Cernius
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


#ifndef _TESTTAGMANAGER_HPP_
#define _TESTTAGMANAGER_HPP_

#include "itagmanager.hpp"

namespace test {

class TagManager
  : public gnote::ITagManager
{
public:
  gnote::Tag::ORef get_tag(const Glib::ustring & tag_name) const override;
  gnote::Tag &get_or_create_tag(const Glib::ustring &) override;
  gnote::Tag::ORef get_system_tag(const Glib::ustring & tag_name) const override;
  gnote::Tag &get_or_create_system_tag(const Glib::ustring & name) override;
  void remove_tag(gnote::Tag &tag) override;
  std::vector<gnote::Tag::Ref> all_tags() const override;
private:
  typedef std::unique_ptr<gnote::Tag> TagPtr;
  std::vector<TagPtr> m_tags;
};

}

#endif

