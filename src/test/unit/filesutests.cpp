/*
 * gnote
 *
 * Copyright (C) 2017 Aurimas Cernius
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

#include <glibmm/miscutils.h>
#include <UnitTest++/UnitTest++.h>

#include "sharp/files.hpp"


SUITE(files)
{
  TEST(basename)
  {
    CHECK_EQUAL("baz", sharp::file_basename("/foo/bar/baz.txt"));
    CHECK_EQUAL("baz", sharp::file_basename("/foo/bar/baz"));
    CHECK_EQUAL(".", sharp::file_basename("/foo/bar/.."));
    CHECK_EQUAL("bar", sharp::file_basename("/foo/bar/"));
  }

  TEST(dirname)
  {
    CHECK_EQUAL("/foo/bar", sharp::file_dirname("/foo/bar/baz.txt"));
    CHECK_EQUAL("/foo/bar", sharp::file_dirname("/foo/bar/baz"));
    CHECK_EQUAL("/foo/bar", sharp::file_dirname("/foo/bar/.."));
    CHECK_EQUAL("/foo/bar", sharp::file_dirname("/foo/bar/"));
  }

  TEST(filename)
  {
    CHECK_EQUAL("baz.txt", sharp::file_filename("/foo/bar/baz.txt"));
    CHECK_EQUAL("baz", sharp::file_filename("/foo/bar/baz"));
    CHECK_EQUAL("..", sharp::file_filename("/foo/bar/.."));
    CHECK_EQUAL("bar", sharp::file_filename("/foo/bar/"));
  }

  TEST(exists)
  {
    Glib::ustring dir = Glib::get_current_dir();

    CHECK(sharp::file_exists(dir) == false);
    // Very unlikely to exist.
    CHECK(sharp::file_exists(__FILE__ __FILE__) == false);
    CHECK(sharp::file_exists(__FILE__) == true);
  }
}

