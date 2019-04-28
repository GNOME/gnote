/*
 * gnote
 *
 * Copyright (C) 2014,2017-2019 Aurimas Cernius
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


#include "itagmanager.hpp"

namespace test {

class TagManager
  : public gnote::ITagManager
{
public:
  static void ensure_exists();

  virtual gnote::Tag::Ptr get_tag(const Glib::ustring & tag_name) const override;
  virtual gnote::Tag::Ptr get_or_create_tag(const Glib::ustring &) override;
  virtual gnote::Tag::Ptr get_system_tag(const Glib::ustring & tag_name) const override;
  virtual gnote::Tag::Ptr get_or_create_system_tag(const Glib::ustring & name) override;
  virtual void remove_tag(const gnote::Tag::Ptr & tag) override;
  virtual std::vector<gnote::Tag::Ptr> all_tags() const override;
private:
  static TagManager* s_manager;

  std::map<Glib::ustring, gnote::Tag::Ptr> m_tags;
};

}

