


#include "sharp/string.hpp"
#include "sharp/uri.hpp"

#define FILE_URI_SCHEME "file://"

namespace sharp {

  bool Uri::is_file() const
  {
    return string_starts_with(m_uri, FILE_URI_SCHEME);
  }


  std::string Uri::local_path() const
  {
    if(!is_file()) {
      return m_uri;
    }
    return string_replace_first(m_uri, FILE_URI_SCHEME, "");
  }


  std::string Uri::escape_uri_string(const std::string &s)
  {
    return string_replace_all(s, " ", "%20");
  }

}

