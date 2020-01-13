/*
 * gnote
 *
 * Copyright (C) 2012-2013,2017,2020 Aurimas Cernius
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


#include "base/macros.hpp"
#include "string.hpp"
#include "timespan.hpp"


namespace sharp {

Glib::TimeSpan time_span(int hrs, int mins, int secs)
{
  return hrs * G_TIME_SPAN_HOUR + mins * G_TIME_SPAN_MINUTE + secs * G_TIME_SPAN_SECOND;
}

Glib::TimeSpan time_span(int days, int hrs, int mins, int secs, int usecs)
{
  return days * G_TIME_SPAN_DAY + hrs * G_TIME_SPAN_HOUR + mins * G_TIME_SPAN_MINUTE + secs * G_TIME_SPAN_SECOND + usecs;
}

Glib::TimeSpan time_span_parse(const Glib::ustring & s)
{
  std::vector<Glib::ustring> tokens;
  sharp::string_split(tokens, s, ":");
  if(tokens.size() != 5) {
    return time_span(0, 0, 0, 0, 0);
  }
  int days = STRING_TO_INT(tokens[0]);
  int hours = STRING_TO_INT(tokens[1]);
  int mins = STRING_TO_INT(tokens[2]);
  int secs = STRING_TO_INT(tokens[3]);
  int usecs = STRING_TO_INT(tokens[4]);
  Glib::ustring fmt = Glib::ustring::compose("%1:%2:%3:%4:%5", days, hours, mins, secs, usecs);
  if(fmt != s) {
    return time_span(0, 0, 0, 0, 0);
  }

  return time_span(days, hours, mins, secs, usecs);
}

double time_span_total_minutes(Glib::TimeSpan ts)
{
  return double(ts) / G_TIME_SPAN_MINUTE;
}

double time_span_total_seconds(Glib::TimeSpan ts)
{
  return double(ts) / G_TIME_SPAN_SECOND;
}

double time_span_total_milliseconds(Glib::TimeSpan ts)
{
  return double(ts) / G_TIME_SPAN_MILLISECOND;
}

Glib::ustring time_span_string(Glib::TimeSpan ts)
{
  unsigned days = ts / G_TIME_SPAN_DAY;
  ts = ts % G_TIME_SPAN_DAY;
  unsigned hours = ts / G_TIME_SPAN_HOUR;
  ts = ts % G_TIME_SPAN_HOUR;
  unsigned minutes = ts / G_TIME_SPAN_MINUTE;
  ts = ts % G_TIME_SPAN_MINUTE;
  unsigned seconds = ts / G_TIME_SPAN_SECOND;
  unsigned usecs = ts % G_TIME_SPAN_SECOND;
  return Glib::ustring::compose("%1:%2:%3:%4:%5", days, hours, minutes, seconds, usecs);
}

}
