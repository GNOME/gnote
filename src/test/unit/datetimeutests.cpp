/*
 * gnote
 *
 * Copyright (C) 2017,2020-2021,2024 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
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


#include <glibmm/stringutils.h>
#include <UnitTest++/UnitTest++.h>

#include "utils.hpp"
#include "sharp/xmlconvert.hpp"

namespace gnote {
namespace utils {
  // not in header, inner impl as separate func for testing
  Glib::ustring get_pretty_print_date(const Glib::DateTime& date, bool show_time, bool use_12h, const Glib::DateTime& now);
}
}

SUITE(DateTime)
{

  TEST(date_time_to_string)
  {
    auto d = Glib::DateTime::create_local(1991, 7, 7, 15, 40, 34);
    CHECK(bool(d));
    Glib::ustring date_string = sharp::date_time_to_string(d, "%F %T");
    CHECK_EQUAL("1991-07-07 15:40:34", date_string);
  }

  TEST(date_time_to_iso8601)
  {
    auto d = Glib::DateTime::create_local(2009, 3, 24, 7, 34, 35);
    CHECK(bool(d));
    Glib::ustring date_string = sharp::date_time_to_iso8601(d);
    CHECK_EQUAL("2009-03-24T07:34:35.000000Z", date_string);

    d = Glib::DateTime::create_local(2009, 3, 24, 7, 34, 35.54);
    CHECK(bool(d));
    date_string = sharp::date_time_to_iso8601(d);
    CHECK_EQUAL("2009-03-24T07:34:35.540000Z", date_string);

    d = Glib::DateTime::create_local(2021, 1, 2, 3, 4, 5);
    CHECK(bool(d));
    date_string = sharp::date_time_to_iso8601(d);
    CHECK_EQUAL("2021-01-02T03:04:05.000000Z", date_string);
  }

  TEST(date_time_from_iso8601)
  {
    int hour_offset;
    {
      // use same time as the test bellow
      auto dt = Glib::DateTime::create_local(1991, 07, 07, 15, 40, 34);
      auto offset = std::div(dt.get_utc_offset(), 3600000000);
      CHECK_EQUAL(0, offset.rem);  // test time-zone should be strictly full hours away from UTC
      hour_offset = offset.quot;
    }

    Glib::DateTime d = sharp::date_time_from_iso8601("1991-07-07T15:40:34.067890Z");
    CHECK(bool(d));
    CHECK_EQUAL(1991, d.get_year());
    CHECK_EQUAL(7, d.get_month());
    CHECK_EQUAL(7, d.get_day_of_month());
    CHECK_EQUAL(15 + hour_offset, d.get_hour());  // time-zone corrected
    CHECK_EQUAL(40, d.get_minute());
    CHECK_EQUAL(34, d.get_second());
    CHECK_EQUAL(67890, d.get_microsecond());

    Glib::DateTime d2 = sharp::date_time_from_iso8601("2009-03-24T03:34:35.2914680-04:00Z");
    CHECK(bool(d2));
    CHECK_EQUAL(24, d2.get_day_of_month());
    CHECK_EQUAL(7, d2.get_hour());
    CHECK_EQUAL(291468, d2.get_microsecond());

    Glib::DateTime d3 = sharp::date_time_from_iso8601("2009-03-24T03:34:35.2914680+00:30Z");
    CHECK(bool(d3));
    CHECK_EQUAL(24, d3.get_day_of_month());
    CHECK_EQUAL(3, d3.get_hour());
    CHECK_EQUAL(4, d3.get_minute());
    CHECK_EQUAL(291468, d3.get_microsecond());
  }

  TEST(get_pretty_print_date_against_now)
  {
    Glib::DateTime d = Glib::DateTime::create_now_local();
    Glib::ustring date_string = gnote::utils::get_pretty_print_date(d, false, false);
    CHECK_EQUAL("Today", date_string);

    d = d.add_days(1);
    date_string = gnote::utils::get_pretty_print_date(d, false, false);
    CHECK_EQUAL("Tomorrow", date_string);

    d = Glib::DateTime::create_now_local();
    d = d.add_days(-1);
    date_string = gnote::utils::get_pretty_print_date(d, false, false);
    CHECK_EQUAL("Yesterday", date_string);

    d = sharp::date_time_from_iso8601("2009-03-24T13:34:35.2914680-04:00");
    date_string = gnote::utils::get_pretty_print_date(d, true, false);
    CHECK(Glib::str_has_suffix(date_string, "17:34"));

    date_string = gnote::utils::get_pretty_print_date(d, true, true);
    CHECK(Glib::str_has_suffix(date_string.lowercase(), "5:34 pm"));
  }

  TEST(get_pretty_print_date_different_year)
  {
    auto now = sharp::date_time_from_iso8601("2024-01-01T13:34:35.2914680");
    auto d = sharp::date_time_from_iso8601("2023-12-31T12:38:15.2214680");

    auto date_string = gnote::utils::get_pretty_print_date(d, false, false, now);
    CHECK_EQUAL("Yesterday", date_string);

    date_string = gnote::utils::get_pretty_print_date(now, false, false, d);
    CHECK_EQUAL("Tomorrow", date_string);

    d = d.add_months(-2);
    date_string = gnote::utils::get_pretty_print_date(d, true, false, now);
    CHECK_EQUAL("Oct 31 2023, 12:38", date_string);
  }

  TEST(date_time_equality_operators)
  {
    auto d1 = Glib::DateTime::create_local(2020, 1, 2, 13, 14, 15);
    auto d2 = d1.add_days(1);
    auto d3 = d2.add_days(-1);
    Glib::DateTime invalid;

    CHECK(d1 == d3);
    CHECK(d2 == d2);
    CHECK(!(d1 == d2));
    CHECK(!(d1 == invalid));
    CHECK(invalid == invalid);

    CHECK(d1 != d2);
    CHECK(!(d1 != d3));
    CHECK(d1 != invalid);
    CHECK(!(d2 != d2));
    CHECK(!(invalid != invalid));
  }

  TEST(date_time_less_than_operators)
  {
    auto d1 = Glib::DateTime::create_local(2020, 1, 2, 13, 14, 15);
    auto d2 = d1.add_days(1);
    auto d3 = d2.add_days(-1);
    Glib::DateTime invalid;

    CHECK(d1 < d2);
    CHECK(d1 <= d2);
    CHECK(!(d2 < d1));
    CHECK(!(d2 <= d1));
    CHECK(!(d1 < d3));
    CHECK(d1 <= d3);
    CHECK(d3 <= d1);
    CHECK(invalid < d1);
    CHECK(invalid <= d1);
    CHECK(!(d1 < invalid));
    CHECK(!(d1 <= invalid));
    CHECK(!(invalid < invalid));
    CHECK(invalid <= invalid);
  }

  TEST(date_time_greater_than_operators)
  {
    auto d1 = Glib::DateTime::create_local(2020, 1, 2, 13, 14, 15);
    auto d2 = d1.add_days(1);
    auto d3 = d2.add_days(-1);
    Glib::DateTime invalid;

    CHECK(!(d1 > d2));
    CHECK(!(d1 >= d2));
    CHECK(d2 > d1);
    CHECK(d2 >= d1);
    CHECK(!(d1 > d3));
    CHECK(d1 >= d3);
    CHECK(d3 >= d1);
    CHECK(!(invalid > d1));
    CHECK(!(invalid >= d1));
    CHECK(d1 > invalid);
    CHECK(d1 >= invalid);
    CHECK(!(invalid > invalid));
    CHECK(invalid >= invalid);
  }

}

