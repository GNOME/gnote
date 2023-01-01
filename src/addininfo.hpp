/*
 * gnote
 *
 * Copyright (C) 2013-2017,2023 Aurimas Cernius
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

#include <map>

#include <glibmm/keyfile.h>
#include <glibmm/ustring.h>
#include <glibmm/varianttype.h>


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
  explicit AddinInfo(const Glib::ustring & info_file);
  void load_from_file(const Glib::ustring & info_file);

  const Glib::ustring & id() const
    {
      return m_id;
    }
  const Glib::ustring & name() const
    {
      return m_name;
    }
  const Glib::ustring & description() const
    {
      return m_description;
    }
  const Glib::ustring & authors() const
    {
      return m_authors;
    }
  AddinCategory category() const
    {
      return m_category;
    }
  const Glib::ustring & version() const
    {
      return m_version;
    }
  const Glib::ustring & copyright() const
    {
      return m_copyright;
    }
  bool default_enabled() const
    {
      return m_default_enabled;
    }
  const Glib::ustring & addin_module() const
    {
      return m_addin_module;
    }
  void addin_module(const Glib::ustring & module)
    {
      m_addin_module = module;
    }
  const std::map<Glib::ustring, Glib::ustring> & attributes() const
    {
      return m_attributes;
    }
  const std::map<Glib::ustring, const Glib::VariantType*> & actions() const
    {
      return m_actions;
    }
  const std::vector<Glib::ustring> & non_modifying_actions() const
    {
      return m_non_modifying_actions;
    }
  Glib::ustring get_attribute(const Glib::ustring & att);
  bool validate(const Glib::ustring & release, const Glib::ustring & version_info) const;
private:
  Glib::ustring m_id;
  Glib::ustring m_name;
  Glib::ustring m_description;
  Glib::ustring m_authors;
  AddinCategory m_category;
  Glib::ustring m_version;
  Glib::ustring m_copyright;
  bool m_default_enabled;
  Glib::ustring m_addin_module;
  Glib::ustring m_libgnote_release;
  Glib::ustring m_libgnote_version_info;

  std::map<Glib::ustring, Glib::ustring> m_attributes;
  std::map<Glib::ustring, const Glib::VariantType*> m_actions;
  std::vector<Glib::ustring> m_non_modifying_actions;

  bool validate_compatibility(const Glib::ustring & release, const Glib::ustring & version_info) const;
  void load_actions(Glib::KeyFile & addin_info, const Glib::ustring & key, const Glib::VariantType *type);
};

}

#endif
