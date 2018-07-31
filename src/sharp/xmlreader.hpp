/*
 * gnote
 *
 * Copyright (C) 2016-2018 Aurimas Cernius
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




#ifndef __SHARP_XMLREADER_HPP_
#define __SHARP_XMLREADER_HPP_

#include <glibmm/ustring.h>
#include <libxml/xmlreader.h>

#include "noncopyable.hpp"

namespace sharp {

class XmlReader
    : public gnote::NonCopyable
{
public:
  /** Create a XmlReader to read from a buffer */
  XmlReader();
  explicit XmlReader(xmlDocPtr xml_doc);
  /** Create a XmlReader to read from a file */
  explicit XmlReader(const Glib::ustring & filename);
  ~XmlReader();

  /** load the buffer from the s 
   *  The parser is reset.
   */
  void load_buffer(const Glib::ustring &s);
  

  /** read the next node 
   *  return false if it couldn't be read. (either end or error)
   */
  bool read();

  xmlReaderTypes get_node_type();
  
  Glib::ustring  get_name();
  bool           is_empty_element();
  Glib::ustring  get_attribute(const char *);
  Glib::ustring  get_value();
  Glib::ustring  read_string();
  Glib::ustring  read_inner_xml();
  Glib::ustring  read_outer_xml();
  bool           move_to_next_attribute();
  bool           read_attribute_value();

  void           close();
private:

  void setup_error_handling();
  static void error_handler(void* arg, const char* msg, int severity, void* locator);

  xmlDocPtr        m_doc;
  Glib::ustring    m_buffer;
  xmlTextReaderPtr m_reader;
  // error signaling
  bool             m_error;
};


}


#endif
