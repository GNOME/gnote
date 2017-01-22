/*
 * gnote
 *
 * Copyright (C) 2014,2017 Aurimas Cernius
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



#ifndef __SHARP_STRING_HPP_
#define __SHARP_STRING_HPP_

#include <vector>

#include <glibmm/ustring.h>

namespace sharp {

  /**
   * replace the first instance of %what with %with
   * in string %source and return the result
   */
  Glib::ustring string_replace_first(const Glib::ustring & source, const Glib::ustring & what,
                                     const Glib::ustring & with);

  /**
   * replace all instances of %what with %with
   * in string %source and return the result
   */
  Glib::ustring string_replace_all(const Glib::ustring & source, const Glib::ustring & what,
                                   const Glib::ustring & with);
  /** 
   * regex replace in %source with matching %regex with %with
   * and return a copy */
  Glib::ustring string_replace_regex(const Glib::ustring & source, const Glib::ustring & regex,
                                     const Glib::ustring & with);
  bool string_match_iregex(const Glib::ustring & source, const Glib::ustring & regex);

  void string_split(std::vector<Glib::ustring> & split, const Glib::ustring & source,
                    const Glib::ustring & delimiters);

  /** copy the substring for %source, starting at %start until the end */
  Glib::ustring string_substring(const Glib::ustring & source, int start);
  /** copy the substring for %source, starting at %start and running for %len */
  Glib::ustring string_substring(const Glib::ustring & source, int start, int len);

  Glib::ustring string_trim(const Glib::ustring & source);
  Glib::ustring string_trim(const Glib::ustring & source, const Glib::ustring & set_of_char);

  int string_last_index_of(const Glib::ustring & source, const Glib::ustring & search);
}



#endif
