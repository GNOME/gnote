/*
 * gnote
 *
 * Copyright (C) 2023 Aurimas Cernius
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

#include "base/hash.hpp"


SUITE(Hash)
{
  TEST(Hash_Glib_ustring)
  {
    Glib::ustring tst1 = "Hello, World!";
    Glib::ustring tst2 = "Some other string for testing";

    gnote::Hash<Glib::ustring> h;
    std::hash<std::string> check_h;

    auto hash1 = h(tst1);
    auto check1 = check_h(tst1);
    CHECK_EQUAL(check1, hash1);

    auto hash2 = h(tst2);
    auto check2 = check_h(tst2);
    CHECK_EQUAL(check2, hash2);
  }
}

