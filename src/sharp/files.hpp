/*
 * gnote
 *
 * Copyright (C) 2017-2019 Aurimas Cernius
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

#ifndef __SHARP_FILES_HPP_
#define __SHARP_FILES_HPP_

#include <vector>

#include <giomm/file.h>
#include <glibmm/ustring.h>

namespace sharp {

  bool file_exists(const Glib::ustring & p);
  void file_delete(const Glib::ustring & p);
  void file_move(const Glib::ustring & from, const Glib::ustring & to);
  /** return the basename of the file path */
  Glib::ustring file_basename(const Glib::ustring & p);
  /** return the directory from the file path */
  Glib::ustring file_dirname(const Glib::ustring & p);
  /** return the filename from the file path */
  Glib::ustring file_filename(const Glib::ustring & p);
  Glib::ustring file_filename(const Glib::RefPtr<Gio::File> & p);
  void file_copy(const Glib::ustring & source, const Glib::ustring & dest);

  std::vector<Glib::ustring> file_read_all_lines(const Glib::ustring & path);
  Glib::ustring file_read_all_text(const Glib::ustring & path);
  Glib::ustring file_read_all_text(const Glib::RefPtr<Gio::File> & path);
  void file_write_all_text(const Glib::ustring & path, const Glib::ustring & content);
}


#endif
