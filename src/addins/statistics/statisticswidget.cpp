/*
 * gnote
 *
 * Copyright (C) 2013 Aurimas Cernius
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


#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <glibmm/i18n.h>
#include <gtkmm/treestore.h>

#include "debug.hpp"
#include "itagmanager.hpp"
#include "statisticswidget.hpp"
#include "notebooks/notebookmanager.hpp"


namespace statistics {

class StatisticsModel
  : public Gtk::TreeStore
{
public:
  typedef Glib::RefPtr<StatisticsModel> Ptr;
  static Ptr create(gnote::NoteManager & nm)
    {
      return Ptr(new StatisticsModel(nm));
    }

  void update()
    {
      if(m_active) {
        build_stats();
      }
    }

  void active(bool is_active)
    {
      m_active = is_active;
    }
private:
  class StatisticsRecord
    : public Gtk::TreeModelColumnRecord
  {
  public:
    StatisticsRecord()
      {
        add(m_stat);
        add(m_value);
      }
  private:
    Gtk::TreeModelColumn<std::string> m_stat;
    Gtk::TreeModelColumn<std::string> m_value;
  };
  StatisticsRecord m_columns;

  StatisticsModel(gnote::NoteManager & nm)
    : m_note_manager(nm)
    , m_active(false)
    {
      set_column_types(m_columns);
      build_stats();
      nm.signal_note_added.connect(sigc::mem_fun(*this, &StatisticsModel::on_note_list_changed));
      nm.signal_note_deleted.connect(sigc::mem_fun(*this, &StatisticsModel::on_note_list_changed));
      gnote::notebooks::NotebookManager::obj().signal_note_added_to_notebook()
        .connect(sigc::mem_fun(*this, &StatisticsModel::on_notebook_note_list_changed));
      gnote::notebooks::NotebookManager::obj().signal_note_removed_from_notebook()
        .connect(sigc::mem_fun(*this, &StatisticsModel::on_notebook_note_list_changed));
    }

  void build_stats()
    {
      clear();
      gnote::Note::List notes = m_note_manager.get_notes();

      Gtk::TreeIter iter = append();
      std::string stat = _("Total Notes:");
      iter->set_value(0, stat);
      iter->set_value(1, boost::lexical_cast<std::string>(notes.size()));

      Glib::RefPtr<Gtk::TreeModel> notebooks = gnote::notebooks::NotebookManager::obj().get_notebooks();
      iter = append();
      stat = _("Total Notebooks:");
      iter->set_value(0, stat);
      iter->set_value(1, boost::lexical_cast<std::string>(notebooks->children().size()));

      Gtk::TreeIter notebook = notebooks->children().begin();
      std::map<gnote::notebooks::Notebook::Ptr, int> notebook_notes;
      while(notebook) {
        gnote::notebooks::Notebook::Ptr nbook;
        notebook->get_value(0, nbook);
        notebook_notes[nbook] = 0;
        ++notebook;
      }
      gnote::Tag::Ptr template_tag = gnote::ITagManager::obj().get_or_create_system_tag(
        gnote::ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
      for(gnote::Note::List::iterator note = notes.begin(); note != notes.end(); ++note) {
        for(std::map<gnote::notebooks::Notebook::Ptr, int>::iterator nb = notebook_notes.begin();
            nb != notebook_notes.end(); ++nb) {
          if((*note)->contains_tag(nb->first->get_tag()) && !(*note)->contains_tag(template_tag)) {
            ++nb->second;
          }
        }
      }
      std::map<std::string, int> notebook_stats;
      for(std::map<gnote::notebooks::Notebook::Ptr, int>::iterator nb = notebook_notes.begin();
          nb != notebook_notes.end(); ++nb) {
        notebook_stats[nb->first->get_name()] = nb->second;
      }
      for(std::map<std::string, int>::iterator nb = notebook_stats.begin(); nb != notebook_stats.end(); ++nb) {
        Gtk::TreeIter nb_stat = append(iter->children());
        nb_stat->set_value(0, nb->first);
        char *fmt = ngettext("%1% note", "%1% notes", nb->second);
        nb_stat->set_value(1, str(boost::format(fmt) % nb->second));
      }

      DBG_OUT("Statistics updated");
    }

  void on_note_list_changed(const gnote::Note::Ptr &)
    {
      update();
    }

  void on_notebook_note_list_changed(const gnote::Note &, const gnote::notebooks::Notebook::Ptr &)
    {
      update();
    }

  gnote::NoteManager & m_note_manager;
  bool m_active;
};


StatisticsWidget::StatisticsWidget(gnote::NoteManager & nm)
  : Gtk::TreeView(StatisticsModel::create(nm))
{
  set_hexpand(true);
  set_vexpand(true);
  StatisticsModel::Ptr model = StatisticsModel::Ptr::cast_dynamic(get_model());
  set_model(model);
  set_headers_visible(false);

  Gtk::CellRendererText *renderer = manage(new Gtk::CellRendererText);
  Gtk::TreeViewColumn *column = manage(new Gtk::TreeViewColumn("", *renderer));
  column->set_cell_data_func(*renderer, sigc::mem_fun(*this, &StatisticsWidget::col1_data_func));
  append_column(*column);

  renderer = manage(new Gtk::CellRendererText);
  column = manage(new Gtk::TreeViewColumn("", *renderer));
  column->set_cell_data_func(*renderer, sigc::mem_fun(*this, &StatisticsWidget::col2_data_func));
  append_column(*column);
}

std::string StatisticsWidget::get_name() const
{
  return _("Statistics");
}

void StatisticsWidget::foreground()
{
  gnote::EmbeddableWidget::foreground();
  StatisticsModel::Ptr model = StatisticsModel::Ptr::cast_static(get_model());
  model->active(true);
  model->update();
  expand_all();
}

void StatisticsWidget::background()
{
  gnote::EmbeddableWidget::background();
  StatisticsModel::Ptr::cast_static(get_model())->active(false);
}

void StatisticsWidget::col1_data_func(Gtk::CellRenderer *renderer, const Gtk::TreeIter & iter)
{
  std::string val;
  iter->get_value(0, val);
  static_cast<Gtk::CellRendererText*>(renderer)->property_markup() = "<b>" + val + "</b>";
}

void StatisticsWidget::col2_data_func(Gtk::CellRenderer *renderer, const Gtk::TreeIter & iter)
{
  std::string val;
  iter->get_value(1, val);
  static_cast<Gtk::CellRendererText*>(renderer)->property_text() = val;
}

}

