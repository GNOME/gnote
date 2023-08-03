/*
 * gnote
 *
 * Copyright (C) 2011,2017-2019,2022-2023 Aurimas Cernius
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

#include <fstream>

#include <glib/gstdio.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <giomm/file.h>

#include "exception.hpp"
#include "files.hpp"


namespace sharp {


  bool file_exists(const Glib::ustring & file)
  {
    return Glib::file_test(file, Glib::FileTest::EXISTS)
           && Glib::file_test(file, Glib::FileTest::IS_REGULAR);
  }


  Glib::ustring file_basename(const Glib::ustring & p)
  {
    const Glib::ustring filename = Glib::path_get_basename(p.c_str());
    const Glib::ustring::size_type pos = filename.find_last_of('.');

    return Glib::ustring(filename, 0, pos);
  }

  Glib::ustring file_dirname(const Glib::ustring & p)
  {
    return Glib::path_get_dirname(p.c_str());
  }


  Glib::ustring file_filename(const Glib::ustring & p)
  {
    return Glib::path_get_basename(p.c_str());
  }

  Glib::ustring file_filename(const Glib::RefPtr<Gio::File> & p)
  {
    if(!p)
      return "";
    return p->get_basename();
  }

  void file_delete(const Glib::ustring & p)
  {
    g_unlink(p.c_str());
  }


  void file_copy(const Glib::ustring & source, const Glib::ustring & dest)
  {
    Gio::File::create_for_path(source)->copy(Gio::File::create_for_path(dest), Gio::File::CopyFlags::OVERWRITE);
  }

  void file_move(const Glib::ustring & from, const Glib::ustring & to)
  {
    g_rename(from.c_str(), to.c_str());
  }


  std::vector<Glib::ustring> file_read_all_lines(const Glib::ustring & path)
  {
    std::vector<Glib::ustring> lines;
    std::ifstream fin;
    fin.open(path.c_str());
    if(fin.is_open()) {
      std::string line;
      while(std::getline(fin, line)) {
        lines.push_back(line);
      }
      if(!fin.eof()) {
        throw sharp::Exception("Failure reading file");
      }
      fin.close();
    }
    else {
      throw sharp::Exception("Failed to open file: " + path);
    }

    return lines;
  }

  Glib::ustring file_read_all_text(const Glib::ustring & path)
  {
    auto lines = file_read_all_lines(path);
    if(lines.size() == 0) {
      return "";
    }

    Glib::ustring text = lines[0];
    for(unsigned i = 1; i < lines.size(); ++i) {
      text += "\n" + lines[i];
    }

    return text;
  }

  Glib::ustring file_read_all_text(const Glib::RefPtr<Gio::File> & path)
  {
    Glib::ustring ret;
    char *contents = nullptr;
    gsize size = 0;
    if(path->load_contents(contents, size)) {
      if(contents) {
        ret = contents;
        g_free(contents);
      }
    }

    return ret;
  }

  void file_write_all_text(const Glib::ustring & path, const Glib::ustring & content)
  {
    std::ofstream fout(path);
    if(!fout.is_open()) {
      throw sharp::Exception("Failed to open file: " + path);
    }
    fout << content;
    if(!fout.good()) {
      throw sharp::Exception("Failed to write to file");
    }
    fout.close();
  }
}

