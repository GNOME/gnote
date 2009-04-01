

#include <string>
#include <list>

#include <boost/test/minimal.hpp>

#include <libxml++/parsers/domparser.h>

#include "note.hpp"

int test_main(int /*argc*/, char ** /*argv*/)
{
  xmlpp::DomParser parser;
//  std::string markup = "<tags><tag>system:notebook:ggoiiii</tag><tag>system:template</tag></tags>";
  std::string markup = "<tags xmlns=\"http://beatniksoftware.com/tomboy\"><tag>system:notebook:ggoiiii</tag><tag>system:template</tag></tags>";
  parser.parse_memory(markup);
  BOOST_CHECK(parser);
  if(parser) {
    xmlpp::Document * doc2 = parser.get_document();
    BOOST_CHECK(doc2);
    xmlpp::Element * element = doc2->get_root_node();
    element->remove_attribute("xmlns");
    std::list<std::string> tags = gnote::Note::parse_tags(element);
    BOOST_CHECK(!tags.empty());
  }

  return 0;
}

