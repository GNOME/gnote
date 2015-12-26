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

#include <gtkmm/modelbutton.h>

#include "base/macros.hpp"
#include "noteaddin.hpp"

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
    virtual std::map<int, Gtk::Widget*> get_actions_popover_widgets() const override;

  protected:
    NotebookNoteAddin();

  private:
    void on_note_window_foregrounded();
    void on_note_window_backgrounded();
    void on_new_notebook_menu_item(const Glib::VariantBase&) const;
    void on_move_to_notebook(const Glib::VariantBase &);
    void on_notebooks_changed();
    void get_notebook_menu_items(std::list<Gtk::ModelButton*> &) const;
    void update_menu(Gtk::Grid *) const;

    sigc::connection          m_new_notebook_cid;
    sigc::connection          m_move_to_notebook_cid;
    static Tag::Ptr           s_templateTag;
  };

}
}


#endif
