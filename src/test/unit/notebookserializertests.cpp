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
  using gnote::notebooks::NotebookData;

  class Notebook
    : public INotebook
  {
  public:
    explicit Notebook(const Glib::ustring &name, const Glib::DateTime &created)
      : m_name(name)
      , m_created(created)
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
    Glib::DateTime created() const override
      {
        return m_created;
      }
    void created(const Glib::DateTime &new_date)
      {
        m_created = new_date;
      }
  private:
    Glib::ustring m_name;
    Glib::DateTime m_created;
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
    auto created = Glib::DateTime::create_utc(2025, 1, 2, 5, 6, 32.5);
    Notebook nb("Test", created);
    std::vector<INotebook::Ref> notebooks;
    notebooks.emplace_back(nb);
    auto str = NotebookSerializer::serialize(notebooks);

    const char *xml = "<?xml version=\"1.0\"?>"
                      "<notebooks>"
                      "  <notebook id=\"test\" name=\"Test\" created=\"2025-01-02T05:06:32.500000Z\"/>"
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
                      "  <notebook id=\"test\" name=\"Test\" created=\"2025-01-02T05:06:32.500000Z\"/>"
                      "  <notebook id=\"testnb1\" name=\"Testnb1\" created=\"2025-02-02T05:06:32.500000Z\"/>"
                      "  <notebook id=\"testnb2\" name=\"TestNB2\" created=\"2025-02-02T05:06:32.500000Z\"/>"
                      "</notebooks>";

    auto date1 = Glib::DateTime::create_utc(2025, 1, 2, 5, 6, 32.5);
    auto date2 = Glib::DateTime::create_utc(2025, 2, 2, 5, 6, 32.5);

    auto result = NotebookSerializer::deserialize(xml);
    REQUIRE CHECK_EQUAL(3, result.size());
    CHECK_EQUAL("test", result[0].get_normalized_name());
    CHECK_EQUAL("Test", result[0].get_name());
    CHECK(date1.equal(result[0].created()));
    CHECK_EQUAL("testnb1", result[1].get_normalized_name());
    CHECK_EQUAL("Testnb1", result[1].get_name());
    CHECK(date2.equal(result[1].created()));
    CHECK_EQUAL("testnb2", result[2].get_normalized_name());
    CHECK_EQUAL("TestNB2", result[2].get_name());
    CHECK(date2.equal(result[2].created()));
  }

  void check_notebook_update(const std::vector<NotebookData> &updates, const INotebook &nb)
  {
    for(const auto &upd : updates) {
      if(upd.get_normalized_name() == nb.get_normalized_name()) {
        CHECK(upd.created().equal(nb.created()));
        CHECK_EQUAL(nb.get_name(), upd.get_name());
        return;
      }
    }

    CHECK(false);
  }

  TEST(merge)
  {
    auto date1 = Glib::DateTime::create_utc(2025, 1, 2, 5, 6, 32.5);
    auto date2 = Glib::DateTime::create_utc(2025, 2, 2, 5, 6, 32.5);
    auto date3 = Glib::DateTime::create_utc(2025, 3, 3, 5, 6, 32.0);
    Notebook same("Same", date1);
    Notebook new_date("New date", date1);
    Notebook removed("Removed", date1);
    std::vector<INotebook::Ref> current { same, new_date, removed };
    std::vector<NotebookData> loaded;
    loaded.emplace_back("same", date1);
    loaded.back().set_name("Same");
    loaded.emplace_back("new notebook", date2);
    loaded.back().set_name("New Notebook");
    loaded.emplace_back("new date", date2);
    loaded.back().set_name("New date");

    auto updates = NotebookSerializer::merge(loaded, current, date3);
    REQUIRE CHECK_EQUAL(3, loaded.size());
    check_notebook_update(loaded, same);
    new_date.created(date2);
    check_notebook_update(loaded, new_date);
    CHECK_EQUAL(2, updates.size());
    check_notebook_update(updates, loaded[1]);
    check_notebook_update(updates, loaded[2]);
  }
}

