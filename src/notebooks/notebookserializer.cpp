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
#include "notebookserializer.hpp"
#include "sharp/datetime.hpp"
#include "sharp/xmlreader.hpp"
#include "sharp/xmlwriter.hpp"

namespace gnote {
namespace notebooks {

namespace {

Glib::DateTime parse_date(const Glib::ustring s)
{
  if(s.empty()) {
    return Glib::DateTime::create_now_utc();
  }

  return sharp::date_time_from_iso8601(s).to_utc();
}

}


template <typename NotebookT>
Glib::ustring NotebookSerializer::serialize(const std::vector<NotebookT> &notebooks, const Glib::DateTime &timestamp)
{
  if(notebooks.empty()) {
    return Glib::ustring();
  }
  auto timestamp_str = sharp::date_time_to_iso8601(timestamp.to_utc());

  sharp::XmlWriter writer;
  writer.write_start_document();
  writer.write_start_element("", "notebooks", "");
  if(!timestamp_str.empty()) {
    writer.write_attribute_string("", "timestamp", "", timestamp_str);
  }

  for(const INotebook &nb : notebooks) {
    writer.write_start_element("", "notebook", "");
    writer.write_attribute_string("", "id", "", nb.get_normalized_name());
    writer.write_attribute_string("", "name", "", nb.get_name());
    auto created = sharp::date_time_to_iso8601(nb.created().to_utc());
    if(!created.empty()) {
      writer.write_attribute_string("", "created", "", created);
    }
    writer.write_end_element();
  }

  writer.write_end_element();
  writer.write_end_document();
  return writer.to_string();
}

Glib::ustring NotebookSerializer::serialize(const std::vector<INotebook::Ref> &notebooks, const Glib::DateTime &timestamp)
{
  return serialize<INotebook::Ref>(notebooks, timestamp);
}

Glib::ustring NotebookSerializer::serialize(const std::vector<NotebookData> &notebooks, const Glib::DateTime &timestamp)
{
  return serialize<NotebookData>(notebooks, timestamp);
}

NotebookSerializer::Notebooks NotebookSerializer::deserialize(const Glib::ustring &xml)
{
  sharp::XmlReader reader;
  reader.load_buffer(xml);
  Notebooks result;

  while(reader.read()) {
    result.valid = true;
    if(reader.get_node_type() == XML_READER_TYPE_ELEMENT) {
      auto name = reader.get_name();
      if(name == "notebooks") {
        result.timestamp = parse_date(reader.get_attribute("timestamp"));
        continue;
      }
      else if(name == "notebook") {
        auto id = reader.get_attribute("id");
        auto name = reader.get_attribute("name");
	auto created = parse_date(reader.get_attribute("created"));
        if(!id.empty() && !name.empty()) {
          result.notebooks.emplace_back(std::move(id), created);
          result.notebooks.back().set_name(name);
        }
      }
      else {
        ERR_OUT("Unexpected XML element '%s'", name.c_str());
        throw std::runtime_error("Unexpected XML element");
      }
    }
  }

  return result;
}

std::vector<NotebookData> NotebookSerializer::merge(std::vector<NotebookData> &loaded,
  const std::vector<INotebook::Ref> &current, const Glib::DateTime &loaded_time)
{
  std::vector<NotebookData> updated;

  // loaded not among the current or requiring update
  for(const auto &ld_nb : loaded) {
    auto found = current.end();
    for(auto iter = current.begin(); iter != current.end(); ++iter) {
      if(iter->get().get_normalized_name() == ld_nb.get_normalized_name()) {
        found = iter;
        break;
      }
    }

    bool update = false;
    if(found == current.end()) {
      update = true;
    }
    else if(found->get().created() < ld_nb.created()) {
      update = true;
    }

    if(update) {
      updated.emplace_back(ld_nb.get_normalized_name(), ld_nb.created());
      updated.back().set_name(ld_nb.get_name());
    }
  }

  // current notebooks not among the loaded
  for(const INotebook &nb : current) {
    auto found = loaded.end();
    for(auto iter = loaded.begin(); iter != loaded.end(); ++iter) {
      if(iter->get_normalized_name() == nb.get_normalized_name()) {
        found = iter;
        break;
      }
    }

    if(found == loaded.end() && nb.created() > loaded_time) {
      loaded.emplace_back(nb.get_normalized_name(), nb.created());
      loaded.back().set_name(nb.get_name());
    }
  }

  return updated;
}

}
}

