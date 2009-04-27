/*
 * gnote
 *
 * Copyright (C) 2009 Hubert Figuiere
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */



#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "sharp/directory.hpp"
#include "sharp/string.hpp"

namespace sharp {


  void directory_get_files_with_ext(const std::string & dir, 
                                    const std::string & ext,
                                    std::list<std::string> & list)
  {
    boost::filesystem::path p(dir);
    
    if(!exists(p)) {
      return;
    }
    boost::filesystem::directory_iterator end_itr; 
    for ( boost::filesystem::directory_iterator itr( p );
          itr != end_itr;
          ++itr )
    {
      // is_regular() is deprecated but is_regular_file isn't in 1.34.
      if ( is_regular(*itr) && (ext.empty() || (sharp::string_to_lower(extension(*itr)) == ext)) )
      {
        list.push_back(itr->string());
      }
    }
  }

  void directory_get_files(const std::string & dir, std::list<std::string>  & files)
  {
    directory_get_files_with_ext(dir, "", files);
  }

  bool directory_exists(const std::string & dir)
  {
    boost::filesystem::path p(dir);
    return (exists(p) && is_directory(p));
  }

  bool directory_create(const std::string & dir)
  {
    try {
      return boost::filesystem::create_directories(dir);
    }
    catch(...) {
    }
    return false;
  }

}
