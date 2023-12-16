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



#ifndef _NOTEBOOKNAMEPOPOVER_HPP_
#define _NOTEBOOKNAMEPOPOVER_HPP_

#include <gtkmm/entry.h>
#include <gtkmm/popover.h>

#include "notebook.hpp"
#include "notebookmanager.hpp"

namespace gnote {
namespace notebooks {

class NotebookNamePopover
  : public Gtk::Popover
{
public:
  static NotebookNamePopover& create(Gtk::Widget& parent, NotebookManager& manager);
  static NotebookNamePopover& create(Gtk::Widget & parent, Notebook & notebook, sigc::slot<void(const Notebook&, const Glib::ustring&)> renamed);
private:
  NotebookNamePopover(Gtk::Widget& parent, NotebookManager& manager);
  NotebookNamePopover(Gtk::Widget & parent, Notebook & notebook, sigc::slot<void(const Notebook&, const Glib::ustring&)> renamed);
  void init(Gtk::Widget& parent, sigc::slot<void()> on_confirm);
  void on_create();
  void on_rename();

  Gtk::Entry *m_name;
  NotebookManager & m_nb_manager;
  Glib::ustring m_notebook;
  sigc::slot<void(const Notebook&, const Glib::ustring&)> m_renamed;
};

}
}

#endif

