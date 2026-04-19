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


#include <giomm/file.h>


namespace gnote {
namespace sync {

class ManifestFile
{
public:
  explicit ManifestFile(Glib::RefPtr<Gio::File> && path);
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
private:
  Glib::RefPtr<Gio::File> m_path;
};

}
}

