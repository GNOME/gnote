


#ifndef __SHARP_DATETIME_HPP__
#define __SHARP_DATETIME_HPP__

#include <time.h>

#include <string>

#include <glib.h>

namespace sharp {


class DateTime
{
public:
	DateTime();
	explicit DateTime(time_t t);
	
	DateTime & add_days(int days);
	DateTime & add_hours(int hours);

  int year() const;
  int day_of_year() const;

	bool is_valid() const;
	bool operator>(const DateTime & dt) const;

  std::string to_string(const char * format) const;
  std::string to_short_time_string() const;
	static DateTime now();
  static int compare(const DateTime &, const DateTime &);

private:
  // return the string formatted according to strftime
  std::string _to_string(const char * format, struct tm *) const;
	// implementation detail. really make public if needed.
	explicit DateTime(const GTimeVal & v);
	GTimeVal m_date;
};


}

#endif
