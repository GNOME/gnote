/*
 * gnote
 *
 * Copyright (C) 2013 Aurimas Cernius
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

#ifndef _ITAGMANAGER_HPP_
#define _ITAGMANAGER_HPP_

#include "tag.hpp"
#include "base/singleton.hpp"

namespace gnote {

class ITagManager
  : public base::Singleton<ITagManager>
{
public:
  static const char * TEMPLATE_NOTE_SYSTEM_TAG;
  static const char * TEMPLATE_NOTE_SAVE_SIZE_SYSTEM_TAG;
  static const char * TEMPLATE_NOTE_SAVE_SELECTION_SYSTEM_TAG;
  static const char * TEMPLATE_NOTE_SAVE_TITLE_SYSTEM_TAG;

  virtual ~ITagManager();

  virtual Tag::Ptr get_tag(const std::string & tag_name) const = 0;
  virtual Tag::Ptr get_or_create_tag(const std::string &) = 0;
  virtual Tag::Ptr get_system_tag(const std::string & tag_name) const = 0;
  virtual Tag::Ptr get_or_create_system_tag(const std::string & name) = 0;
  virtual void remove_tag(const Tag::Ptr & tag) = 0;
  virtual void all_tags(std::list<Tag::Ptr> &) const = 0;
};

}

#endif

