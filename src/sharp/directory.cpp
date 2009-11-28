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

  void directory_copy(const Glib::RefPtr<Gio::File> & src,
                      const Glib::RefPtr<Gio::File> & dest)
                      throw(Gio::Error)
  {
    if (false == dest->query_exists()
        || Gio::FILE_TYPE_DIRECTORY
             != dest->query_file_type(Gio::FILE_QUERY_INFO_NONE))
        return;

    if (Gio::FILE_TYPE_REGULAR
          == src->query_file_type(Gio::FILE_QUERY_INFO_NONE)) {
      src->copy(dest->get_child(src->get_basename()),
                Gio::FILE_COPY_OVERWRITE);
    }
    else if (Gio::FILE_TYPE_DIRECTORY
                 == src->query_file_type(Gio::FILE_QUERY_INFO_NONE)) {
      const Glib::RefPtr<Gio::File> dest_dir
        = dest->get_child(src->get_basename());

      if (false == dest_dir->query_exists())
        dest_dir->make_directory_with_parents();

      Glib::Dir src_dir(src->get_path());

      for (Glib::Dir::iterator it = src_dir.begin();
           src_dir.end() != it; it++) {
        const Glib::RefPtr<Gio::File> file = src->get_child(*it);

        if (Gio::FILE_TYPE_DIRECTORY == file->query_file_type(
                                          Gio::FILE_QUERY_INFO_NONE))
          directory_copy(file, dest_dir);
        else
          file->copy(dest_dir->get_child(file->get_basename()),
                                         Gio::FILE_COPY_OVERWRITE);
      }
    }
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
