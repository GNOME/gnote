/*
 * gnote
 *
 * Copyright (C) 2026 Aurimas Cernius
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <algorithm>

#include <UnitTest++/UnitTest++.h>

#include "notebooks/notebookserializer.hpp"


SUITE(NotebookSerializer)
{
  using gnote::notebooks::INotebook;
  using gnote::notebooks::NotebookSerializer;

  class Notebook
    : public INotebook
  {
  public:
    explicit Notebook(const Glib::ustring &name)
      : m_name(name)
    {}

    Glib::ustring get_name() const override
      {
        return m_name;
      }
    void set_name(const Glib::ustring &name) override
      {
        m_name = name;
      }
    Glib::ustring get_normalized_name() const override
      {
        return m_name.lowercase();
      }
  private:
    Glib::ustring m_name;
  };

  void erase_white_space(Glib::ustring &str)
  {
    std::string s = str;
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); }), s.end());
    str = s;
  }

  TEST(serialize_empty)
  {
    auto str = NotebookSerializer::serialize({});
    CHECK_EQUAL("", str);
  }

  TEST(serialize_non_empty)
  {
    Notebook nb("Test");
    std::vector<INotebook::Ref> notebooks;
    notebooks.emplace_back(nb);
    auto str = NotebookSerializer::serialize(notebooks);

    const char *xml = "<?xml version=\"1.0\"?>"
                      "<notebooks>"
                      "  <notebook id=\"test\" name=\"Test\"/>"
                      "</notebooks>";

    Glib::ustring reference = xml;
    erase_white_space(reference);
    erase_white_space(str);

    CHECK_EQUAL(reference, str);
  }

  TEST(deserialize)
  {
    const char *xml = "<?xml version=\"1.0\"?>"
                      "<notebooks>"
                      "  <notebook id=\"test\" name=\"Test\"/>"
                      "  <notebook id=\"testnb1\" name=\"Testnb1\"/>"
                      "  <notebook id=\"testnb2\" name=\"TestNB2\"/>"
                      "</notebooks>";

    auto result = NotebookSerializer::deserialize(xml);
    REQUIRE CHECK_EQUAL(3, result.size());
    CHECK_EQUAL("test", result[0].get_normalized_name());
    CHECK_EQUAL("Test", result[0].get_name());
    CHECK_EQUAL("testnb1", result[1].get_normalized_name());
    CHECK_EQUAL("Testnb1", result[1].get_name());
    CHECK_EQUAL("testnb2", result[2].get_normalized_name());
    CHECK_EQUAL("TestNB2", result[2].get_name());
  }
}

