/*
 * gnote
 *
 * Copyright (C) 2014 Aurimas Cernius
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


#include "base/macros.hpp"
#include "itagmanager.hpp"

namespace test {

class TagManager
  : public gnote::ITagManager
{
public:
  virtual gnote::Tag::Ptr get_tag(const std::string & tag_name) const override;
  virtual gnote::Tag::Ptr get_or_create_tag(const std::string &) override;
  virtual gnote::Tag::Ptr get_system_tag(const std::string & tag_name) const override;
  virtual gnote::Tag::Ptr get_or_create_system_tag(const std::string & name) override;
  virtual void remove_tag(const gnote::Tag::Ptr & tag) override;
  virtual void all_tags(std::list<gnote::Tag::Ptr> &) const override;
private:
  std::map<std::string, gnote::Tag::Ptr> m_tags;
};

}

