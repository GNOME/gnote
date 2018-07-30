/*
 * gnote
 *
 * Copyright (C) 2011-2014,2017-2018 Aurimas Cernius
 * Copyright (C) 2011 Debarshi Ray
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



#include <glib/gstdio.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include "sharp/directory.hpp"
#include "sharp/fileinfo.hpp"
#include "sharp/string.hpp"

namespace sharp {


  void directory_get_files_with_ext(const Glib::ustring & dir,
                                    const Glib::ustring & ext,
                                    std::list<Glib::ustring> & list)
  {
    if (!Glib::file_test(dir, Glib::FILE_TEST_EXISTS))
      return;

    if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR))
      return;

    Glib::Dir d(dir);

    for (Glib::Dir::iterator itr = d.begin(); itr != d.end(); ++itr) {
      const Glib::ustring file(dir + "/" + *itr);
      const sharp::FileInfo file_info(file);
      const Glib::ustring extension = file_info.get_extension();

      if (Glib::file_test(file, Glib::FILE_TEST_IS_REGULAR)
          && (ext.empty() || (Glib::ustring(extension).lowercase() == ext))) {
        list.push_back(file);
      }
    }
  }

  void directory_get_directories(const Glib::ustring & dir,
                                 std::list<Glib::ustring> & files)
  {
    if(!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR)) {
      return;
    }

    Glib::Dir d(dir);

    for(Glib::Dir::iterator iter = d.begin(); iter != d.end(); ++iter) {
      const Glib::ustring file(dir + "/" + *iter);

      if(Glib::file_test(file, Glib::FILE_TEST_IS_DIR)) {
        files.push_back(file);
      }
    }
  }

  void directory_get_directories(const Glib::RefPtr<Gio::File> & dir,
                                 std::vector<Glib::RefPtr<Gio::File>> & files)
  {
    if(!dir || !dir->query_exists()) {
      return;
    }
    auto info = dir->query_info();
    if(!info || info->get_file_type() != Gio::FILE_TYPE_DIRECTORY) {
      return;
    }

    auto children = dir->enumerate_children();
    while(true) {
      auto fileinfo = children->next_file();
      if(!fileinfo) {
        break;
      }
      if(fileinfo->get_file_type() & Gio::FILE_TYPE_DIRECTORY) {
        auto child = Gio::File::create_for_uri(Glib::build_filename(dir->get_uri(), fileinfo->get_name()));
        files.push_back(child);
      }
    }
  }

  void directory_get_files(const Glib::ustring & dir, std::list<Glib::ustring> & files)
  {
    directory_get_files_with_ext(dir, "", files);
  }

  bool directory_exists(const Glib::ustring & dir)
  {
    return Glib::file_test(dir, Glib::FILE_TEST_EXISTS) && Glib::file_test(dir, Glib::FILE_TEST_IS_DIR);
  }

  void directory_copy(const Glib::RefPtr<Gio::File> & src,
                      const Glib::RefPtr<Gio::File> & dest)
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

  bool directory_create(const Glib::ustring & dir)
  {
    return Gio::File::create_for_path(dir)->make_directory_with_parents();
  }

  bool directory_delete(const Glib::ustring & dir, bool recursive)
  {
    if(!recursive) {
      std::list<Glib::ustring> files;
      directory_get_files(dir, files);
      if(files.size()) {
        return false;
      }
    }

    return g_remove(dir.c_str()) == 0;
  }

}
