

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "sharp/directory.hpp"

namespace sharp {

	std::list<std::string> directory_get_files(const std::string & dir, const std::string & glob)
	{
		std::list<std::string> list;

		boost::filesystem::path p(dir);
		
		boost::filesystem::directory_iterator end_itr; 
		for ( boost::filesystem::directory_iterator itr( p );
				  itr != end_itr;
				  ++itr )
		{
			if ( is_regular_file(*itr) )
			{
//				if( filter(*itr) ) {
				list.push_back(itr->string());
//					DBG_OUT( "found file %s", itr->string().c_str() );
//				}
			}
		}

		return list;
	}


}
