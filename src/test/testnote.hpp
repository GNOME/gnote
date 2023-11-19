/*
 * gnote
 *
 * Copyright (C) 2014,2018-2019,2022-2023 Aurimas Cernius
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
  static gnote::NoteBase::Ptr create(std::unique_ptr<gnote::NoteData> _data, Glib::ustring && filepath, gnote::NoteManagerBase & manager)
    {
      return Glib::make_refptr_for_instance(new Note(std::move(_data), std::move(filepath), manager));
    }
  void set_change_type(gnote::ChangeType c);
protected:
  virtual const gnote::NoteDataBufferSynchronizerBase & data_synchronizer() const;
  virtual gnote::NoteDataBufferSynchronizerBase & data_synchronizer();
private:
  Note(std::unique_ptr<gnote::NoteData> _data, Glib::ustring && filepath, gnote::NoteManagerBase & manager);

  gnote::NoteDataBufferSynchronizerBase m_data_synchronizer;
};

}

