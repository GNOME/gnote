/*
 * gnote
 *
 * Copyright (C) 2014 Aurimas Cernius
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


#include "notebase.hpp"

namespace test {

class Note
  : public gnote::NoteBase
{
public:
  Note(gnote::NoteData *_data, const Glib::ustring & filepath, gnote::NoteManagerBase & manager);
protected:
  virtual const gnote::NoteDataBufferSynchronizerBase & data_synchronizer() const;
  virtual gnote::NoteDataBufferSynchronizerBase & data_synchronizer();
private:
  gnote::NoteDataBufferSynchronizerBase m_data_synchronizer;
};

}

