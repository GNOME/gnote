/*
 * gnote
 *
 * Copyright (C) 2010,2013,2016,2019,2023,2025 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
 * Original C# file
 * (C) 2006 Ryan Lortie <desrt@desrt.ca>
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




// Translated from FixedWidthNoteAddin.cs:

// Add a 'fixed width' item to the font styles menu.


#ifndef __ADDIN_FIXEDWIDTHNOTEADDIN_H_
#define __ADDIN_FIXEDWIDTHNOTEADDIN_H_

#include <gtkmm/texttag.h>

#include "sharp/dynamicmodule.hpp"
#include "noteaddin.hpp"

namespace fixedwidth {

  class FixedWidthModule
    : public sharp::DynamicModule
  {
  public:
    FixedWidthModule();
  };


  DECLARE_MODULE(FixedWidthModule);

  class FixedWidthNoteAddin
    : public gnote::NoteAddin
  {
  public:
    static FixedWidthNoteAddin* create()
      { 
        return new FixedWidthNoteAddin();
      }
    virtual void initialize() override;
    virtual void shutdown() override;
    virtual void on_note_opened() override;
  private:
    void on_note_foregrounded() override;
    void on_note_backgrounded() override;
    void on_menu_item_state_changed(const Glib::VariantBase & state);
    void add_menu_item(gnote::NoteTextMenu & menu);

    Glib::RefPtr<Gtk::TextTag> m_tag;
    sigc::connection           m_menu_item_cid;
  };


}


#endif

