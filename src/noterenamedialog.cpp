/*
 * gnote
 *
 * Copyright (C) 2011-2014,2017,2019,2021-2023 Aurimas Cernius
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

#include <glibmm/i18n.h>
#include <gtkmm/expression.h>
#include <gtkmm/columnview.h>
#include <gtkmm/expander.h>
#include <gtkmm/label.h>
#include <gtkmm/numericsorter.h>
#include <gtkmm/singleselection.h>
#include <gtkmm/sortlistmodel.h>
#include <gtkmm/stringsorter.h>

#include "ignote.hpp"
#include "mainwindow.hpp"
#include "notewindow.hpp"
#include "noterenamedialog.hpp"

namespace gnote {

namespace {

class ToggleFactory
  : public Gtk::SignalListItemFactory
{
public:
  static Glib::RefPtr<ToggleFactory> create()
    {
      return Glib::make_refptr_for_instance(new ToggleFactory);
    }
private:
  ToggleFactory()
    {
      signal_setup().connect(sigc::mem_fun(*this, &ToggleFactory::on_setup));
      signal_bind().connect(sigc::mem_fun(*this, &ToggleFactory::on_bind));
      signal_unbind().connect(sigc::mem_fun(*this, &ToggleFactory::on_unbind));
    }

  void on_setup(const Glib::RefPtr<Gtk::ListItem> & list_item)
    {
      list_item->set_child(*Gtk::make_managed<Gtk::CheckButton>());
    }

  void on_bind(const Glib::RefPtr<Gtk::ListItem> & list_item)
    {
      auto record = get_record(list_item);
      auto check_button = get_check_button(list_item);
      check_button->set_active(record->selected());
      record->set_check_button(check_button);
      record->signal_cid = check_button->signal_toggled().connect([record, check_button] {
        record->selected(check_button->get_active());
      });
    }

  void on_unbind(const Glib::RefPtr<Gtk::ListItem> & list_item)
    {
      auto record = get_record(list_item);
      record->signal_cid.disconnect();
      record->set_check_button(nullptr);
    }

  Glib::RefPtr<NoteRenameRecord> get_record(const Glib::RefPtr<Gtk::ListItem> & list_item)
    {
      return std::dynamic_pointer_cast<NoteRenameRecord>(list_item->get_item());
    }

  Gtk::CheckButton *get_check_button(const Glib::RefPtr<Gtk::ListItem> & list_item)
    {
      return dynamic_cast<Gtk::CheckButton*>(list_item->get_child());
    }
};

class NoteTitleFactory
  : public utils::LabelFactory
{
public:
  static Glib::RefPtr<NoteTitleFactory> create()
    {
      return Glib::make_refptr_for_instance(new NoteTitleFactory);
    }
protected:
  Glib::ustring get_text(Gtk::ListItem & item) override
    {
      return std::dynamic_pointer_cast<NoteRenameRecord>(item.get_item())->note->get_title();
    }
  void set_text(Gtk::Label & label, const Glib::ustring & text) override
    {
      label.set_text(text);
    }
};

}

Glib::RefPtr<NoteRenameRecord> NoteRenameRecord::create(const NoteBase::Ptr & note, bool selected)
{
  return Glib::make_refptr_for_instance(new NoteRenameRecord(note, selected));
}

NoteRenameRecord::NoteRenameRecord(const NoteBase::Ptr & note, bool selected)
  : note(note)
  , m_selected(selected)
{
}

NoteRenameRecord::~NoteRenameRecord()
{
}

void NoteRenameRecord::selected(bool select)
{
  m_selected = select;
  if(m_check_button) {
    m_check_button->set_active(select);
  }
}

NoteRenameDialog::NoteRenameDialog(const NoteBase::List & notes,
                                   const Glib::ustring & old_title,
                                   const NoteBase::Ptr & renamed_note,
                                   IGnote & g)
  : Gtk::Dialog(_("Rename Note Links?"),
                *dynamic_cast<Gtk::Window*>(std::static_pointer_cast<Note>(renamed_note)->get_window()->host()),
                false)
  , m_gnote(g)
  , m_notes_model(Gio::ListStore<NoteRenameRecord>::create())
  , m_dont_rename_button(_("_Don't Rename Links"), true)
  , m_rename_button(_("_Rename Links"), true)
  , m_select_all_button(_("Select All"))
  , m_select_none_button(_("Select None"))
  , m_always_show_dlg_radio(_("Always show this _window"), true)
  , m_always_rename_radio(_("Alwa_ys rename links"), true)
  , m_never_rename_radio(_("Never rename _links"), true)
{
  set_default_response(Gtk::ResponseType::CANCEL);
  set_margin(10);

  Gtk::Box *const vbox = get_content_area();

  add_action_widget(m_rename_button, Gtk::ResponseType::YES);
  add_action_widget(m_dont_rename_button, Gtk::ResponseType::NO);

  for(const auto & note : notes) {
    m_notes_model->append(NoteRenameRecord::create(note, true));
  };

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
  label->set_wrap(true);
  label->set_margin(5);
  vbox->append(*label);

  auto notes_view = Gtk::make_managed<Gtk::ColumnView>();
  notes_view->signal_activate().connect([this, old_title](guint pos) { on_notes_view_row_activated(pos, old_title); });

  {
    auto column = Gtk::ColumnViewColumn::create(_("Rename Links"), ToggleFactory::create());
    auto expr = Gtk::ClosureExpression<bool>::create([](const Glib::RefPtr<Glib::ObjectBase> & item) {
      return std::dynamic_pointer_cast<NoteRenameRecord>(item)->selected();
    });
    column->set_sorter(Gtk::NumericSorter<bool>::create(expr));
    column->set_resizable(true);
    notes_view->append_column(column);
  }

  {
    auto column = Gtk::ColumnViewColumn::create(_("Note Title"), NoteTitleFactory::create());
    auto expr = Gtk::ClosureExpression<Glib::ustring>::create([](const Glib::RefPtr<Glib::ObjectBase> & item) {
      return std::dynamic_pointer_cast<NoteRenameRecord>(item)->note->get_title();
    });
    column->set_sorter(Gtk::StringSorter::create(expr));
    column->set_resizable(true);
    notes_view->append_column(column);
  }

  {
    auto sort_model = Gtk::SortListModel::create(m_notes_model, notes_view->get_sorter());
    auto selection = Gtk::SingleSelection::create(sort_model);
    notes_view->set_model(selection);
  }

  m_select_all_button.signal_clicked().connect([this] {
    on_select_all_button_clicked(true);
  });

  m_select_none_button.signal_clicked().connect([this] {
    on_select_all_button_clicked(false);
  });

  auto notes_button_box = Gtk::make_managed<Gtk::Grid>();
  notes_button_box->set_column_spacing(5);
  notes_button_box->attach(m_select_none_button, 0, 0, 1, 1);
  notes_button_box->attach(m_select_all_button, 1, 0, 1, 1);
  notes_button_box->set_hexpand(true);

  auto notes_scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
  notes_scroll->set_child(*notes_view);
  notes_scroll->set_hexpand(true);
  notes_scroll->set_vexpand(true);
  notes_scroll->set_size_request(-1, 200);

  m_notes_box.attach(*notes_scroll, 0, 0, 1, 1);
  m_notes_box.attach(*notes_button_box, 0, 1, 1, 1);

  auto advanced_expander = Gtk::make_managed<Gtk::Expander>(_("Ad_vanced"), true);
  auto expand_box = Gtk::make_managed<Gtk::Grid>();
  expand_box->attach(m_notes_box, 0, 0, 1, 1);

  m_always_show_dlg_radio.set_active(true);
  m_always_show_dlg_radio.signal_toggled().connect(
    sigc::mem_fun(*this,
                  &NoteRenameDialog::on_always_show_dlg_clicked));

  m_never_rename_radio.set_group(m_always_show_dlg_radio);
  m_never_rename_radio.signal_toggled().connect(
    sigc::mem_fun(*this,
                  &NoteRenameDialog::on_never_rename_clicked));

  m_always_rename_radio.set_group(m_always_show_dlg_radio);
  m_always_rename_radio.signal_toggled().connect(
    sigc::mem_fun(*this,
                  &NoteRenameDialog::on_always_rename_clicked));

  expand_box->attach(m_always_show_dlg_radio, 0, 1, 1, 1);
  expand_box->attach(m_never_rename_radio, 0, 2, 1, 1);
  expand_box->attach(m_always_rename_radio, 0, 3, 1, 1);
  advanced_expander->set_child(*expand_box);
  advanced_expander->set_margin(5);
  advanced_expander->set_expand(true);
  vbox->append(*advanced_expander);

  advanced_expander->property_expanded().signal_changed().connect(
    sigc::bind(
      sigc::mem_fun(*this,
                    &NoteRenameDialog::on_advanced_expander_changed),
      advanced_expander->property_expanded().get_value()));

  set_focus(m_dont_rename_button);
}

NoteRenameDialog::MapPtr NoteRenameDialog::get_notes() const
{
  const MapPtr notes(std::make_shared<std::map<NoteBase::Ptr, bool>>());
  auto count = m_notes_model->get_n_items();
  for(guint i = 0; i < count; ++i) {
    auto record = m_notes_model->get_item(i);
    notes->insert(std::make_pair(record->note, record->selected()));
  }
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
  on_select_all_button_clicked(true);
  m_notes_box.set_sensitive(false);
  m_rename_button.set_sensitive(true);
  m_dont_rename_button.set_sensitive(false);
}

void NoteRenameDialog::on_always_show_dlg_clicked()
{
  on_select_all_button_clicked(true);
  m_notes_box.set_sensitive(true);
  m_rename_button.set_sensitive(true);
  m_dont_rename_button.set_sensitive(true);
}

void NoteRenameDialog::on_never_rename_clicked()
{
  on_select_all_button_clicked(true);
  m_notes_box.set_sensitive(false);
  m_rename_button.set_sensitive(false);
  m_dont_rename_button.set_sensitive(true);
}

void NoteRenameDialog::on_notes_view_row_activated(guint pos, const Glib::ustring & old_title)
{
  auto item = m_notes_model->get_item(pos);
  if(!item) {
    return;
  }

  auto & window = MainWindow::present_default(m_gnote, static_cast<Note&>(*item->note));
  window.set_search_text(Glib::ustring::compose("\"%1\"", old_title));
  window.show_search_bar();
}

void NoteRenameDialog::on_select_all_button_clicked(bool select)
{
  auto count = m_notes_model->get_n_items();
  for(guint i = 0; i < count; ++i) {
    auto item = m_notes_model->get_item(i);
    item->selected(select);
  }
}

}
