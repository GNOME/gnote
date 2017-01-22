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

#include "sharp/string.hpp"


SUITE(String)
{
  TEST(replace_first)
  {
    Glib::ustring res;
    res = sharp::string_replace_first("foo bar baz", "ba", "");
    CHECK_EQUAL("foo r baz", res);

    res = sharp::string_replace_first("foo bar baz", "foo", "bingo");
    CHECK_EQUAL("bingo bar baz", res);

    res = sharp::string_replace_first("foo bar baz", "baz", "bar");
    CHECK_EQUAL("foo bar bar", res);

    res = sharp::string_replace_first("foo bar baz", "", "bar");
    CHECK_EQUAL("foo bar baz", res);

    res = sharp::string_replace_first("", "foo", "bar");
    CHECK_EQUAL("", res);

    res = sharp::string_replace_first("foo bar baz", "boo", "bingo");
    CHECK_EQUAL("foo bar baz", res);
  }
}

