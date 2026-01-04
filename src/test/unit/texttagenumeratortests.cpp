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


#include <UnitTest++/UnitTest++.h>

#include "utils.hpp"


SUITE(TextTagEnumerator)
{
  struct Fixture
  {
    Glib::RefPtr<Gtk::TextBuffer> buffer;
    Glib::RefPtr<Gtk::TextTag> bold;

    Fixture()
    {
      buffer = Gtk::TextBuffer::create();
      bold = buffer->create_tag("bold");
    }

    void bold_text(const Glib::ustring &text)
    {
      auto start = buffer->get_text().find(text);
      CHECK(Glib::ustring::npos != start);
      auto begin = buffer->get_iter_at_offset(start);
      auto end = buffer->get_iter_at_offset(start + text.size());
      buffer->apply_tag(bold, begin, end);
    }
  };

  struct Fixture3BoldWords
    : Fixture
  {
    Fixture3BoldWords()
    {
      buffer->set_text("=== Hello, World! Have a nice day! ===");
      bold_text("Hello");
      bold_text("World");
      bold_text("day");
    }

    void test_no_mark_leaks(std::function<void()> func)
    {
      std::vector<Glib::RefPtr<Gtk::TextMark>> marks;
      auto connection = buffer->signal_mark_set().connect(
        [&marks](const Gtk::TextIter&, const Glib::RefPtr<Gtk::TextMark> &mark)
        {
          marks.emplace_back(mark);
        });

      func();

      connection.disconnect();
      CHECK(!marks.empty());
      for(const auto &mark : marks) {
        CHECK(mark->get_deleted());
      }
    }
  };

  TEST_FIXTURE(Fixture, empty_buffer_gives_no_tags)
  {
    gnote::utils::TextTagEnumerator enumerator(*buffer, bold);
    CHECK(enumerator.begin() == enumerator.end());
  }

  TEST_FIXTURE(Fixture, non_empty_buffer_without_tags_set_gives_no_tags)
  {
    buffer->set_text("Hello, World!");

    gnote::utils::TextTagEnumerator enumerator(*buffer, bold);
    CHECK(enumerator.begin() == enumerator.end());
  }

  TEST_FIXTURE(Fixture, non_empty_fully_tagged_buffer_returns_tag)
  {
    buffer->set_text("Hello, World!");
    buffer->apply_tag(bold, buffer->begin(), buffer->end());

    gnote::utils::TextTagEnumerator enumerator(*buffer, bold);
    auto begin = enumerator.begin();
    CHECK(begin != enumerator.end());
    CHECK(begin->start() == buffer->begin());
    CHECK(begin->end() == buffer->end());
    CHECK_EQUAL("Hello, World!", begin->text());
  }

  TEST_FIXTURE(Fixture, with_tagged_text_in_the_middle_returns_tag_and_text)
  {
    buffer->set_text("Hello, World!");
    bold_text("World");

    gnote::utils::TextTagEnumerator enumerator(*buffer, bold);
    auto begin = enumerator.begin();
    CHECK(begin != enumerator.end());
    CHECK_EQUAL("World", begin->text());
    ++begin;
    CHECK(begin == enumerator.end());
  }

  TEST_FIXTURE(Fixture3BoldWords, with_multiple_tagged_pieces_returns_those)
  {
    gnote::utils::TextTagEnumerator enumerator(*buffer, bold);
    auto begin = enumerator.begin();
    CHECK(begin != enumerator.end());
    CHECK_EQUAL("Hello", begin->text());
    ++begin;
    CHECK(begin != enumerator.end());
    CHECK_EQUAL("World", begin->text());
    CHECK(begin != enumerator.end());
    ++begin;
    CHECK_EQUAL("day", begin->text());
    auto begin2 = begin++;
    CHECK(begin == enumerator.end());
    CHECK(begin2 == enumerator.end());
  }

  TEST_FIXTURE(Fixture3BoldWords, with_multiple_tagged_pieces_returns_those_ranged_for)
  {
    std::deque<Glib::ustring> words { "Hello", "World", "day" };
    for(auto range : gnote::utils::TextTagEnumerator(*buffer, bold)) {
      CHECK_EQUAL(words.front(), range.text());
      words.pop_front();
    }

    CHECK(words.empty());
  }

  TEST_FIXTURE(Fixture3BoldWords, can_replace_found_ranges_when_iterating)
  {
    std::deque<Glib::ustring> replacements { "X", "YYYYY", "ZZZZZZZZ" };
    for(auto range : gnote::utils::TextTagEnumerator(*buffer, bold)) {
      auto replacement = replacements.front();
      replacements.pop_front();

      buffer->erase(range.start(), range.end());
      buffer->insert_with_tag(range.start(), replacement, bold);
    }

    CHECK_EQUAL("=== X, YYYYY! Have a nice ZZZZZZZZ! ===", buffer->get_text());
  }

  TEST_FIXTURE(Fixture3BoldWords, iteration_does_not_leak_marks)
  {
    test_no_mark_leaks([this] {
      gnote::utils::TextTagEnumerator enumerator(*buffer, bold);
      auto begin = enumerator.begin();
      while(begin != enumerator.end()) {
        ++begin;
      }
    });
  }

  TEST_FIXTURE(Fixture3BoldWords, unfinished_iteration_and_retrieved_range_do_not_leak_marks)
  {
    test_no_mark_leaks([this] {
      gnote::utils::TextTagEnumerator enumerator(*buffer, bold);
      auto begin = enumerator.begin();
      auto range = *begin;
      CHECK_EQUAL("Hello", range.text());
      range.destroy(); // TODO: this should happen auto in destructor
    });
  }
}

