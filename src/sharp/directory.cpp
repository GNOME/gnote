/*
 * gnote
 *
 * Copyright (C) 2011-2014,2017-2019,2022 Aurimas Cernius
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

#include "debug.hpp"

namespace sharp {


  std::vector<Glib::ustring> directory_get_files_with_ext(const Glib::ustring & dir, const Glib::ustring & ext)
  {
    std::vector<Glib::ustring> list;
    if (!Glib::file_test(dir, Glib::FileTest::EXISTS))
      return list;

    if (!Glib::file_test(dir, Glib::FileTest::IS_DIR))
      return list;

    Glib::Dir d(dir);

    for (Glib::Dir::iterator itr = d.begin(); itr != d.end(); ++itr) {
      const Glib::ustring file(dir + "/" + *itr);
      const sharp::FileInfo file_info(file);
      const Glib::ustring extension = file_info.get_extension();

      if (Glib::file_test(file, Glib::FileTest::IS_REGULAR)
          && (ext.empty() || (Glib::ustring(extension).lowercase() == ext))) {
        list.push_back(file);
      }
    }

    return list;
  }

  std::vector<Glib::RefPtr<Gio::File>> directory_get_files_with_ext(const Glib::RefPtr<Gio::File> & dir,
                                    const Glib::ustring & ext)
  {
    std::vector<Glib::RefPtr<Gio::File>> files;
    if(!directory_exists(dir)) {
      return files;
    }

    auto children = dir->enumerate_children();
    while(true) {
      auto fileinfo = children->next_file();
      if(!fileinfo) {
        break;
      }
      if(fileinfo->get_file_type() == Gio::FileType::REGULAR) {
        if(ext.size()) {
          Glib::ustring name = fileinfo->get_name();
          auto pos = name.find_last_of('.');
          if(pos != Glib::ustring::npos && name.substr(pos) == ext) {
            auto child = Gio::File::create_for_uri(Glib::build_filename(dir->get_uri(), name));
            files.push_back(child);
          }
        }
        else {
          auto child = Gio::File::create_for_uri(Glib::build_filename(dir->get_uri(), fileinfo->get_name()));
          files.push_back(child);
        }
      }
    }

    return files;
  }

  std::vector<Glib::ustring> directory_get_directories(const Glib::ustring & dir)
  {
    std::vector<Glib::ustring> files;
    if(!Glib::file_test(dir, Glib::FileTest::IS_DIR)) {
      return files;
    }

    Glib::Dir d(dir);

    for(Glib::Dir::iterator iter = d.begin(); iter != d.end(); ++iter) {
      const Glib::ustring file(dir + "/" + *iter);

      if(Glib::file_test(file, Glib::FileTest::IS_DIR)) {
        files.push_back(file);
      }
    }

    return files;
  }

  std::vector<Glib::RefPtr<Gio::File>> directory_get_directories(const Glib::RefPtr<Gio::File> & dir)
  {
    std::vector<Glib::RefPtr<Gio::File>> files;
    if(!directory_exists(dir)) {
      return files;
    }

    auto children = dir->enumerate_children();
    while(true) {
      auto fileinfo = children->next_file();
      if(!fileinfo) {
        break;
      }
      if(fileinfo->get_file_type() == Gio::FileType::DIRECTORY) {
        auto child = Gio::File::create_for_uri(Glib::build_filename(dir->get_uri(), fileinfo->get_name()));
        files.push_back(child);
      }
    }

    return files;
  }

  std::vector<Glib::ustring> directory_get_files(const Glib::ustring & dir)
  {
    return directory_get_files_with_ext(dir, "");
  }

  std::vector<Glib::RefPtr<Gio::File>> directory_get_files(const Glib::RefPtr<Gio::File> & dir)
  {
    return directory_get_files_with_ext(dir, "");
  }

  bool directory_exists(const Glib::ustring & dir)
  {
    return Glib::file_test(dir, Glib::FileTest::EXISTS|Glib::FileTest::IS_DIR);
  }

  bool directory_exists(const Glib::RefPtr<Gio::File> & dir)
  {
    if(!dir || !dir->query_exists()) {
      return false;
    }
    auto info = dir->query_info();
    if(!info || info->get_file_type() != Gio::FileType::DIRECTORY) {
      return false;
    }

    return true;
  }

  void directory_copy(const Glib::RefPtr<Gio::File> & src,
                      const Glib::RefPtr<Gio::File> & dest)
  {
    if (false == dest->query_exists()
        || Gio::FileType::DIRECTORY != dest->query_file_type(Gio::FileQueryInfoFlags::NONE))
        return;

    if (Gio::FileType::REGULAR == src->query_file_type(Gio::FileQueryInfoFlags::NONE)) {
      src->copy(dest->get_child(src->get_basename()), Gio::File::CopyFlags::OVERWRITE);
    }
    else if (Gio::FileType::DIRECTORY == src->query_file_type(Gio::FileQueryInfoFlags::NONE)) {
      const Glib::RefPtr<Gio::File> dest_dir = dest->get_child(src->get_basename());

      if (false == dest_dir->query_exists())
        dest_dir->make_directory_with_parents();

      Glib::Dir src_dir(src->get_path());

      for (Glib::Dir::iterator it = src_dir.begin();
           src_dir.end() != it; it++) {
        const Glib::RefPtr<Gio::File> file = src->get_child(*it);

        if (Gio::FileType::DIRECTORY == file->query_file_type(Gio::FileQueryInfoFlags::NONE))
          directory_copy(file, dest_dir);
        else
          file->copy(dest_dir->get_child(file->get_basename()), Gio::File::CopyFlags::OVERWRITE);
      }
    }
  }

  bool directory_create(const Glib::ustring & dir)
  {
    return directory_create(Gio::File::create_for_path(dir));
  }

  bool directory_create(const Glib::RefPtr<Gio::File> & dir)
  {
    return dir->make_directory_with_parents();
  }

  bool directory_delete(const Glib::ustring & dir, bool recursive)
  {
    if(!recursive) {
      std::vector<Glib::ustring> files = directory_get_files(dir);
      if(files.size()) {
        return false;
      }
    }

    return g_remove(dir.c_str()) == 0;
  }

  bool directory_delete(const Glib::RefPtr<Gio::File> & dir, bool recursive)
  {
    if(recursive) {
      std::vector<Glib::RefPtr<Gio::File>> files = directory_get_files(dir);
      for(auto file : files) {
        if(!file->remove()) {
          ERR_OUT("Failed to remove file %s", file->get_uri().c_str());
          return false;
        }
      }
      files = directory_get_directories(dir);
      for(auto d : files) {
        if(!directory_delete(d, true)) {
          ERR_OUT("Failed to remove directory %s", d->get_uri().c_str());
          return false;
        }
      }
    }

    return dir->remove();
  }

}
