


#ifndef __SHARP_DATETIME_HPP_
#define __SHARP_DATETIME_HPP_ 

#include <string>

#include "sharp/datetime.hpp"

namespace sharp {

	class XmlConvert
	{
	public:
		static DateTime to_date_time(const std::string & date, const std::string & format);
		static std::string to_string(const DateTime & date, const std::string & format);
	};

}


#endif

