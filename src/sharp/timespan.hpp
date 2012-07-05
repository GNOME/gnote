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



#ifndef _SHARP_TIMESPAN_HPP_
#define _SHARP_TIMESPAN_HPP_


#include <string>


namespace sharp {

  class TimeSpan
  {
  public:
    TimeSpan(int hrs, int mins, int secs);
    TimeSpan(int days, int hrs, int mins, int secs);
    TimeSpan(int days, int hrs, int mins, int secs, int usecs);
    int days() const
      {
        return m_days;
      }
    int hours() const
      {
        return m_hours;
      }
    int minutes() const
      {
        return m_minutes;
      }
    int seconds() const
      {
        return m_seconds;
      }
    int microseconds() const
      {
        return m_usecs;
      }
    double total_days() const;
    double total_hours() const;
    double total_minutes() const;
    double total_seconds() const;
    double total_milliseconds() const;
    std::string string() const;
    TimeSpan operator-(const TimeSpan & ts);

    static TimeSpan parse(const std::string & s);
  private:
    int _total_hours() const;
    int _total_minutes() const;
    int _total_seconds() const;
    double _remaining_hours() const;
    double _remaining_minutes() const;
    double _remaining_seconds() const;

    int m_days;
    int m_hours;
    int m_minutes;
    int m_seconds;
    int m_usecs;
  };

}


#endif
