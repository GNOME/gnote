/*
 * gnote
 *
 * Copyright (C) 2017 Aurimas Cernius
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


SUITE(DateTime)
{

  TEST(to_string)
  {
    sharp::DateTime d(678901234, 67890);
    Glib::ustring date_string = sharp::XmlConvert::to_string(d);
    CHECK_EQUAL("1991-07-07T15:40:34.067890Z", date_string);

    d = sharp::DateTime::from_iso8601("2009-03-24T03:34:35.2914680-04:00");
    // check when usec is 0.
    // see http://bugzilla.gnome.org/show_bug.cgi?id=581844
    d.set_usec(0);

    date_string = sharp::XmlConvert::to_string(d);
    CHECK_EQUAL("2009-03-24T07:34:35.000000Z", date_string);
  }

  TEST(from_iso8601)
  {
    sharp::DateTime d(678901234, 67890);
    sharp::DateTime d2 = sharp::DateTime::from_iso8601("1991-07-07T15:40:34.067890Z");
    CHECK(d == d2);

    sharp::DateTime d3 = sharp::DateTime::from_iso8601("2009-03-24T03:34:35.2914680-04:00");
    CHECK(d3.is_valid());
  }

  TEST(pretty_print_date)
  {
    sharp::DateTime d = sharp::DateTime::now();
    Glib::ustring date_string = gnote::utils::get_pretty_print_date(d, false, false);
    CHECK_EQUAL("Today", date_string);

    d.add_days(1);
    date_string = gnote::utils::get_pretty_print_date(d, false, false);
    CHECK_EQUAL("Tomorrow", date_string);

    d = sharp::DateTime::now();
    d.add_days(-1);
    date_string = gnote::utils::get_pretty_print_date(d, false, false);
    CHECK_EQUAL("Yesterday", date_string);

    d = sharp::DateTime::from_iso8601("2009-03-24T13:34:35.2914680-04:00");
    date_string = gnote::utils::get_pretty_print_date(d, true, false);
    CHECK(Glib::str_has_suffix(date_string, "17:34"));

    date_string = gnote::utils::get_pretty_print_date(d, true, true);
    CHECK(Glib::str_has_suffix(date_string.lowercase(), "5:34 pm"));
  }

}

