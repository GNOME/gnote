/*
 * gnote
 *
 * Copyright (C) 2023 Aurimas Cernius
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

using gnote::utils::XmlDecoder;

SUITE(XmlDecoder)
{
  TEST(decode_returns_plain_text)
  {
    const auto note =
      "<note-content xmlns:size=\"http://beatniksoftware.com/tomboy/size\">"
      "<note-title>Test Note</note-title>\n\n\n\n"
      "<size:huge>Tasks</size:huge>\n\n\n"
      "<size:huge>Other</size:huge>\n\n\n"
      "</note-content>";
    const auto plain_text = "Test Note\n\n\n\nTasks\n\n\nOther\n\n\n";

    auto decoded = XmlDecoder::decode(note);
    CHECK_EQUAL(plain_text, decoded);
  }
}

