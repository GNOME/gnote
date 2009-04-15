

#include <iostream>

#include <boost/test/minimal.hpp>

#include "sharp/string.hpp"

using namespace sharp;

int test_main(int /*argc*/, char ** /*argv*/)
{
  std::string test1("foo bar baz");
  std::string test2("   foo   ");
  std::string test3("** foo ++");
  std::string test4("CamelCase");
  std::string test5("\t\tjust\na\tbunch of\n\nrandom\t words\n\n\t");

  BOOST_CHECK(string_replace_first(test1, "ba", "ft") == "foo ftr baz");
  BOOST_CHECK(string_replace_all(test1, "ba", "ft") == "foo ftr ftz");
#if 0
  BOOST_CHECK(string_match_iregex(test4, "^Camel"));
#endif
  BOOST_CHECK(string_replace_regex(test4, "([aem])", "Xx") == "CXxmelCase");
  std::cout << string_replace_regex(test4, "([aem])", "Xx") << std::endl;
  BOOST_CHECK(string_replace_regex(test4, "ame", "Xx") == "CXxlCase");
  std::cout << string_replace_regex(test4, "ame", "Xx") << std::endl;

  std::vector<std::string> splits;
  string_split(splits, test1, " ");
  BOOST_CHECK(splits.size() == 3);
  BOOST_CHECK(splits[0] == "foo");
  BOOST_CHECK(splits[1] == "bar");
  BOOST_CHECK(splits[2] == "baz");

  splits.clear();
  string_split(splits, test5, " \t\n");

  BOOST_CHECK(splits.size() == 13);
  BOOST_CHECK(splits[0] == "");
  BOOST_CHECK(splits[1] == "");
  BOOST_CHECK(splits[2] == "just");
  BOOST_CHECK(splits[3] == "a");
  BOOST_CHECK(splits[4] == "bunch");
  BOOST_CHECK(splits[5] == "of");
  BOOST_CHECK(splits[6] == "");
  BOOST_CHECK(splits[7] == "random");
  BOOST_CHECK(splits[8] == "");
  BOOST_CHECK(splits[9] == "words");
  BOOST_CHECK(splits[10] == "");
  BOOST_CHECK(splits[11] == "");
  BOOST_CHECK(splits[12] == "");

  // C# string.Contains matches true on empty strings
  BOOST_CHECK(string_contains(test5, ""));

  BOOST_CHECK(string_substring(test1, 4) == "bar baz");
  BOOST_CHECK(string_substring(test1, 4, 3) == "bar");

  BOOST_CHECK(string_trim(test2) == "foo");
  BOOST_CHECK(string_trim(test3, "*+") == " foo ");

  BOOST_CHECK(string_starts_with(test1, "foo"));

  BOOST_CHECK(string_ends_with(test1, "baz"));

  BOOST_CHECK(string_contains(test1, "ba"));
  BOOST_CHECK(!string_contains(test1, "CD"));

  BOOST_CHECK(string_index_of(test1, "ba") == 4);
  BOOST_CHECK(string_index_of(test1, "ba", 5) == 8);

  BOOST_CHECK(string_last_index_of(test1, "ba") == 8);
  BOOST_CHECK(string_last_index_of(test1, "Camel") == -1);

  BOOST_CHECK(string_to_lower(test4) == "camelcase");

  return 0;
}
