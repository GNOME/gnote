/*
 * gnote
 *
 * Copyright (C) 2013 Aurimas Cernius
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



#include <iostream>
#include <boost/test/minimal.hpp>

#include "sharp/xmlconvert.cpp"
#include "utils.hpp"



bool string_ends_with(const std::string & s, const std::string & other)
{
  return (s.length() - s.rfind(other)) == other.length();
}

std::string string_to_lower(std::string s)
{
  for(std::string::size_type i = 0; i < s.size(); ++i) {
    if(std::isalpha(s[i]) && std::isupper(s[i])) {
      s[i] = std::tolower(s[i]);
    }
  }
  return s;
}

int test_main(int /*argc*/, char ** /*argv*/)
{
  // force certain timezone so that time tests work
  setenv("TZ", "Europe/London", 1);

  sharp::DateTime d(678901234, 67890);

  std::string date_string = sharp::XmlConvert::to_string(d);
  BOOST_CHECK(date_string == "1991-07-07T15:40:34.067890Z");

  sharp::DateTime d2 = sharp::DateTime::from_iso8601(date_string);
  
  BOOST_CHECK(d == d2);

  sharp::DateTime d3 = sharp::DateTime::from_iso8601("2009-03-24T03:34:35.2914680-04:00");
  BOOST_CHECK(d3.is_valid());

  // check when usec is 0.
  // see http://bugzilla.gnome.org/show_bug.cgi?id=581844
  d3.set_usec(0);

  date_string = sharp::XmlConvert::to_string(d3);
  BOOST_CHECK(date_string == "2009-03-24T07:34:35.000000Z");

  sharp::DateTime d4 = sharp::DateTime::now();
  date_string = gnote::utils::get_pretty_print_date(d4, false, false);
  BOOST_CHECK(date_string == "Today");

  d4.add_days(1);
  date_string = gnote::utils::get_pretty_print_date(d4, false, false);
  BOOST_CHECK(date_string == "Tomorrow");

  sharp::DateTime d5 = sharp::DateTime::now();
  d5.add_days(-1);
  date_string = gnote::utils::get_pretty_print_date(d5, false, false);
  BOOST_CHECK(date_string == "Yesterday");

  sharp::DateTime d6 = sharp::DateTime::from_iso8601("2009-03-24T13:34:35.2914680-04:00");
  date_string = gnote::utils::get_pretty_print_date(d6, true, false);
  BOOST_CHECK(string_ends_with(date_string, "17:34"));

  date_string = string_to_lower(gnote::utils::get_pretty_print_date(d6, true, true));
  BOOST_CHECK(string_ends_with(date_string, "5:34 pm"));

  return 0;
}

