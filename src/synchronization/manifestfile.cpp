/*
 * gnote
 *
 * Copyright (C) 2026 Aurimas Cernius
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


#include "manifestfile.hpp"
#include "base/macros.hpp"
#include "sharp/files.hpp"
#include "sharp/xml.hpp"


namespace gnote {
namespace sync {

ManifestFile::ManifestFile(Glib::RefPtr<Gio::File> && path)
  : m_path(std::move(path))
  , m_xml(nullptr, nullptr)
{
  if(!m_path) {
    throw std::invalid_argument("Manifest path must be provided");
  }
}

ManifestFile::ManifestFile(Glib::ustring && xml_content)
  : m_xml_content(std::move(xml_content))
  , m_xml(nullptr, nullptr)
{
}

bool ManifestFile::load()
{
  if(m_path) {
    if(m_path->query_exists()) {
      m_xml_content = sharp::file_read_all_text(*m_path);
    }
    else {
      return true;
    }
  }

  if(xmlDocPtr xml = xmlReadMemory(m_xml_content.c_str(), m_xml_content.size(), m_path ? m_path->get_uri().c_str() : nullptr, "UTF-8", 0)) {
    m_xml = xmlDocUniquePtr(std::move(xml), &xmlFreeDoc);
    return true;
  }

  m_xml_content.clear();
  throw std::runtime_error("Failed to parse xml");
}

unsigned ManifestFile::revision()
{
  if(m_revision.has_value()) {
    return m_revision.value();
  }

  if(!m_xml) {
    throw std::runtime_error("No valid manifest file has been loaded");
  }

  xmlNodePtr root_node = xmlDocGetRootElement(m_xml.get());
  xmlNodePtr sync_node = sharp::xml_node_xpath_find_single_node(root_node, "//sync");
  Glib::ustring latest_rev_str = sharp::xml_node_get_attribute(sync_node, "revision");
  if(latest_rev_str != "") {
    return STRING_TO_INT(latest_rev_str );
  }

  throw std::runtime_error("No revision found in the manifest");
}

}
}

