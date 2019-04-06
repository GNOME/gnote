/*
 * gnote
 *
 * Copyright (C) 2013,2016,2019 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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

#ifndef __READ_ONLY_NOTE_ADDIN_HPP_
#define __READ_ONLY_NOTE_ADDIN_HPP_

#include "base/macros.hpp"
#include "sharp/dynamicmodule.hpp"
#include "noteaddin.hpp"

namespace readonly {

class ReadOnlyModule
  : public sharp::DynamicModule
{
public:
  ReadOnlyModule();
};

class ReadOnlyNoteAddin
  : public gnote::NoteAddin
{
public:
  static ReadOnlyNoteAddin * create()
    {
      return new ReadOnlyNoteAddin;
    }

  virtual ~ReadOnlyNoteAddin();
  virtual void initialize() override;
  virtual void shutdown() override;
  virtual void on_note_opened() override;
  virtual std::vector<gnote::PopoverWidget> get_actions_popover_widgets() const override;
private:
  ReadOnlyNoteAddin();
  void on_menu_item_toggled(const Glib::VariantBase & state);
  void on_foreground();
  void on_background();

  sigc::connection m_readonly_toggle_cid;
};

}

#endif
