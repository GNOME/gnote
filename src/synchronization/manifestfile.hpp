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


#include <optional>

#include <giomm/file.h>
#include <libxml/xmlreader.h>


namespace gnote {
namespace sync {

class ManifestFile
{
public:
  explicit ManifestFile(Glib::RefPtr<Gio::File> && path);
  explicit ManifestFile(Glib::ustring && xml_content);
  ManifestFile(const ManifestFile&) = delete;
  ManifestFile &operator=(const ManifestFile&) = delete;

  [[nodiscard]] const Glib::RefPtr<Gio::File> &path() const
    {
      if(!m_path) {
        throw std::logic_error("path can only be called when provided to constructor");
      }

      return m_path;
    }
  [[nodiscard]] Gio::File &file() const
    {
      return *path();
    }

  [[nodiscard]] bool load();
  [[nodiscard]] bool is_loaded() const
    {
      return m_xml.get() != nullptr;
    }
  xmlDoc &xml_doc()
    {
      if(m_xml) {
        return *m_xml;
      }

      throw std::logic_error("Attempt to get XML doc that wasn't loaded");
    }
  [[nodiscard]] unsigned revision();
  [[nodiscard]] Glib::ustring server_id();
  void write_new(const Glib::ustring &content);
private:
  bool load_xml();

  Glib::RefPtr<Gio::File> m_path;
  Glib::ustring m_xml_content;
  using xmlDocUniquePtr = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;
  xmlDocUniquePtr m_xml;
  std::optional<unsigned> m_revision;
  Glib::ustring m_server_id;
};

}
}

