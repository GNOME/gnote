/*
 * gnote
 *
 * Copyright (C) 2017-2020 Aurimas Cernius
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

#include <fstream>
#include <stdlib.h>
#include <unistd.h>

#include <glibmm/miscutils.h>
#include <UnitTest++/UnitTest++.h>

#include "sharp/exception.hpp"
#include "sharp/files.hpp"


SUITE(files)
{
  TEST(basename)
  {
    CHECK_EQUAL("baz", sharp::file_basename("/foo/bar/baz.txt"));
    CHECK_EQUAL("baz", sharp::file_basename("/foo/bar/baz"));
    CHECK_EQUAL(".", sharp::file_basename("/foo/bar/.."));
    CHECK_EQUAL("bar", sharp::file_basename("/foo/bar/"));
  }

  TEST(dirname)
  {
    CHECK_EQUAL("/foo/bar", sharp::file_dirname("/foo/bar/baz.txt"));
    CHECK_EQUAL("/foo/bar", sharp::file_dirname("/foo/bar/baz"));
    CHECK_EQUAL("/foo/bar", sharp::file_dirname("/foo/bar/.."));
    CHECK_EQUAL("/foo/bar", sharp::file_dirname("/foo/bar/"));
  }

  TEST(filename_ustr)
  {
    CHECK_EQUAL("baz.txt", sharp::file_filename("/foo/bar/baz.txt"));
    CHECK_EQUAL("baz", sharp::file_filename("/foo/bar/baz"));
    CHECK_EQUAL("..", sharp::file_filename("/foo/bar/.."));
    CHECK_EQUAL("bar", sharp::file_filename("/foo/bar/"));
  }

  TEST(filename_File)
  {
    CHECK_EQUAL("", sharp::file_filename(Glib::RefPtr<Gio::File>()));
    CHECK_EQUAL("baz.txt", sharp::file_filename(Gio::File::create_for_path("/foo/bar/baz.txt")));
    CHECK_EQUAL("baz", sharp::file_filename(Gio::File::create_for_path("/foo/bar/baz")));
    CHECK_EQUAL("bar", sharp::file_filename(Gio::File::create_for_path("/foo/bar/")));
  }

  TEST(exists)
  {
    Glib::ustring dir = Glib::get_current_dir();

    CHECK(sharp::file_exists(dir) == false);
    // Very unlikely to exist.
    CHECK(sharp::file_exists(__FILE__ __FILE__) == false);
    CHECK(sharp::file_exists(__FILE__) == true);
  }

  TEST(read_all_lines)
  {
    std::vector<Glib::ustring> lines;
    // very unlikely to exist
    CHECK_THROW(sharp::file_read_all_lines(__FILE__ __FILE__), sharp::Exception);

    char temp_file_name[] = "/tmp/gnotetestXXXXXX";
    int fd = mkstemp(temp_file_name);
    close(fd);

    lines = sharp::file_read_all_lines(temp_file_name);
    CHECK_EQUAL(0, lines.size());

    FILE *file = fopen(temp_file_name, "w");
    fputs("line1\nline2\nline3", file);
    fclose(file);

    lines = sharp::file_read_all_lines(temp_file_name);
    CHECK_EQUAL(3, lines.size());
    CHECK_EQUAL("line1", lines[0]);
    CHECK_EQUAL("line2", lines[1]);
    CHECK_EQUAL("line3", lines[2]);

    std::remove(temp_file_name);
  }

  TEST(read_all_text)
  {
    Glib::ustring file_content;
    // very unlikely to exist
    CHECK_THROW(sharp::file_read_all_text(__FILE__ __FILE__), sharp::Exception);

    char temp_file_name[] = "/tmp/gnotetestXXXXXX";
    int fd = mkstemp(temp_file_name);
    close(fd);

    file_content = sharp::file_read_all_text(temp_file_name);
    CHECK_EQUAL("", file_content);

    FILE *file = fopen(temp_file_name, "w");
    fputs("line1\nline2\nline3", file);
    fclose(file);

    file_content = sharp::file_read_all_text(temp_file_name);
    CHECK_EQUAL("line1\nline2\nline3", file_content);

    std::remove(temp_file_name);
  }

  TEST(read_all_text_gio)
  {
    Glib::ustring file_content;

    char temp_file_name[] = "/tmp/gnotetestXXXXXX";
    int fd = mkstemp(temp_file_name);
    close(fd);

    auto file = Gio::File::create_for_path(temp_file_name);
    file_content = sharp::file_read_all_text(file);
    CHECK_EQUAL("", file_content);

    FILE *f = fopen(temp_file_name, "w");
    fputs("line1\nline2\nline3", f);
    fclose(f);

    file_content = sharp::file_read_all_text(file);
    CHECK_EQUAL("line1\nline2\nline3", file_content);

    std::remove(temp_file_name);
  }

  TEST(write_all_text)
  {
    Glib::ustring file_content = "line1\nline2\nline3";
    char temp_file_name[] = "/tmp/gnotetestXXXXXX";
    int fd = mkstemp(temp_file_name);
    close(fd);

    sharp::file_write_all_text(temp_file_name, file_content);
    std::ifstream fin(temp_file_name);
    std::string line1, line2, line3;
    std::getline(fin, line1);
    std::getline(fin, line2);
    std::getline(fin, line3);
    fin.close();

    CHECK_EQUAL("line1", line1);
    CHECK_EQUAL("line2", line2);
    CHECK_EQUAL("line3", line3);

    CHECK_THROW(sharp::file_write_all_text("/usr/gnotetest", file_content), sharp::Exception);

    std::remove(temp_file_name);
  }
}

