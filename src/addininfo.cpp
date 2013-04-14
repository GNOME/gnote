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

#include <stdexcept>

#include <glibmm/i18n.h>
#include <glibmm/keyfile.h>

#include "addininfo.hpp"
#include "debug.hpp"


namespace gnote {

  namespace {

    const char * ADDIN_INFO = "AddinInfo";

    AddinCategory resolve_addin_category(const std::string & cat)
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



AddinInfo::AddinInfo(const std::string & info_file)
  : m_category(ADDIN_CATEGORY_UNKNOWN)
  , m_default_enabled(false)
{
  load_from_file(info_file);
}

void AddinInfo::load_from_file(const std::string & info_file)
{
  try {
    Glib::KeyFile addin_info;
    if(!addin_info.load_from_file(info_file)) {
      throw std::runtime_error(_("Failed to load add-in information!"));
    }
    m_id = addin_info.get_string(ADDIN_INFO, "Id");
    m_name = addin_info.get_locale_string(ADDIN_INFO, "Name");
    m_description = addin_info.get_locale_string(ADDIN_INFO, "Description");
    m_authors = addin_info.get_locale_string(ADDIN_INFO, "Authors");
    m_category = resolve_addin_category(addin_info.get_string(ADDIN_INFO, "Category"));
    m_version = addin_info.get_string(ADDIN_INFO, "Version");
    try {
      m_copyright = addin_info.get_locale_string(ADDIN_INFO, "Copyright");
    }
    catch(Glib::KeyFileError & e) {
      DBG_OUT("Can't read copyright, using none: %s", e.what().c_str());
    }
    try {
      m_default_enabled = addin_info.get_boolean(ADDIN_INFO, "DefaultEnabled");
    }
    catch(Glib::KeyFileError & e) {
      DBG_OUT("Can't read default enabled status, assuming default: %s", e.what().c_str());
    }
    m_addin_module = addin_info.get_string(ADDIN_INFO, "Module");
  }
  catch(Glib::Error & e) {
    throw std::runtime_error(e.what());
  }
}

}
