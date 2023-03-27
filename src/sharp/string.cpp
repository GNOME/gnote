/*
 * gnote
 *
 * Copyright (C) 2012,2014,2017,2022 Aurimas Cernius
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





#include "sharp/string.hpp"

#include <glibmm/regex.h>

#include "debug.hpp"

namespace sharp {


  Glib::ustring string_replace_first(const Glib::ustring & source, const Glib::ustring & what,
                                     const Glib::ustring & with)
  {
    if(source.empty() || what.empty() || what == with) {
      return source;
    }

    Glib::ustring result;
    auto pos = source.find(what);
    if(pos != Glib::ustring::npos) {
      result += source.substr(0, pos);
      result += with;
      result += source.substr(pos + what.size());
    }
    else {
      result = source;
    }

    return result;
  }

  Glib::ustring string_replace_all(const Glib::ustring & source, const Glib::ustring & what,
                                   const Glib::ustring & with)
  {
    if(source.empty() || what.empty() || what == with) {
      return source;
    }

    Glib::ustring result;
    Glib::ustring::size_type start = 0;
    do {
      auto pos = source.find(what, start);
      if(pos != Glib::ustring::npos) {
        result += source.substr(start, pos - start);
        result += with;
        start = pos + what.size();
      }
      else {
        result += source.substr(start);
        start = source.size();
      }
    }
    while(start < source.size());

    return result;
  }

  Glib::ustring string_replace_regex(const Glib::ustring & source,
                                     const Glib::ustring & regex,
                                     const Glib::ustring & with)
  {
    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(regex);
    return re->replace(source, 0, with, static_cast<Glib::Regex::MatchFlags>(0));
  }
  
  bool string_match_iregex(const Glib::ustring & source, const Glib::ustring & regex)
  {
    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(regex, Glib::Regex::CompileFlags::CASELESS);
    Glib::MatchInfo match_info;
    if(re->match(source, match_info)) {
      return match_info.fetch(0) == source;
    }
    return false;
  }

  void string_split(std::vector<Glib::ustring> & split, const Glib::ustring & source,
                    const Glib::ustring & delimiters)
  {
    Glib::ustring::size_type start = 0;
    while(start < source.size()) {
      auto pos = source.find_first_of(delimiters, start);
      if(pos == start) {
        split.push_back("");
      }
      else if(pos == Glib::ustring::npos) {
        split.push_back(source.substr(start));
        break;
      }
      else {
        split.push_back(source.substr(start, pos - start));
      }
      if(pos == source.size() - 1) {
        // match at the last char in source, meaning empty part in the end
        split.push_back("");
        break;
      }
      start = pos + 1;
    }
  }

  Glib::ustring string_substring(const Glib::ustring & source, int start)
  {
    DBG_ASSERT(start >= 0, "start can't be negative");
    if(source.size() <= (unsigned int)start) {
      return "";
    }
    return Glib::ustring(source, start, Glib::ustring::npos);
  }

  Glib::ustring string_substring(const Glib::ustring & source, int start, int len)
  {
    if(source.size() <= (unsigned int)start) {
      return "";
    }
    return Glib::ustring(source, start, len);
  }

  Glib::ustring string_trim(const Glib::ustring & source)
  {
    if(source.empty()) {
      return source;
    }

    auto start = source.begin();
    while(start != source.end()) {
      if(g_unichar_isspace(*start)) {
        ++start;
      }
      else {
        break;
      }
    }

    if(start == source.end()) {
      return "";
    }

    auto end = source.end();
    --end;
    while(end != start) {
      if(g_unichar_isspace(*end)) {
        --end;
      }
      else {
        ++end;
        break;
      }
    }

    if(start == end) {
      // this happens when there is only one non-white space character
      ++end;
    }

    return Glib::ustring(start, end);
  }  

  Glib::ustring string_trim(const Glib::ustring & source, const Glib::ustring & set_of_char)
  {
    if(source.empty()) {
      return source;
    }

    auto start = source.find_first_not_of(set_of_char);
    auto end = source.find_last_not_of(set_of_char) + 1;
    return source.substr(start, end - start);
  }

  int string_last_index_of(const Glib::ustring & source, const Glib::ustring & search)
  {
    if(search.empty()) {
        // Return last index, unless the source is the empty string, return 0
        return source.empty() ? 0 : source.size() - 1;
    }

    return source.rfind(search);
  }

}
