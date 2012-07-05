/*
 * gnote
 *
 * Copyright (C) 2012 Aurimas Cernius
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


#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include "string.hpp"
#include "timespan.hpp"


namespace sharp {

  TimeSpan::TimeSpan(int hrs, int mins, int secs)
    : m_days(0)
    , m_hours(hrs)
    , m_minutes(mins)
    , m_seconds(secs)
    , m_usecs(0)
  {}


  TimeSpan::TimeSpan(int d, int hrs, int mins, int secs)
    : m_days(d)
    , m_hours(hrs)
    , m_minutes(mins)
    , m_seconds(secs)
    , m_usecs(0)
  {}


  TimeSpan::TimeSpan(int d, int hrs, int mins, int secs, int usecs)
    : m_days(d)
    , m_hours(hrs)
    , m_minutes(mins)
    , m_seconds(secs)
    , m_usecs(usecs)
  {}


  double TimeSpan::total_days() const
  {
    return m_days + _remaining_hours() / 24.0;
  }


  double TimeSpan::total_hours() const
  {
    return _total_hours() + _remaining_minutes() / 60.0;
  }


  double TimeSpan::total_minutes() const
  {
    return _total_minutes() + _remaining_seconds() / 60.0;
  }


  double TimeSpan::total_seconds() const
  {
    return _total_seconds() + m_usecs / 1000000.0;
  }


  double TimeSpan::total_milliseconds() const
  {
    return _total_seconds() * 60.0 + m_usecs / 1000.0;
  }


  std::string TimeSpan::string() const
  {
    return str(boost::format("%1%:%2%:%3%:%4%:%5%") % m_days % m_hours % m_minutes % m_seconds % m_usecs);
  }


  TimeSpan TimeSpan::operator-(const TimeSpan & ts)
  {
    double result = total_milliseconds() - ts.total_milliseconds();
    int secs = int(result / 1000);
    int usecs = int(result - (secs * 1000));
    int mins = secs / 60;
    secs %= 60;
    int hrs = mins / 60;
    mins %= 60;
    int ds = hrs / 24;
    hrs %= 24;
    return TimeSpan(ds, hrs, mins, secs, usecs);
  }


  TimeSpan TimeSpan::parse(const std::string & s)
  {
    std::vector<std::string> tokens;
    sharp::string_split(tokens, s, ":");
    if(tokens.size() != 5) {
      return TimeSpan(0, 0, 0, 0, 0);
    }
    int days = boost::lexical_cast<int>(tokens[0]);
    int hours = boost::lexical_cast<int>(tokens[1]);
    int mins = boost::lexical_cast<int>(tokens[2]);
    int secs = boost::lexical_cast<int>(tokens[3]);
    int usecs = boost::lexical_cast<int>(tokens[4]);
    boost::format fmt("%1%:%2%:%3%:%4%:%5%");
    fmt % days % hours % mins % secs % usecs;
    if(str(fmt) != s) {
      return TimeSpan(0, 0, 0, 0, 0);
    }

    return TimeSpan(days, hours, mins, secs, usecs);
  }


  int TimeSpan::_total_hours() const
  {
    return m_days * 24 + m_hours;
  }


  int TimeSpan::_total_minutes() const
  {
    return _total_hours() * 60 + m_minutes;
  }


  int TimeSpan::_total_seconds() const
  {
    return _total_minutes() * 60 + m_seconds;
  }


  double TimeSpan::_remaining_hours() const
  {
    return m_hours + _remaining_minutes() / 60.0;
  }


  double TimeSpan::_remaining_minutes() const
  {
    return m_minutes + _remaining_seconds() / 60.0;
  }


  double TimeSpan::_remaining_seconds() const
  {
    return m_seconds + m_usecs / 1000000.0;
  }

}
