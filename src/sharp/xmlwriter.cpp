/*
 * gnote
 *
 * Copyright (C) 2013,2017 Aurimas Cernius
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


#include <glibmm/i18n.h>
#include <glibmm/ustring.h>

#include "debug.hpp"
#include "exception.hpp"
#include "xmlwriter.hpp"


namespace {

  Glib::ustring make_write_failure_msg(const Glib::ustring & caller, const Glib::ustring & fail_func)
  {
    Glib::ustring msg = caller + ": ";
    msg += Glib::ustring::compose(_("%1 failed"), fail_func);
    return msg;
  }

}


namespace sharp {

  XmlWriter::XmlWriter()
  {
    m_buf = xmlBufferCreate();
    m_writer = xmlNewTextWriterMemory(m_buf, 0);
  }

  XmlWriter::XmlWriter(const Glib::ustring & filename)
    : m_buf(NULL)
  {
    m_writer = xmlNewTextWriterFilename(filename.c_str(), 0);
  }

  
  XmlWriter::XmlWriter(xmlDocPtr doc)
    : m_buf(NULL)
  {
    m_writer = xmlNewTextWriterTree(doc, NULL, 0);
  }


  XmlWriter::~XmlWriter()
  {
    xmlFreeTextWriter(m_writer);
    if(m_buf)
      xmlBufferFree(m_buf);
  }


  int XmlWriter::write_start_document()
  {
    int res = xmlTextWriterStartDocument(m_writer, NULL, NULL, NULL);
    if(res < 0) {
      throw Exception(make_write_failure_msg(__FUNCTION__, "xmlTextWriterStartDocument"));
    }

    return res;
  }


  int XmlWriter::write_end_document()
  {
    int res = xmlTextWriterEndDocument(m_writer);
    if(res < 0) {
      throw Exception(make_write_failure_msg(__FUNCTION__, "xmlTextWriterEndDocument"));
    }

    return res;
  }


  int XmlWriter::write_start_element(const Glib::ustring & prefix,
                                     const Glib::ustring & name, const Glib::ustring & nsuri)
  {
    int res = xmlTextWriterStartElementNS(m_writer, to_xmlchar(prefix), 
                                          (const xmlChar*)name.c_str(), to_xmlchar(nsuri));
    if(res < 0) {
      throw Exception(make_write_failure_msg(__FUNCTION__, "xmlTextWriterStartElementNS"));
    }

    return res;
  }


  int XmlWriter::write_full_end_element()
  {
    // ???? what is the difference with write_end_element()
    int res = xmlTextWriterEndElement(m_writer);
    if(res < 0) {
      throw Exception(make_write_failure_msg(__FUNCTION__, "xmlTextWriterEndElement"));
    }

    return res;
  }


  int XmlWriter::write_end_element()
  {
    int res = xmlTextWriterEndElement(m_writer);
    if(res < 0) {
      throw Exception(make_write_failure_msg(__FUNCTION__, "xmlTextWriterEndElement"));
    }

    return res;
  }


  int XmlWriter::write_start_attribute(const Glib::ustring & name)
  {
    int res = xmlTextWriterStartAttribute(m_writer, (const xmlChar*)name.c_str());
    if(res < 0) {
      throw Exception(make_write_failure_msg(__FUNCTION__, "xmlTextWriterStartAttribute"));
    }

    return res;
  }


  int XmlWriter::write_attribute_string(const Glib::ustring & prefix,const Glib::ustring & local_name,
                                        const Glib::ustring & ns ,const Glib::ustring & value)
  {
    int res = xmlTextWriterWriteAttributeNS(m_writer, to_xmlchar(prefix), 
                                            (const xmlChar*)local_name.c_str(),
                                            to_xmlchar(ns), (const xmlChar*)value.c_str());
    if(res < 0) {
      throw Exception(make_write_failure_msg(__FUNCTION__, "xmlTextWriterWriteAttributeNS"));
    }

    return res;
  }


  int XmlWriter::write_end_attribute()
  {
    int res = xmlTextWriterEndAttribute(m_writer);
    if(res < 0) {
      throw Exception(make_write_failure_msg(__FUNCTION__, "xmlTextWriterEndAttribute"));
    }

    return res;
  }


  int XmlWriter::write_raw(const Glib::ustring & raw)
  {
    int res = xmlTextWriterWriteRaw(m_writer, (const xmlChar*)raw.c_str());
    if(res < 0) {
      throw Exception(make_write_failure_msg(__FUNCTION__, "xmlTextWriterWriteRaw"));
    }

    return res;
  }


  int XmlWriter::write_char_entity(gunichar ch)
  {
    Glib::ustring unistring(1, (gunichar)ch);
    DBG_OUT("write entity %s", unistring.c_str());
    return xmlTextWriterWriteString(m_writer, (const xmlChar*)unistring.c_str());
  }

  int XmlWriter::write_string(const Glib::ustring & s)
  {
    return xmlTextWriterWriteString(m_writer, (const xmlChar*)s.c_str());
  }


  int  XmlWriter::close()
  {
    int rc = xmlTextWriterEndDocument(m_writer);
    xmlTextWriterFlush(m_writer);
    return rc;
  }


  Glib::ustring XmlWriter::to_string()
  {
    if(!m_buf) {
      return "";
    }
    Glib::ustring output((const char*)m_buf->content);
    return output;
  }

}
