


#ifndef __SHARP_DIRECTORY_HPP_
#define __SHARP_DIRECTORY_HPP_

#include <list>
#include <string>


namespace sharp {

	std::list<std::string> directory_get_files(const std::string & dir, const std::string & glob);

}


#endif
