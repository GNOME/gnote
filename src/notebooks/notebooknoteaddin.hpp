/*
 * gnote
 *
 * Copyright (C) 2011-2015 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
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




#ifndef __NOTEBOOKS_NOTEBOOK_NOTE_ADDIN_HPP__
#define __NOTEBOOKS_NOTEBOOK_NOTE_ADDIN_HPP__

#include <gtkmm/menu.h>
#include <gtkmm/menutoolbutton.h>

#include "base/macros.hpp"
#include "noteaddin.hpp"
#include "notebooks/notebookmenuitem.hpp"

namespace gnote {
namespace notebooks {

  class NotebookNoteAddin
    : public NoteAddin
  {
  public:
    static NoteAddin * create();
    static Tag::Ptr get_template_tag();
    virtual void initialize() override;
    virtual void shutdown() override;
    virtual void on_note_opened() override;

  protected:
    NotebookNoteAddin();

  private:
    void on_new_notebook_menu_item();
    void get_notebook_menu_items(std::list<NotebookMenuItem*> &);
    void update_menu(Gtk::Menu *);

    static Tag::Ptr           s_templateTag;
  };

}
}


#endif
