

#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/path.hpp>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem/operations.hpp>

#include "sharp/directory.hpp"
#include "sharp/string.hpp"

namespace sharp {


	std::list<std::string> directory_get_files_with_ext(const std::string & dir, 
                                                      const std::string & ext)
	{
		std::list<std::string> list;

		boost::filesystem::path p(dir);
		
		boost::filesystem::directory_iterator end_itr; 
		for ( boost::filesystem::directory_iterator itr( p );
				  itr != end_itr;
				  ++itr )
		{
      // is_regular() is deprecated but is_regular_file isn't in 1.34.
			if ( is_regular(*itr) && (sharp::string_to_lower(extension(*itr)) == ext) )
			{
        list.push_back(itr->string());
			}
		}

		return list;
	}


}
