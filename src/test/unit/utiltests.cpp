/*
 * gnote
 *
 * Copyright (C) 2019 Aurimas Cernius
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

#include "utils.hpp"


SUITE(Utis)
{
  int contains(const std::vector<int> & v, int val)
  {
    int count = 0;
    for(auto i : v) {
      if(i == val) {
        ++count;
      }
    }

    return count;
  }

  TEST(remove_swap_back)
  {
    std::vector<int> v;
    CHECK(gnote::utils::remove_swap_back(v, 5) == false);

    for(int i = 0; i < 100; ++i) {
      v.push_back(i);
    }
    for(int i = 10; i < 100; i += 10) {
      v.push_back(i);
    }

    CHECK(gnote::utils::remove_swap_back(v, 3));
    CHECK_EQUAL(108, v.size());
    CHECK_EQUAL(0, contains(v, 3));
    CHECK(gnote::utils::remove_swap_back(v, 3) == false);
    CHECK_EQUAL(108, v.size());

    CHECK_EQUAL(2, contains(v, 20));
    CHECK(gnote::utils::remove_swap_back(v, 20));
    CHECK_EQUAL(1, contains(v, 20));
    CHECK(gnote::utils::remove_swap_back(v, 20));
    CHECK_EQUAL(0, contains(v, 20));
  }
}
 
