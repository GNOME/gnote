


#ifndef __SHARP_DIRECTORY_HPP_
#define __SHARP_DIRECTORY_HPP_

#include <list>
#include <string>


namespace sharp {

	std::list<std::string> directory_get_files_with_ext(const std::string & dir, const std::string & ext);

}


#endif
