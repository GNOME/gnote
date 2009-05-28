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

#include <boost/version.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

#include "files.hpp"


namespace sharp {


  bool file_exists(const std::string & file)
  {
    boost::filesystem::path p(file);
    // is_regular_file isn't in 1.34. is_regular is deprecated.
    return (exists(p) && is_regular(p));
  }


  std::string file_basename(const std::string & p)
  {
#if BOOST_VERSION >= 103600
    return boost::filesystem::path(p).stem();
#else
    return boost::filesystem::basename(boost::filesystem::path(p));
#endif
  }

  std::string file_dirname(const std::string & p)
  {
    return boost::filesystem::path(p).branch_path().string();
  }


  std::string file_filename(const std::string & p)
  {
    return boost::filesystem::path(p).leaf();
  }

  void file_delete(const std::string & p)
  {
    boost::filesystem::remove(p);
  }


  void file_copy(const std::string & source, const std::string & dest)
  {
    boost::filesystem::copy_file(source, dest);
  }

  void file_move(const std::string & from, const std::string & to)
  {
    boost::filesystem::rename(from, to);
  }
}

