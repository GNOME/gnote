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


#include <UnitTest++/UnitTest++.h>

#include "testutils.hpp"
#include "synchronization/manifestfile.hpp"


SUITE(ManifestFile)
{
  const char *TEST_MANIFEST_CONTENT = ""
    "<sync revision=\"2\" server-id=\"0cac27e4-cb54-4d9a-aaaa-28a010f213d3\">"
    "  <note id=\"064e27ed-eaf3-4769-9084-0fa925a5cf11\" rev=\"1\"/>"
    "  <note id=\"0ead2704-4c24-4110-b7da-22d00cae25f3\" rev=\"2\"/>"
    "  <note id=\"1006a78a-6e61-4492-b5cc-a42f62d0a9ef\" rev=\"0\"/>"
    "</sync>";


  TEST(null_path_throws)
  {
    try {
      gnote::sync::ManifestFile manifest(Glib::RefPtr<Gio::File>{});
      CHECK(false);
    }
    catch(std::invalid_argument&) {
      // expected
    }
  }

  TEST(load_non_existent_file_fails)
  {
    auto sync_path = Gio::File::create_for_path(test::make_temp_dir());
    gnote::sync::ManifestFile manifest(sync_path->get_child("manifest.xml"));
    bool result = manifest.load();
    CHECK(!result);
  }

  TEST(load_invalid_xml_throws)
  {
    try {
      Glib::ustring xml{TEST_MANIFEST_CONTENT};
      xml.erase(xml.rfind('<'));
      gnote::sync::ManifestFile manifest(std::move(xml));
      [[maybe_unused]] bool result = manifest.load();
      CHECK(false);
    }
    catch(std::runtime_error&) {
      // expected
    }
  }

  TEST(load_valid_xml_succeeds)
  {
    gnote::sync::ManifestFile manifest(Glib::ustring{TEST_MANIFEST_CONTENT});
    bool result = manifest.load();
    CHECK(result);
    auto revision = manifest.revision();
    CHECK_EQUAL(2, revision);
  }

  TEST(server_id_valid_xml_returns_id)
  {
    gnote::sync::ManifestFile manifest(Glib::ustring{TEST_MANIFEST_CONTENT});
    bool result = manifest.load();
    CHECK(result);
    auto server_id = manifest.server_id();
    CHECK_EQUAL("0cac27e4-cb54-4d9a-aaaa-28a010f213d3", server_id);
  }

  TEST(write_new_reloads)
  {
    Glib::ustring xml{TEST_MANIFEST_CONTENT};
    auto sync_path = Gio::File::create_for_path(test::make_temp_dir());
    auto manifest_file = sync_path->get_child("manifest.xml");
    {
      auto stream = manifest_file->create_file();
      gsize written = 0;
      stream->write_all(xml, written);
      stream->close();
    }

    gnote::sync::ManifestFile manifest(std::move(manifest_file));
    bool loaded = manifest.load();
    CHECK(loaded);
    {
      auto pos = xml.find('2');
      xml.replace(pos, 1, "3");
    }

    manifest.write_new(xml);
    auto revision = manifest.revision();
    CHECK_EQUAL(3, revision);
  }
}

