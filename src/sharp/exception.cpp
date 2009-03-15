


#include "sharp/exception.hpp"


namespace sharp {

	Exception::~Exception() throw()
	{
	}

	const char *Exception::what() const throw()
	{
		return m_what.c_str();
	}

}

