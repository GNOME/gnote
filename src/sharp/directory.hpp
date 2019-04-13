/*
 * gnote
 *
 * Copyright (C) 2012,2017-2019 Aurimas Cernius
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




#ifndef __SHARP_DIRECTORY_HPP_
#define __SHARP_DIRECTORY_HPP_

#include <vector>

#include <giomm/file.h>

namespace sharp {

  /** 
   * @param dir the directory to list
   * @param ext the extension. If empty, then all files are listed.
   * @retval files the list of files
   */
  std::vector<Glib::ustring> directory_get_files_with_ext(const Glib::ustring & dir, const Glib::ustring & ext);
  std::vector<Glib::RefPtr<Gio::File>> directory_get_files_with_ext(const Glib::RefPtr<Gio::File> & dir,
                                    const Glib::ustring & ext);

  std::vector<Glib::ustring> directory_get_directories(const Glib::ustring & dir);
  std::vector<Glib::RefPtr<Gio::File>> directory_get_directories(const Glib::RefPtr<Gio::File> & dir);

  std::vector<Glib::ustring> directory_get_files(const Glib::ustring & dir);
  std::vector<Glib::RefPtr<Gio::File>> directory_get_files(const Glib::RefPtr<Gio::File> & dir);

  bool directory_exists(const Glib::ustring & dir);
  bool directory_exists(const Glib::RefPtr<Gio::File> & dir);

  /**
   * @param src The source directory (or file)
   * @param dest The destination directory (should exist)
   */
  void directory_copy(const Glib::RefPtr<Gio::File> & src,
                      const Glib::RefPtr<Gio::File> & dest);

  bool directory_create(const Glib::ustring & dir);
  bool directory_create(const Glib::RefPtr<Gio::File> & dir);

  bool directory_delete(const Glib::ustring & dir, bool recursive);
  bool directory_delete(const Glib::RefPtr<Gio::File> & dir, bool recursive);
}


#endif
