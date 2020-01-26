/*
 * gnote
 *
 * Copyright (C) 2012,2017,2020 Aurimas Cernius
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
  Glib::TimeVal date(dt.to_unix(), dt.get_microsecond());
  localtime_r((const time_t *)&date.tv_sec, &t);
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
  Glib::TimeVal time_val(dt.to_unix(), dt.get_microsecond());
  char *iso8601 = g_time_val_to_iso8601(&time_val);
  if(iso8601) {
    retval = iso8601;
    if(time_val.tv_usec == 0) {
      // see http://bugzilla.gnome.org/show_bug.cgi?id=581844
      // when usec is 0, glib/libc does NOT add the usec values
      // to the output
      retval.insert(19, ".000000");
    }
    g_free(iso8601);
  }
  return retval;
}

Glib::DateTime date_time_from_iso8601(const Glib::ustring & dt)
{
  Glib::TimeVal time_val;
  if(g_time_val_from_iso8601(dt.c_str(), &time_val)) {
    return Glib::DateTime::create_now_local(time_val);
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

