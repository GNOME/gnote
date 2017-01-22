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

  TEST(replace_all)
  {
    Glib::ustring res;
    res = sharp::string_replace_all("", "foo", "bar");
    CHECK_EQUAL("", res);

    res = sharp::string_replace_all("foo bar baz", "", "bar");
    CHECK_EQUAL("foo bar baz", res);

    res = sharp::string_replace_all("foo bar baz", "boo", "bingo");
    CHECK_EQUAL("foo bar baz", res);

    res = sharp::string_replace_all("foo bar baz", "bar", "baz");
    CHECK_EQUAL("foo baz baz", res);

    res = sharp::string_replace_all("foo bar baz", "bar", "");
    CHECK_EQUAL("foo  baz", res);

    res = sharp::string_replace_all("foo bar baz", "ba", "bingo");
    CHECK_EQUAL("foo bingor bingoz", res);

    res = sharp::string_replace_all("ffooo foo", "foo", "o");
    CHECK_EQUAL("foo o", res);

    res = sharp::string_replace_all("foo bar baz", "baz", "bar");
    CHECK_EQUAL("foo bar bar", res);
  }

  TEST(replace_regex)
  {
    Glib::ustring res;
    res = sharp::string_replace_regex("CamelCase", "([aem])", "Xx");
    CHECK_EQUAL("CXxXxXxlCXxsXx", res);

    res = sharp::string_replace_regex("CamelCase", "ame", "Xx");
    CHECK_EQUAL("CXxlCase", res);
  }

  TEST(match_iregex)
  {
    CHECK(sharp::string_match_iregex("CamelCase", "^Camel.*$"));
  }

  TEST(substring)
  {
    CHECK_EQUAL("bar baz", sharp::string_substring("foo bar baz", 4));
    CHECK_EQUAL("", sharp::string_substring("foo bar baz", 14));
    CHECK_EQUAL("bar", sharp::string_substring("foo bar baz", 4, 3));
    CHECK_EQUAL("", sharp::string_substring("foo bar baz", 14, 3));
  }

  TEST(trim)
  {
    CHECK_EQUAL("foo", sharp::string_trim("   foo   "));
    CHECK_EQUAL("foo", sharp::string_trim("foo   "));
    CHECK_EQUAL("foo", sharp::string_trim("   foo"));
    CHECK_EQUAL("foo", sharp::string_trim("foo"));
    CHECK_EQUAL("", sharp::string_trim(""));

    CHECK_EQUAL(" foo ", sharp::string_trim("** foo ++", "*+"));
    CHECK_EQUAL(" foo ", sharp::string_trim("** foo ", "*+"));
    CHECK_EQUAL(" foo ", sharp::string_trim(" foo ++", "*+"));
    CHECK_EQUAL("foo", sharp::string_trim("foo", "*+"));
    CHECK_EQUAL("", sharp::string_trim("", "*+"));
  }
}

