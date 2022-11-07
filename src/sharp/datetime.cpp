/*
 * gnote
 *
 * Copyright (C) 2012,2017,2020-2022 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */



#include <time.h>

#include <glibmm/convert.h>

#include "sharp/datetime.hpp"


namespace sharp {

Glib::ustring date_time_to_string(const Glib::DateTime & dt, const char *format)
{
  struct tm t;
  time_t sec = dt.to_unix();
  localtime_r(&sec, &t);
  char output[256];
  strftime(output, sizeof(output), format, &t);
  return Glib::locale_to_utf8(output);
}

Glib::ustring date_time_to_string(const Glib::DateTime & dt, const Glib::ustring & format)
{
  return date_time_to_string(dt, format.c_str());
}

Glib::ustring date_time_to_iso8601(const Glib::DateTime & dt)
{
  Glib::ustring retval;
  if(!dt) {
    return retval;
  }

  Glib::DateTime date = dt.to_utc();
  char buffer[36] = {0};
  std::sprintf(buffer, "%d-%02d-%02dT%02d:%02d:%09.6lfZ", date.get_year(), date.get_month(), date.get_day_of_month(), date.get_hour(), date.get_minute(), date.get_seconds());
  retval = buffer;
  return retval;
}

Glib::DateTime date_time_from_iso8601(const Glib::ustring & dt)
{
  int y, M, d, h, m, tzh = 0, tzm = 0;
  double s;
  int parsed = std::sscanf(dt.c_str(), "%d-%d-%dT%d:%d:%lf%d:%dZ", &y, &M, &d, &h, &m, &s, &tzh, &tzm);
  if(6 <= parsed) {
    auto ret = Glib::DateTime::create_utc(y, M, d, h, m, s).to_local();
    if(tzh != 0) {
      if(tzh < 0) {
        tzh = -tzh;
      }
    }
    else {
      if(dt.size() > 27 && dt[27] == '+') {
        tzm = -tzm;
      }
    }
    if(tzh != 0) {
      ret = ret.add_hours(tzh);
    }
    if(tzm != 0) {
      ret = ret.add_minutes(tzm);
    }

    return ret;
  }

  return Glib::DateTime();
}

}

bool operator==(const Glib::DateTime & x, const Glib::DateTime & y)
{
  bool x_valid = bool(x);
  bool y_valid = bool(y);
  if(x_valid && y_valid) {
    return x.compare(y) == 0;
  }
  return x_valid == y_valid;
}

bool operator!=(const Glib::DateTime & x, const Glib::DateTime & y)
{
  bool x_valid = bool(x);
  bool y_valid = bool(y);
  if(x_valid && y_valid) {
    return x.compare(y) != 0;
  }
  return x_valid != y_valid;
}

bool operator<(const Glib::DateTime & x, const Glib::DateTime & y)
{
  bool x_valid = bool(x);
  bool y_valid = bool(y);
  if(x_valid && y_valid) {
    return x.compare(y) < 0;
  }
  if(x_valid == y_valid) {
    return false;
  }
  else if(x_valid) {
    return false;
  }
  return true;
}

bool operator<=(const Glib::DateTime & x, const Glib::DateTime & y)
{
  bool x_valid = bool(x);
  bool y_valid = bool(y);
  if(x_valid && y_valid) {
    return x.compare(y) <= 0;
  }
  if(x_valid == y_valid) {
    return true;
  }
  else if(x_valid) {
    return false;
  }
  return true;
}

bool operator>(const Glib::DateTime & x, const Glib::DateTime & y)
{
  bool x_valid = bool(x);
  bool y_valid = bool(y);
  if(x_valid && y_valid) {
    return x.compare(y) > 0;
  }
  if(x_valid == y_valid) {
    return false;
  }
  else if(x_valid) {
    return true;
  }
  return false;
}

bool operator>=(const Glib::DateTime & x, const Glib::DateTime & y)
{
  bool x_valid = bool(x);
  bool y_valid = bool(y);
  if(x_valid && y_valid) {
    return x.compare(y) >= 0;
  }
  if(x_valid == y_valid) {
    return true;
  }
  else if(x_valid) {
    return true;
  }
  return false;
}

