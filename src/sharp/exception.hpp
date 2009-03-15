


#ifndef __SHARP_EXCEPTION_HPP_
#define __SHARP_EXCEPTION_HPP_

#include <exception>
#include <string>

namespace sharp {


class Exception
	: public std::exception
{
public:
	Exception(const std::string & m) throw()
		: m_what(m)
		{
		}
	virtual ~Exception() throw();

	virtual const char *what() const throw();

private:
	std::string m_what;
};

}

#endif
