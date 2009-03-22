

#ifndef __SHARP_STRING_HPP_
#define __SHARP_STRING_HPP_

#include <string>
#include <vector>


namespace sharp {

	/**
	 * replace the first instance of %from with %with 
	 * in string %source and return the result
	 */
	std::string string_replace_first(const std::string & source, const std::string & from,
														 const std::string & with);

	/**
	 * replace all instances of %from with %with 
	 * in string %source and return the result
	 */
	std::string string_replace_all(const std::string & source, const std::string & from,
																 const std::string & with);
	/** 
	 * regex replace in %source with matching %regex with %with
	 * and return a copy */
	std::string string_replace_regex(const std::string & source, const std::string & regex,
																	 const std::string & with);

	void string_split(std::vector<std::string> & split, const std::string & source,
										const char * delimiters);

	std::string string_trim(const std::string & source);
	std::string string_trim(const std::string & source, const char * set_of_char);

	bool string_starts_with(const std::string & source, const std::string & with);

	std::string string_to_lower(const std::string & source);
}



#endif
