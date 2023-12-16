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


#include "notemanager.hpp"
#include "notebooknamepopover.hpp"
#include "popoverwidgets.hpp"

namespace gnote {
namespace notebooks {

NotebookNamePopover& NotebookNamePopover::create(Gtk::Widget& parent, NotebookManager& manager)
{
  auto popover = manage(new NotebookNamePopover(parent, manager));
  utils::unparent_popover_on_close(popover);
  return *popover;
}

NotebookNamePopover& NotebookNamePopover::create(Gtk::Widget & parent, Notebook & notebook, sigc::slot<void(const Notebook&, const Glib::ustring&)> renamed)
{
  auto popover = manage(new NotebookNamePopover(parent, notebook, renamed));
  utils::unparent_popover_on_close(popover);
  return *popover;
}

NotebookNamePopover::NotebookNamePopover(Gtk::Widget& parent, NotebookManager& manager)
  : m_nb_manager(manager)
{
  init(parent, sigc::mem_fun(*this, &NotebookNamePopover::on_create));
}

NotebookNamePopover::NotebookNamePopover(Gtk::Widget & parent, Notebook & notebook, sigc::slot<void(const Notebook&, const Glib::ustring&)> renamed)
  : m_nb_manager(notebook.note_manager().notebook_manager())
  , m_notebook(notebook.get_normalized_name())
  , m_renamed(renamed)
{
  init(parent, sigc::mem_fun(*this, &NotebookNamePopover::on_rename));
  m_name->set_text(notebook.get_name());
}

void NotebookNamePopover::init(Gtk::Widget& parent, sigc::slot<void()> on_confirm)
{
  set_parent(parent);
  set_position(Gtk::PositionType::BOTTOM);

  auto box = Gtk::make_managed<Gtk::Box>();
  box->set_spacing(5);
  m_name = Gtk::make_managed<Gtk::Entry>();
  m_name->set_activates_default(true);
  auto confirm = Gtk::make_managed<Gtk::Button>();
  confirm->set_icon_name("object-select-symbolic");
  confirm->signal_clicked().connect(on_confirm);
  box->append(*m_name);
  box->append(*confirm);
  set_child(*box);
  set_default_widget(*confirm);
}

void NotebookNamePopover::on_create()
{
  const auto new_name = m_name->get_text();
  if(new_name.empty() || m_nb_manager.notebook_exists(new_name)) {
    m_name->grab_focus();
    return;
  }

  m_nb_manager.get_or_create_notebook(new_name);
  popdown();
}

void NotebookNamePopover::on_rename()
{
  const auto new_name = m_name->get_text();
  if(new_name.empty() || m_nb_manager.notebook_exists(new_name)) {
    m_name->grab_focus();
    return;
  }
  auto nb = m_nb_manager.get_notebook(m_notebook);
  if(!nb) {
    popdown();
    return;
  }
  Notebook& notebook = nb.value();

  if(new_name != notebook.get_name()) {
    m_renamed(notebook, new_name);
  }

  popdown();
}

}
}

