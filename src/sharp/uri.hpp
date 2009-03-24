


#ifndef __SHARP_URI_HPP_
#define __SHARP_URI_HPP_


#include <string>

namespace sharp {

  class Uri
  {
  public:
    Uri(const std::string & u)
      : m_uri(u)
      {
      }
    const std::string & to_string() const
      { 
        return m_uri; 
      }
    bool is_file() const;
    std::string local_path() const;
    static std::string escape_uri_string(const std::string &);
  private:
    std::string m_uri;
  };

}


#endif

