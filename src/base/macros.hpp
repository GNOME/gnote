/*
 * gnote
 *
 * Copyright (C) 2013 Aurimas Cernius
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




#ifndef __BASE_MACROS_
#define __BASE_MACROS_

#if __cplusplus < 201103L
  #include <boost/foreach.hpp>
  #include <boost/lexical_cast.hpp>
#else
  #include <string>
#endif

#if __GNUC__
#define _PRINTF_FORMAT(f,a) \
    __attribute__ ((format (printf, f, a)));
#else
#define _PRINTF_FORMAT(f,a)
#endif

// define 'final' and 'override' for pre-C++11 compilers
#if __cplusplus < 201103L
  #define final
  #define override
  #define FOREACH(var, container) BOOST_FOREACH(var, container)
  #define TO_STRING(x) boost::lexical_cast<std::string>(x)
  #define STRING_TO_INT(x) boost::lexical_cast<int>(x)
#else
  #define FOREACH(var, container) for(var : container)
  #define TO_STRING(x) std::to_string(x)
  #define STRING_TO_INT(x) std::stoi(x)
#endif

#endif
