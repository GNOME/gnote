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


#include "notebookserializer.hpp"
#include "sharp/xmlwriter.hpp"

namespace gnote {
namespace notebooks {

Glib::ustring NotebookSerializer::serialize(const std::vector<INotebook::Ref> &notebooks)
{
  if(notebooks.empty()) {
    return Glib::ustring();
  }

  sharp::XmlWriter writer;
  writer.write_start_document();
  writer.write_start_element("", "notebooks", "");

  for(const INotebook &nb : notebooks) {
    writer.write_start_element("", "notebook", "");
    writer.write_attribute_string("", "id", "", nb.get_normalized_name());
    writer.write_attribute_string("", "name", "", nb.get_name());
    writer.write_end_element();
  }

  writer.write_end_element();
  writer.write_end_document();
  return writer.to_string();
}

}
}

