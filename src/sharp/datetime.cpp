

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

  int DateTime::year() const
  {
    struct tm result;
    localtime_r(&m_date.tv_sec, &result);
    return result.tm_year;
  }

  int DateTime::day_of_year() const
  {
    struct tm result;
    localtime_r(&m_date.tv_sec, &result);
    return result.tm_yday;
  }

	bool DateTime::is_valid() const
	{
		return ((m_date.tv_sec != -1) && (m_date.tv_usec != -1));
	}

  std::string DateTime::_to_string(const char * format, struct tm * t) const
  {
    char output[256];
    strftime(output, sizeof(output), format, t);
    return output;
  }

  std::string DateTime::to_string(const char * format) const
  {
    struct tm result; 
    return _to_string(format, localtime_r(&m_date.tv_sec, &result));
  }


  std::string DateTime::to_short_time_string() const
  {
    struct tm result;
    return _to_string("%R", localtime_r(&m_date.tv_sec, &result));
  }


	DateTime DateTime::now()
	{
		GTimeVal n;
		g_get_current_time(&n);
		return DateTime(n);
	}

  int DateTime::compare(const DateTime &a, const DateTime &b)
  {
    if(a > b)
      return +1;
    if(b > a)
      return -1;
    return 0;
  }


	bool DateTime::operator>(const DateTime & dt) const
	{
		if(m_date.tv_sec == dt.m_date.tv_sec) {
			return (m_date.tv_usec > dt.m_date.tv_usec);
		}
		return (m_date.tv_sec > dt.m_date.tv_sec);
	}


}
