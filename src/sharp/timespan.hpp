/*
 * gnote
 *
 * Copyright (C) 2012,2017,2020 Aurimas Cernius
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


#include <glibmm/datetime.h>
#include <glibmm/ustring.h>


namespace sharp {

Glib::TimeSpan time_span(int hrs, int mins, int secs);
Glib::TimeSpan time_span(int days, int hrs, int mins, int secs, int usecs);
Glib::TimeSpan time_span_parse(const Glib::ustring & s);
double time_span_total_minutes(Glib::TimeSpan ts);
double time_span_total_seconds(Glib::TimeSpan ts);
double time_span_total_milliseconds(Glib::TimeSpan ts);
Glib::ustring time_span_string(Glib::TimeSpan ts);

}


#endif
