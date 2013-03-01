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




#ifndef __SHARP_XMLWRITER_HPP_
#define __SHARP_XMLWRITER_HPP_

#include <string>

#include <boost/noncopyable.hpp>

#include <glib.h>
#include <libxml/xmlwriter.h>


namespace sharp {


  inline
  const xmlChar * to_xmlchar(const std::string & s)
  {
    return s.empty() ? NULL : (const xmlChar *)s.c_str();
  }

  class XmlWriter
    : public boost::noncopyable
  {
  public:
    XmlWriter();
    XmlWriter(const std::string & filename);
    XmlWriter(xmlDocPtr doc);
    ~XmlWriter();
    int write_start_document();
    int write_end_document();
    int write_start_element(const std::string & prefix, const std::string & name, const std::string & nsuri);
    int write_full_end_element();
    int write_end_element();
    int write_start_attribute(const std::string & name);
    int write_attribute_string(const std::string & prefix,const std::string & local_name,
                               const std::string & ns ,const std::string & value);
    int write_end_attribute();
    int write_raw(const std::string & raw);
    int write_char_entity(gunichar ch);
    int write_string(const std::string & );

    int close();
    std::string to_string();

  private:
    xmlTextWriterPtr m_writer;
    xmlBufferPtr     m_buf;
  };

}

#endif

