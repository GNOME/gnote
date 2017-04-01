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

#include "sharp/uri.hpp"


SUITE(Uri)
{
  TEST(is_file)
  {
    sharp::Uri uri1("http://bugzilla.gnome.org/");
    sharp::Uri uri2("https://bugzilla.gnome.org/");
    sharp::Uri uri3("file:///tmp/foo.txt");

    CHECK(!uri1.is_file());
    CHECK(!uri2.is_file());
    CHECK(uri3.is_file());
  }

  TEST(local_path)
  {
    sharp::Uri uri1("http://bugzilla.gnome.org/");
    sharp::Uri uri2("https://bugzilla.gnome.org/");
    sharp::Uri uri3("file:///tmp/foo.txt");

    CHECK_EQUAL("https://bugzilla.gnome.org/", uri2.local_path());
    CHECK_EQUAL("http://bugzilla.gnome.org/", uri1.local_path());
    CHECK_EQUAL("/tmp/foo.txt", uri3.local_path());
  }

  TEST(get_host)
  {
    sharp::Uri uri1("http://bugzilla.gnome.org/");
    sharp::Uri uri2("https://bugzilla.gnome.org/");
    sharp::Uri uri3("file:///tmp/foo.txt");

    CHECK_EQUAL("bugzilla.gnome.org", uri1.get_host());
    CHECK_EQUAL("bugzilla.gnome.org", uri2.get_host());
    CHECK_EQUAL("", uri3.get_host());
  }
}

