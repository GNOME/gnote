



#include "sharp/string.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>


namespace sharp {


	std::string string_replace_first(const std::string & source, const std::string & from,
														 const std::string & with)
	{
		return boost::replace_first_copy(source, from, with);
	}

	std::string string_replace_all(const std::string & source, const std::string & from,
																 const std::string & with)
	{
		return boost::replace_all_copy(source, from, with);
	}

	std::string string_replace_regex(const std::string & source, const std::string & regex,
																	 const std::string & with)
	{
		return boost::replace_regex_copy(source, boost::regex(regex), with);
	}
  
  bool string_match_iregex(const std::string & source, const std::string & regex)  
  {
    return boost::regex_match(boost::to_lower_copy(source), 
                              boost::regex(regex,  
                                           boost::regex_constants::icase));
  }

	void string_split(std::vector<std::string> & split, const std::string & source,
										const char * delimiters)
	{
		boost::split(split, source, boost::is_any_of(delimiters));
	}

  std::string string_substring(const std::string & source, int start)
  {
    return std::string(source.begin() + start, source.end());
  }

  std::string string_substring(const std::string & source, int start, int len)
  {
    return std::string(source.begin() + start, source.begin() + start + len);
  }

	std::string string_trim(const std::string & source)
	{
		return boost::trim_copy(source);
	}	

	std::string string_trim(const std::string & source, const char * set_of_char)
	{
		return boost::trim_copy_if(source, boost::is_any_of(set_of_char));
	}

	bool string_starts_with(const std::string & source, const std::string & with)
	{
		return boost::starts_with(source, with);
	}

	bool string_ends_with(const std::string & source, const std::string & with)
	{
		return boost::ends_with(source, with);
	}

  int string_last_index_of(const std::string & source, const std::string & search)
  {
    boost::iterator_range<std::string::const_iterator> iter
      = boost::find_last(source, search);
    if(iter.begin() == source.end()) {
      // NOT FOUND
      return -1;
    }
    return iter.begin() - source.begin();
  }


  int string_index_of(const std::string & source, const std::string & search)
  {
    boost::iterator_range<std::string::const_iterator> iter
      = boost::find_first(source, search);
    if(iter.begin() == source.end()) {
      // NOT FOUND
      return -1;
    }
    return iter.begin() - source.begin();
  }


	std::string string_to_lower(const std::string & source)
	{
		return boost::to_lower_copy(source);
	}

}
