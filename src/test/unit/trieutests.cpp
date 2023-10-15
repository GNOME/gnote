/*
 * gnote
 *
 * Copyright (C) 2017,2023 Aurimas Cernius
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

#include "trie.hpp"

SUITE(TrieTree)
{
  struct Fixture
  {
    const Glib::ustring src;
    gnote::TrieTree<Glib::ustring> trie;
    gnote::TrieHit<Glib::ustring>::List matches;

    Fixture()
      : src("bazar this is some foo, bar, and baz BazBarFooFoo bazbazarbaz end bazar ąČęĖįŠųŪž")
      , trie(false)
    {

      trie.add_keyword("foo", "foo");
      trie.add_keyword("bar", "bar");
      trie.add_keyword("baz", "baz");
      trie.add_keyword("bazar", "bazar");
      trie.add_keyword("ąčęėįšųūž", "ąčęėįšųūž");
      trie.compute_failure_graph();

      matches = trie.find_matches(src);
    }
  };

  TEST_FIXTURE(Fixture, matches)
  {
    CHECK_EQUAL(16, matches.size());
    auto iter = matches.begin();

    CHECK_EQUAL("baz", iter->key());
    CHECK_EQUAL(0, iter->start());
    CHECK_EQUAL(3, iter->end());
    ++iter;
    CHECK_EQUAL("bazar", iter->key());
    CHECK_EQUAL(0, iter->start());
    CHECK_EQUAL(5, iter->end());
  }

  TEST_FIXTURE(Fixture, search)
  {
    auto hit = matches.begin();
    CHECK_EQUAL("baz", hit->key());
    CHECK_EQUAL(0, hit->start());
    CHECK_EQUAL(3, hit->end());

    ++hit;
    CHECK_EQUAL("bazar", hit->key());
    CHECK_EQUAL(0, hit->start());
    CHECK_EQUAL(5, hit->end());

    ++hit;
    CHECK_EQUAL("foo", hit->key());
    CHECK_EQUAL(19, hit->start());
    CHECK_EQUAL(22, hit->end());

    ++hit;
    CHECK_EQUAL("bar", hit->key());
    CHECK_EQUAL(24, hit->start());
    CHECK_EQUAL(27, hit->end());

    ++hit;
    CHECK_EQUAL("baz", hit->key());
    CHECK_EQUAL(33, hit->start());
    CHECK_EQUAL(36, hit->end());

    ++hit;
    CHECK_EQUAL("Baz", hit->key());
    CHECK_EQUAL(37, hit->start());
    CHECK_EQUAL(40, hit->end());

    ++hit;
    CHECK_EQUAL("Bar", hit->key());
    CHECK_EQUAL(40, hit->start());
    CHECK_EQUAL(43, hit->end());

    ++hit;
    CHECK_EQUAL("Foo", hit->key());
    CHECK_EQUAL(43, hit->start());
    CHECK_EQUAL(46, hit->end());

    ++hit;
    CHECK_EQUAL("Foo", hit->key());
    CHECK_EQUAL(46, hit->start());
    CHECK_EQUAL(49, hit->end());

    ++hit;
    CHECK_EQUAL("baz", hit->key());
    CHECK_EQUAL(50, hit->start());
    CHECK_EQUAL(53, hit->end());

    ++hit;
    CHECK_EQUAL("baz", hit->key());
    CHECK_EQUAL(53, hit->start());
    CHECK_EQUAL(56, hit->end());

    ++hit;
    CHECK_EQUAL("bazar", hit->key());
    CHECK_EQUAL(53, hit->start());
    CHECK_EQUAL(58, hit->end());

    ++hit;
    CHECK_EQUAL("baz", hit->key());
    CHECK_EQUAL(58, hit->start());
    CHECK_EQUAL(61, hit->end());

    ++hit;
    CHECK_EQUAL("baz", hit->key());
    CHECK_EQUAL(66, hit->start());
    CHECK_EQUAL(69, hit->end());

    ++hit;
    CHECK_EQUAL("bazar", hit->key());
    CHECK_EQUAL(66, hit->start());
    CHECK_EQUAL(71, hit->end());

    ++hit;
    CHECK_EQUAL("ąČęĖįŠųŪž", hit->key());
    CHECK_EQUAL(72, hit->start());
    CHECK_EQUAL(81, hit->end());
  }
}

