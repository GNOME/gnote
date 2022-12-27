/*
 * gnote
 *
 * Copyright (C) 2018-2019,2022 Aurimas Cernius
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <glibmm/miscutils.h>
#include <UnitTest++/UnitTest++.h>

#include "sharp/directory.hpp"

#include <glibmm/fileutils.h>

SUITE(directory)
{
  TEST(get_directories__ustr__non_existent_directory)
  {
    Glib::ustring dir = Glib::build_filename(Glib::path_get_dirname(__FILE__), "nonexistent");

    std::vector<Glib::ustring> directories = sharp::directory_get_directories(dir);
    CHECK_EQUAL(0, directories.size());
  }

  TEST(get_directories__File__non_existent_directory)
  {
    auto dir = Gio::File::create_for_path(Glib::build_filename(Glib::path_get_dirname(__FILE__), "nonexistent"));

    std::vector<Glib::RefPtr<Gio::File>> directories = sharp::directory_get_directories(dir);
    CHECK_EQUAL(0, directories.size());
  }

  TEST(get_directories__ustr__regular_file)
  {
    Glib::ustring dir(__FILE__);

    std::vector<Glib::ustring> directories = sharp::directory_get_directories(dir);
    CHECK_EQUAL(0, directories.size());
  }

  TEST(get_directories__File__regular_file)
  {
    auto dir = Gio::File::create_for_path(__FILE__);

    std::vector<Glib::RefPtr<Gio::File>> directories = sharp::directory_get_directories(dir);
    CHECK_EQUAL(0, directories.size());
  }

  TEST(get_directories__ustr__no_subdirs)
  {
    Glib::ustring dir = Glib::build_filename(Glib::path_get_dirname(__FILE__), ".deps");

    std::vector<Glib::ustring> directories = sharp::directory_get_directories(dir);
    CHECK_EQUAL(0, directories.size());
  }

  TEST(get_directories__File__no_subdirs)
  {
    auto dir = Gio::File::create_for_path(Glib::build_filename(Glib::path_get_dirname(__FILE__), ".deps"));

    std::vector<Glib::RefPtr<Gio::File>> directories = sharp::directory_get_directories(dir);
    CHECK_EQUAL(0, directories.size());
  }

#if 0 // TODO rewrite using generated directory tree
  TEST(get_directories__ustr__one_subdir)
  {
    Glib::ustring dir = Glib::path_get_dirname(__FILE__);

    std::vector<Glib::ustring> directories = sharp::directory_get_directories(dir);
    CHECK_EQUAL(1, directories.size());
  }

  TEST(get_directories__File__one_subdir)
  {
    auto dir = Gio::File::create_for_path(Glib::path_get_dirname(__FILE__));

    std::vector<Glib::RefPtr<Gio::File>> directories = sharp::directory_get_directories(dir);
    CHECK_EQUAL(1, directories.size());
  }
#endif

  void remove_matching_files(const std::vector<Glib::RefPtr<Gio::File>> & dirsf,
      std::vector<Glib::ustring> & dirss)
  {
    for(auto f : dirsf) {
      auto name = Glib::path_get_basename(f->get_path());
      for(auto iter = dirss.begin(); iter != dirss.end(); ++iter) {
        if(name == Glib::path_get_basename(iter->c_str())) {
          dirss.erase(iter);
          break;
        }
      }
    }
  }

  TEST(get_directories__same_return)
  {
    auto dir = Glib::path_get_dirname(Glib::path_get_dirname(__FILE__));

    std::vector<Glib::ustring> dirss = sharp::directory_get_directories(dir);

    auto file = Gio::File::create_for_path(dir);

    std::vector<Glib::RefPtr<Gio::File>> dirsf = sharp::directory_get_directories(file);

    CHECK(0 < dirss.size());
    CHECK_EQUAL(dirss.size(), dirsf.size());
    remove_matching_files(dirsf, dirss);
    CHECK_EQUAL(0, dirss.size());
  }

  TEST(directory_get_files_with_ext__ustr__non_existent_dir)
  {
    Glib::ustring dir = Glib::build_filename(Glib::path_get_dirname(__FILE__), "nonexistent");

    std::vector<Glib::ustring> files = sharp::directory_get_files_with_ext(dir, "");
    CHECK_EQUAL(0, files.size());
  }

  TEST(directory_get_files_with_ext__File__non_existent_dir)
  {
    auto dir = Gio::File::create_for_path(Glib::build_filename(Glib::path_get_dirname(__FILE__), "nonexistent"));

    std::vector<Glib::RefPtr<Gio::File>> files = sharp::directory_get_files_with_ext(dir, "");
    CHECK_EQUAL(0, files.size());
  }

  TEST(directory_get_files_with_ext__ustr__regular_file)
  {
    Glib::ustring dir(__FILE__);

    std::vector<Glib::ustring> files = sharp::directory_get_files_with_ext(dir, "");
    CHECK_EQUAL(0, files.size());
  }

  TEST(directory_get_files_with_ext__File__regular_file)
  {
    auto dir = Gio::File::create_for_path(__FILE__);

    std::vector<Glib::RefPtr<Gio::File>> files = sharp::directory_get_files_with_ext(dir, "");
    CHECK_EQUAL(0, files.size());
  }

  void directory_get_files_with_ext__same_return_test(const Glib::ustring & ext)
  {
    Glib::ustring dir = Glib::path_get_dirname(__FILE__);

    std::vector<Glib::ustring> filess = sharp::directory_get_files_with_ext(dir, ext);
    CHECK(0 < filess.size());
    if(ext.size()) {
      for(auto f : filess) {
        auto pos = f.find_last_of('.');
        CHECK_EQUAL(ext, f.substr(pos));
      }
    }

    auto file = Gio::File::create_for_path(dir);
    std::vector<Glib::RefPtr<Gio::File>> filesf = sharp::directory_get_files_with_ext(file, ext);
    CHECK_EQUAL(filess.size(), filesf.size());
    remove_matching_files(filesf, filess);
    CHECK_EQUAL(0, filess.size());
  }

  TEST(directory_get_files_with_ext__same_return)
  {
    directory_get_files_with_ext__same_return_test("");
  }

  TEST(directory_get_files_with_ext__same_return_filterred)
  {
    directory_get_files_with_ext__same_return_test(".cpp");
  }
}

