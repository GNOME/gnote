/*
 * gnote
 *
 * Copyright (C) 2011-2012,2017,2021,2022 Aurimas Cernius
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


#include <glibmm/miscutils.h>
#include <giomm/file.h>
#include <giomm/fileinfo.h>
#include "sharp/fileinfo.hpp"


namespace sharp {


  FileInfo::FileInfo(const Glib::ustring & s)
    : m_path(s)
  {
  }


  Glib::ustring FileInfo::get_name() const
  {
    return Glib::path_get_basename(m_path.c_str());
  }


  Glib::ustring FileInfo::get_extension() const
  {
    const Glib::ustring name = get_name();

    if ("." == name || ".." == name)
      return "";

    const Glib::ustring::size_type pos = name.find_last_of('.');
    return (Glib::ustring::npos == pos) ? "" : Glib::ustring(name, pos);
  }


  Glib::DateTime file_modification_time(const Glib::ustring &path)
  {
    Glib::RefPtr<Gio::FileInfo> file_info = Gio::File::create_for_path(path)->query_info(
        G_FILE_ATTRIBUTE_TIME_MODIFIED + Glib::ustring(",") + G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC);
    if(file_info)
      return file_info->get_modification_date_time();
    return Glib::DateTime();
  }
}
