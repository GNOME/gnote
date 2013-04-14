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

#ifndef _ADDININFO_HPP_
#define _ADDININFO_HPP_

#include <string>

namespace gnote {

enum AddinCategory {
  ADDIN_CATEGORY_UNKNOWN,
  ADDIN_CATEGORY_TOOLS,
  ADDIN_CATEGORY_FORMATTING,
  ADDIN_CATEGORY_DESKTOP_INTEGRATION,
  ADDIN_CATEGORY_SYNCHRONIZATION
};


class AddinInfo
{
public:
  AddinInfo(){}
  explicit AddinInfo(const std::string & info_file);
  void load_from_file(const std::string & info_file);

  const std::string & id() const
    {
      return m_id;
    }
  const std::string & name() const
    {
      return m_name;
    }
  const std::string & description() const
    {
      return m_description;
    }
  const std::string & authors() const
    {
      return m_authors;
    }
  AddinCategory category() const
    {
      return m_category;
    }
  const std::string & version() const
    {
      return m_version;
    }
  const std::string & copyright() const
    {
      return m_copyright;
    }
  bool default_enabled() const
    {
      return m_default_enabled;
    }
  const std::string & addin_module() const
    {
      return m_addin_module;
    }
  void addin_module(const std::string & module)
    {
      m_addin_module = module;
    }
private:
  std::string m_id;
  std::string m_name;
  std::string m_description;
  std::string m_authors;
  AddinCategory m_category;
  std::string m_version;
  std::string m_copyright;
  bool m_default_enabled;
  std::string m_addin_module;
};

}

#endif
