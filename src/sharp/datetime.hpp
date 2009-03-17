


#ifndef __SHARP_DATETIME_HPP__
#define __SHARP_DATETIME_HPP__

#include <glib.h>

namespace sharp {


class DateTime
{
public:
	DateTime();
	explicit DateTime(time_t t);
	
	DateTime & add_days(int days);
	DateTime & add_hours(int hours);
	bool is_valid() const;
	bool operator>(const DateTime & dt) const;
	
	static DateTime now();

private:
	// implementation detail. really make public if needed.
	explicit DateTime(const GTimeVal & v);
	GTimeVal m_date;
};


}

#endif
