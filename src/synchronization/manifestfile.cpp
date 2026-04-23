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


#include "debug.hpp"
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
      return false;
    }
  }

  if(load_xml()) {
    return true;
  }

  m_xml_content.clear();
  throw std::runtime_error("Failed to parse xml");
}

bool ManifestFile::load_xml()
{
  if(xmlDocPtr xml = xmlReadMemory(m_xml_content.c_str(), m_xml_content.size(), m_path ? m_path->get_uri().c_str() : nullptr, "UTF-8", 0)) {
    m_xml = xmlDocUniquePtr(std::move(xml), &xmlFreeDoc);
    return true;
  }

  return false;
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

void ManifestFile::write_new(const Glib::ustring &content)
{
  // Rename original /manifest.xml to /manifest.xml.old
  auto old_manifest_path = Gio::File::create_for_uri(m_path->get_uri() + ".old");
  if(old_manifest_path->query_exists()) {
    old_manifest_path->remove();
  }
  if(m_path->query_exists()) {
    m_path->move(old_manifest_path);
  }

  {
    auto stream = m_path->create_file();
    gsize written;
    if(!stream->write_all(content, written)) {
      throw std::runtime_error("Failed to write new manifest file");
    }
    stream->close();
  }

  try {
    if(old_manifest_path->query_exists()) {
      old_manifest_path->remove();
    }
  }
  catch(std::exception &e) {
    ERR_OUT("Recoverable failure: failed to remove old manifest: %s", e.what());
  }


  m_xml.reset();
  m_xml_content = content;
  if(!load_xml()) {
    throw std::runtime_error("Failed to parse new xml");
  }
}

}
}

