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

#include <UnitTest++/UnitTest++.h>

#include "sharp/fileinfo.hpp"


SUITE(FileInfo)
{
  TEST(get_name)
  {
    sharp::FileInfo file_info1("/foo/bar/baz.txt");
    sharp::FileInfo file_info2("/foo/bar/baz.");
    sharp::FileInfo file_info3("/foo/bar/baz");
    sharp::FileInfo file_info4("/foo/bar/..");
    sharp::FileInfo file_info5("/foo/bar/");

    CHECK_EQUAL("baz.txt", file_info1.get_name());
    CHECK_EQUAL("baz.", file_info2.get_name());
    CHECK_EQUAL("baz", file_info3.get_name());
    CHECK_EQUAL("..", file_info4.get_name());
    CHECK_EQUAL("bar", file_info5.get_name());
  }

  TEST(get_extension)
  {
    sharp::FileInfo file_info1("/foo/bar/baz.txt");
    sharp::FileInfo file_info2("/foo/bar/baz.");
    sharp::FileInfo file_info3("/foo/bar/baz");
    sharp::FileInfo file_info4("/foo/bar/..");
    sharp::FileInfo file_info5("/foo/bar/");

    CHECK_EQUAL(".txt", file_info1.get_extension());
    CHECK_EQUAL(".", file_info2.get_extension());
    CHECK_EQUAL("", file_info3.get_extension());
    CHECK_EQUAL("", file_info4.get_extension());
    CHECK_EQUAL("", file_info5.get_extension());
  }
}
