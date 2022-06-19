/*
 * gnote
 *
 * Copyright (C) 2017,2022 Aurimas Cernius
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




#ifndef __SHARP_URI_HPP_
#define __SHARP_URI_HPP_


#include <glibmm/ustring.h>

namespace sharp {

  class Uri
  {
  public:
    Uri(Glib::ustring && u)
      : m_uri(std::move(u))
      {
      }
    const Glib::ustring & to_string() const
      { 
        return m_uri; 
      }
    bool is_file() const;
    Glib::ustring local_path() const;
    Glib::ustring get_host() const;
    Glib::ustring get_absolute_uri() const;
    static Glib::ustring escape_uri_string(const Glib::ustring &);
  private:
    bool _is_scheme(const Glib::ustring & scheme) const;

    Glib::ustring m_uri;
  };

}


#endif

