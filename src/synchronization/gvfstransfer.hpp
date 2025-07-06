/*
 * gnote
 *
 * Copyright (C) 2025 Aurimas Cernius
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

#ifndef _SYNCHRONIZATION_GVFSTRANSFER_HPP_
#define _SYNCHRONIZATION_GVFSTRANSFER_HPP_

#include <giomm/file.h>

namespace gnote {
namespace sync {

enum class TransferResult
{
  NOT_STARTED,
  SUCCESS,
  FAILURE,
};

struct NoteTransfer
{
  NoteTransfer(const Glib::RefPtr<Gio::File> &src, const Glib::RefPtr<Gio::File> &dest)
    : source(src)
    , destination(dest)
    , result(TransferResult::NOT_STARTED)
  {}

  Glib::RefPtr<Gio::File> source;
  Glib::RefPtr<Gio::File> destination;
  TransferResult result;
};

}
}

#endif

