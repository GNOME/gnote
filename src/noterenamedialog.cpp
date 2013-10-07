/*
 * gnote
 *
 * Copyright (C) 2011-2013 Aurimas Cernius
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <functional>
#include <utility>

#include <glibmm/i18n.h>
#include <gtkmm/expander.h>

#include "mainwindow.hpp"
#include "notewindow.hpp"
#include "noterenamedialog.hpp"

namespace gnote {

ModelColumnRecord::ModelColumnRecord()
  : Gtk::TreeModelColumnRecord()
  , m_column_selected()
  , m_column_title()
  , m_column_note()
{
    add(m_column_selected);
    add(m_column_title);
    add(m_column_note);
}

ModelColumnRecord::~ModelColumnRecord()
{
}

const Gtk::TreeModelColumn<bool> & ModelColumnRecord::get_column_selected()
                                     const
{
    return m_column_selected;
}

gint ModelColumnRecord::get_column_selected_num() const
{
    return COLUMN_BOOL;
}

const Gtk::TreeModelColumn<std::string> & ModelColumnRecord::get_column_title()
                                            const
{
    return m_column_title;
}

gint ModelColumnRecord::get_column_title_num() const
{
    return COLUMN_TITLE;
}

const Gtk::TreeModelColumn<Note::Ptr> & ModelColumnRecord::get_column_note()
                                          const
{
    return m_column_note;
}

gint ModelColumnRecord::get_column_note_num() const
{
    return COLUMN_NOTE;
}

class ModelFiller
  : public std::unary_function<const Note::Ptr &, void>
{
public:

  ModelFiller(const Glib::RefPtr<Gtk::ListStore> & list_store);
  void operator()(const Note::Ptr & note);

private:

  Glib::RefPtr<Gtk::ListStore> m_list_store;
};

ModelFiller::ModelFiller(
               const Glib::RefPtr<Gtk::ListStore> & list_store)
  : std::unary_function<const Note::Ptr &, void>()
  , m_list_store(list_store)
{
}

void ModelFiller::operator()(const Note::Ptr & note)
{
  if (!note)
    return;

  ModelColumnRecord model_column_record;
  const Gtk::TreeModel::iterator iter = m_list_store->append();
  Gtk::TreeModel::Row row = *iter;

  row[model_column_record.get_column_selected()] = true;
  row[model_column_record.get_column_title()] = note->get_title();
  row[model_column_record.get_column_note()] = note;
}

NoteRenameDialog::NoteRenameDialog(const Note::List & notes,
                                   const std::string & old_title,
                                   const Note::Ptr & renamed_note)
  : Gtk::Dialog(_("Rename Note Links?"),
                *dynamic_cast<Gtk::Window*>(renamed_note->get_window()->host()),
                false)
  , m_notes_model(Gtk::ListStore::create(m_model_column_record))
  , m_dont_rename_button(_("_Don't Rename Links"), true)
  , m_rename_button(_("_Rename Links"), true)
  , m_select_all_button(_("Select All"))
  , m_select_none_button(_("Select None"))
  , m_always_show_dlg_radio(_("Always show this _window"), true)
  , m_always_rename_radio(_("Alwa_ys rename links"),
                          true)
  , m_never_rename_radio(_("Never rename _links"),
                         true)
{
  set_default_response(Gtk::RESPONSE_CANCEL);
  set_border_width(10);

  Gtk::Box * const vbox = get_vbox();

  add_action_widget(m_rename_button, Gtk::RESPONSE_YES);
  add_action_widget(m_dont_rename_button, Gtk::RESPONSE_NO);

  std::for_each(notes.begin(), notes.end(),
                ModelFiller(m_notes_model));

  Gtk::Label * const label = Gtk::manage(new Gtk::Label());
  label->set_use_markup(true);
  label->set_markup(
    Glib::ustring::compose(
      _("Rename links in other notes from "
        "\"<span underline=\"single\">%1</span>\" to "
        "\"<span underline=\"single\">%2</span>\"?\n\n"
        "If you do not rename the links, they will no longer link to "
        "anything."),
      old_title,
      renamed_note->get_title()));
  label->set_line_wrap(true);
  vbox->pack_start(*label, false, true, 5);

  Gtk::TreeView * const notes_view = Gtk::manage(
                                       new Gtk::TreeView(
                                             m_notes_model));
  notes_view->set_size_request(-1, 200);
  notes_view->signal_row_activated().connect(
    sigc::bind(
      sigc::mem_fun(*this,
                    &NoteRenameDialog::on_notes_view_row_activated),
      old_title));

  ModelColumnRecord model_column_record;

  Gtk::CellRendererToggle * const toggle_cell
                                    = Gtk::manage(
                                        new Gtk::CellRendererToggle);
  toggle_cell->set_activatable(true);
  toggle_cell->signal_toggled().connect(
    sigc::mem_fun(*this,
                  &NoteRenameDialog::on_toggle_cell_toggled));

  {
    Gtk::TreeViewColumn * const column = Gtk::manage(
                                           new Gtk::TreeViewColumn(
                                                 _("Rename Links"),
                                                 *toggle_cell));
    column->add_attribute(*toggle_cell,
                          "active",
                          model_column_record.get_column_selected());
    column->set_sort_column(
              model_column_record.get_column_selected());
    column->set_resizable(true);
    notes_view->append_column(*column);
  }

  {
    Gtk::TreeViewColumn * const column
      = Gtk::manage(
          new Gtk::TreeViewColumn(
                _("Note Title"),
                model_column_record.get_column_title()));
    column->set_sort_column(model_column_record.get_column_title());
    column->set_resizable(true);
    notes_view->append_column(*column);
  }

  m_select_all_button.signal_clicked().connect(
    sigc::bind(
      sigc::mem_fun(*this,
                    &NoteRenameDialog::on_select_all_button_clicked),
      true));

  m_select_none_button.signal_clicked().connect(
    sigc::bind(
      sigc::mem_fun(*this,
                    &NoteRenameDialog::on_select_all_button_clicked),
      false));

  Gtk::Grid * const notes_button_box = manage(new Gtk::Grid);
  notes_button_box->set_column_spacing(5);
  notes_button_box->attach(m_select_none_button, 0, 0, 1, 1);
  notes_button_box->attach(m_select_all_button, 1, 0, 1, 1);
  notes_button_box->set_hexpand(true);

  Gtk::ScrolledWindow * const notes_scroll
                                = Gtk::manage(
                                         new Gtk::ScrolledWindow());
  notes_scroll->add(*notes_view);
  notes_scroll->set_hexpand(true);
  notes_scroll->set_vexpand(true);

  m_notes_box.attach(*notes_scroll, 0, 0, 1, 1);
  m_notes_box.attach(*notes_button_box, 0, 1, 1, 1);

  Gtk::Expander * const advanced_expander
                          = Gtk::manage(new Gtk::Expander(
                                              _("Ad_vanced"), true));
  Gtk::Grid * const expand_box = Gtk::manage(new Gtk::Grid);
  expand_box->attach(m_notes_box, 0, 0, 1, 1);

  m_always_show_dlg_radio.signal_clicked().connect(
    sigc::mem_fun(*this,
                  &NoteRenameDialog::on_always_show_dlg_clicked));

  Gtk::RadioButton::Group group = m_always_show_dlg_radio.get_group();

  m_never_rename_radio.set_group(group);
  m_never_rename_radio.signal_clicked().connect(
    sigc::mem_fun(*this,
                  &NoteRenameDialog::on_never_rename_clicked));

  m_always_rename_radio.set_group(group);
  m_always_rename_radio.signal_clicked().connect(
    sigc::mem_fun(*this,
                  &NoteRenameDialog::on_always_rename_clicked));

  expand_box->attach(m_always_show_dlg_radio, 0, 1, 1, 1);
  expand_box->attach(m_never_rename_radio, 0, 2, 1, 1);
  expand_box->attach(m_always_rename_radio, 0, 3, 1, 1);
  advanced_expander->add(*expand_box);
  vbox->pack_start(*advanced_expander, true, true, 5);

  advanced_expander->property_expanded().signal_changed().connect(
    sigc::bind(
      sigc::mem_fun(*this,
                    &NoteRenameDialog::on_advanced_expander_changed),
      advanced_expander->property_expanded().get_value()));

  set_focus(m_dont_rename_button);
  vbox->show_all();
}

NoteRenameDialog::MapPtr NoteRenameDialog::get_notes() const
{
  const MapPtr notes(new std::map<Note::Ptr, bool>);

  m_notes_model->foreach_iter(
    sigc::bind(
      sigc::mem_fun(
        *this,
        &NoteRenameDialog::on_notes_model_foreach_iter_accumulate),
      notes));
  return notes;
}

NoteRenameBehavior NoteRenameDialog::get_selected_behavior() const
{
  if (m_never_rename_radio.get_active())
    return NOTE_RENAME_ALWAYS_REMOVE_LINKS;
  else if (m_always_rename_radio.get_active())
    return NOTE_RENAME_ALWAYS_RENAME_LINKS;

  return NOTE_RENAME_ALWAYS_SHOW_DIALOG;
}

void NoteRenameDialog::on_advanced_expander_changed(bool expanded)
{
  set_resizable(expanded);
}

void NoteRenameDialog::on_always_rename_clicked()
{
  m_select_all_button.clicked();
  m_notes_box.set_sensitive(false);
  m_rename_button.set_sensitive(true);
  m_dont_rename_button.set_sensitive(false);
}

void NoteRenameDialog::on_always_show_dlg_clicked()
{
  m_select_all_button.clicked();
  m_notes_box.set_sensitive(true);
  m_rename_button.set_sensitive(true);
  m_dont_rename_button.set_sensitive(true);
}

void NoteRenameDialog::on_never_rename_clicked()
{
  m_select_none_button.clicked();
  m_notes_box.set_sensitive(false);
  m_rename_button.set_sensitive(false);
  m_dont_rename_button.set_sensitive(true);
}

bool NoteRenameDialog::on_notes_model_foreach_iter_accumulate(
                         const Gtk::TreeIter & iter,
                         const MapPtr & notes) const
{
  ModelColumnRecord model_column_record;
  const Gtk::TreeModel::Row row = *iter;

  notes->insert(std::make_pair(
                  row[model_column_record.get_column_note()],
                  row[model_column_record.get_column_selected()]));
  return false;
}

bool NoteRenameDialog::on_notes_model_foreach_iter_select(
                         const Gtk::TreeIter & iter,
                         bool select)
{
  ModelColumnRecord model_column_record;
  Gtk::TreeModel::Row row = *iter;
  row[model_column_record.get_column_selected()] = select;

  return false;
}

void NoteRenameDialog::on_notes_view_row_activated(
                         const Gtk::TreePath & p,
                         Gtk::TreeView::Column *,
                         const std::string & old_title)
{
  const Gtk::TreeModel::iterator iter = m_notes_model->get_iter(p);
  if (!iter)
    return;

  ModelColumnRecord model_column_record;
  Gtk::TreeModel::Row row = *iter;
  const Note::Ptr note = row[model_column_record.get_column_note()];
  if (!note)
    return;

  MainWindow *window = MainWindow::present_default(note);
  if(window) {
    window->set_search_text(Glib::ustring::compose("\"%1\"", old_title));
    window->show_search_bar();
  }
}

void NoteRenameDialog::on_select_all_button_clicked(bool select)
{
  m_notes_model->foreach_iter(
    sigc::bind(
      sigc::mem_fun(
        *this,
        &NoteRenameDialog::on_notes_model_foreach_iter_select),
      select));
}

void NoteRenameDialog::on_toggle_cell_toggled(const std::string & p)
{
  const Gtk::TreeModel::iterator iter = m_notes_model->get_iter(p);
  if (!iter)
    return;

  ModelColumnRecord model_column_record;
  Gtk::TreeModel::Row row = *iter;
  row[model_column_record.get_column_selected()]
    = !row[model_column_record.get_column_selected()];
}

}
