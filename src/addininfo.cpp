/*
 * gnote
 *
 * Copyright (C) 2013-2017,2019,2021,2023 Aurimas Cernius
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

#include <stdexcept>

#include <glibmm/i18n.h>
#include <glibmm/variant.h>

#include "addininfo.hpp"
#include "debug.hpp"
#include "sharp/string.hpp"


namespace gnote {

  namespace {

    const char * ADDIN_INFO = "Plugin";
    const char * ADDIN_ATTS = "PluginAttributes";
    const char * ADDIN_ACTIONS = "Actions";

    AddinCategory resolve_addin_category(const Glib::ustring & cat)
    {
      if(cat == "Tools") {
        return ADDIN_CATEGORY_TOOLS;
      }
      if(cat == "Formatting") {
        return ADDIN_CATEGORY_FORMATTING;
      }
      if(cat == "DesktopIntegration") {
        return ADDIN_CATEGORY_DESKTOP_INTEGRATION;
      }
      if(cat == "Synchronization") {
        return ADDIN_CATEGORY_SYNCHRONIZATION;
      }

      return ADDIN_CATEGORY_UNKNOWN;
    }

  }



AddinInfo::AddinInfo(const Glib::ustring & info_file)
  : m_category(ADDIN_CATEGORY_UNKNOWN)
  , m_default_enabled(false)
{
  load_from_file(info_file);
}

void AddinInfo::load_from_file(const Glib::ustring & info_file)
{
  try {
    auto addin_info = Glib::KeyFile::create();
    if(!addin_info->load_from_file(info_file)) {
      throw std::runtime_error(_("Failed to load plugin information!"));
    }
    m_id = addin_info->get_string(ADDIN_INFO, "Id");
    m_name = addin_info->get_locale_string(ADDIN_INFO, "Name");
    m_description = addin_info->get_locale_string(ADDIN_INFO, "Description");
    m_authors = addin_info->get_locale_string(ADDIN_INFO, "Authors");
    m_category = resolve_addin_category(addin_info->get_string(ADDIN_INFO, "Category"));
    m_version = addin_info->get_string(ADDIN_INFO, "Version");
    if(addin_info->has_key(ADDIN_INFO, "Copyright")) {
      m_copyright = addin_info->get_locale_string(ADDIN_INFO, "Copyright");
    }
    if(addin_info->has_key(ADDIN_INFO, "DefaultEnabled")) {
      m_default_enabled = addin_info->get_boolean(ADDIN_INFO, "DefaultEnabled");
    }
    m_addin_module = addin_info->get_string(ADDIN_INFO, "Module");
    m_libgnote_release = addin_info->get_string(ADDIN_INFO, "LibgnoteRelease");
    m_libgnote_version_info = addin_info->get_string(ADDIN_INFO, "LibgnoteVersionInfo");

    if(addin_info->has_group(ADDIN_ATTS)) {
      for(const Glib::ustring & key : addin_info->get_keys(ADDIN_ATTS)) {
        m_attributes[key] = addin_info->get_string(ADDIN_ATTS, key);
      }
    }
    if(addin_info->has_group(ADDIN_ACTIONS)) {
      load_actions(*addin_info, "ActionsVoid", NULL);
      load_actions(*addin_info, "ActionsBool", &Glib::Variant<bool>::variant_type());
      load_actions(*addin_info, "ActionsInt", &Glib::Variant<gint32>::variant_type());
      load_actions(*addin_info, "ActionsString", &Glib::Variant<Glib::ustring>::variant_type());
      if(addin_info->has_key(ADDIN_ACTIONS, "NonModifyingActions")) {
        std::vector<Glib::ustring> actions;
        sharp::string_split(actions, addin_info->get_string(ADDIN_ACTIONS, "NonModifyingActions"), ",");
        for(auto action : actions) {
          m_non_modifying_actions.push_back(action);
        }
      }
    }
  }
  catch(Glib::Error & e) {
    throw std::runtime_error(e.what());
  }
}

void AddinInfo::load_actions(Glib::KeyFile & addin_info, const Glib::ustring & key, const Glib::VariantType *type)
{
  if(addin_info.has_key(ADDIN_ACTIONS, key)) {
    std::vector<Glib::ustring> actions;
    sharp::string_split(actions, addin_info.get_string(ADDIN_ACTIONS, key), ",");
    for(auto action : actions) {
      m_actions[action] = type;
    }
  }
}

Glib::ustring AddinInfo::get_attribute(const Glib::ustring & att)
{
  std::map<Glib::ustring, Glib::ustring>::iterator iter = m_attributes.find(att);
  if(iter != m_attributes.end()) {
    return iter->second;
  }
  return Glib::ustring();
}

bool AddinInfo::validate(const Glib::ustring & release, const Glib::ustring & version_info) const
{
  if(validate_compatibility(release, version_info)) {
    return true;
  }

  ERR_OUT(_("Incompatible plug-in %s: expected %s, got %s"),
          m_id.c_str(), (release + " " + version_info).c_str(),
          (m_libgnote_release + " " + m_libgnote_version_info).c_str());
  return false;
}

bool AddinInfo::validate_compatibility(const Glib::ustring & release, const Glib::ustring & version_info) const
{
  if(release != m_libgnote_release) {
    return false;
  }
  if(version_info == m_libgnote_version_info) {
    return true;
  }
  else {
    try {
      std::vector<Glib::ustring> parts;
      sharp::string_split(parts, m_libgnote_version_info, ":");
      if(parts.size() != 3) {
        return false;
      }

      int this_ver = STRING_TO_INT(parts[0]);
      parts.clear();
      sharp::string_split(parts, version_info, ":");
      int ver = STRING_TO_INT(parts[0]);
      int compat = STRING_TO_INT(parts[2]);

      if(this_ver > ver) {
        // too new
        return false;
      }
      if(this_ver < ver - compat) {
        // too old
        return false;
      }

      return true;
    }
    catch(std::bad_cast &) {
      return false;
    }
  }
}


}
