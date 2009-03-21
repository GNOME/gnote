
#include <glibmm/ustring.h>

#include "debug.hpp"
#include "xmlwriter.hpp"

namespace sharp {

	XmlWriter::XmlWriter()
	{
		m_buf = xmlBufferCreate();
		m_writer = xmlNewTextWriterMemory(m_buf, 0);
	}

	XmlWriter::XmlWriter(const std::string & filename)
		: m_buf(NULL)
	{
		m_writer = xmlNewTextWriterFilename(filename.c_str(), 0);
	}

	XmlWriter::~XmlWriter()
	{
		xmlFreeTextWriter(m_writer);
		if(m_buf)
			xmlBufferFree(m_buf);
	}


	int XmlWriter::write_char_entity(gunichar ch)
	{
		Glib::ustring unistring(1, (gunichar)ch);
		DBG_OUT("write entity %s", unistring.c_str());
		return xmlTextWriterWriteString(m_writer, (const xmlChar*)unistring.c_str());
	}

	int XmlWriter::write_string(const std::string & s)
	{
		return xmlTextWriterWriteString(m_writer, (const xmlChar*)s.c_str());
	}


	int  XmlWriter::close()
	{
		int rc = xmlTextWriterEndDocument(m_writer);
		xmlTextWriterFlush(m_writer);
		return rc;
	}


	std::string XmlWriter::to_string()
	{
		if(!m_buf) {
			return "";
		}
		std::string output((const char*)m_buf->content);
		return output;
	}

}
