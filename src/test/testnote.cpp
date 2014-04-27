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


#include "testnote.hpp"

namespace test {

Note::Note(gnote::NoteData *_data, const Glib::ustring & filepath, gnote::NoteManagerBase & manager_)
  : gnote::NoteBase(_data, filepath, manager_)
  , m_data_synchronizer(_data)
{
}

const gnote::NoteDataBufferSynchronizerBase & Note::data_synchronizer() const
{
  return m_data_synchronizer;
}

gnote::NoteDataBufferSynchronizerBase & Note::data_synchronizer()
{
  return m_data_synchronizer;
}

}

