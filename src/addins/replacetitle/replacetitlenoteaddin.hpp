/*
 * gnote
 *
 * Copyright (C) 2011-2013,2016,2019 Aurimas Cernius
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


#ifndef _REPLACETITLE_ADDIN_HPP_
#define _REPLACETITLE_ADDIN_HPP_

#include "sharp/dynamicmodule.hpp"
#include "note.hpp"
#include "noteaddin.hpp"

namespace replacetitle {

class ReplaceTitleModule
  : public sharp::DynamicModule
{
public:
  ReplaceTitleModule();
};
  
DECLARE_MODULE(ReplaceTitleModule);

class ReplaceTitleNoteAddin
  : public gnote::NoteAddin
{
public:
  static ReplaceTitleNoteAddin * create()
  {
    return new ReplaceTitleNoteAddin;
  }
  virtual void initialize() override;
  virtual void shutdown() override;
  virtual void on_note_opened() override;
  virtual std::vector<gnote::PopoverWidget> get_actions_popover_widgets() const override;
private:
  void replacetitle_button_clicked(const Glib::VariantBase&);
};


}

#endif
