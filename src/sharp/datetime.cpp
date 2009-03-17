

#include <time.h>

#include "sharp/datetime.hpp"


namespace sharp {

#define SEC_PER_MINUTE 60
#define SEC_PER_HOUR (SEC_PER_MINUTE * 60)
#define SEC_PER_DAY (SEC_PER_HOUR * 24)

	DateTime::DateTime()
	{
		m_date.tv_sec = -1;
		m_date.tv_usec = -1;
	}

	DateTime::DateTime(time_t t)
	{
		m_date.tv_sec = t;
		m_date.tv_usec = 0;		
	}

	DateTime::DateTime(const GTimeVal & v)
		: m_date(v)
	{
	}

	DateTime & DateTime::add_days(int days)
	{
		m_date.tv_sec += (days * SEC_PER_DAY);
		return *this;
	}

	DateTime & DateTime::add_hours(int hours)
	{
		m_date.tv_sec += (hours * SEC_PER_HOUR);
		return *this;
	}

	bool DateTime::is_valid() const
	{
		return ((m_date.tv_sec != -1) && (m_date.tv_usec != -1));
	}

	DateTime DateTime::now()
	{
		GTimeVal n;
		g_get_current_time(&n);
		return DateTime(n);
	}

	bool DateTime::operator>(const DateTime & dt) const
	{
		if(m_date.tv_sec == dt.m_date.tv_sec) {
			return (m_date.tv_usec > dt.m_date.tv_usec);
		}
		return (m_date.tv_sec > dt.m_date.tv_sec);
	}


}
