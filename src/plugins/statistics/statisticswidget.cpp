/*
 * gnote
 *
 * Copyright (C) 2013-2014,2017,2019,2023 Aurimas Cernius
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


#include <giomm/liststore.h>
#include <glibmm/i18n.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/singleselection.h>

#include "debug.hpp"
#include "ignote.hpp"
#include "itagmanager.hpp"
#include "statisticswidget.hpp"
#include "notebooks/notebookmanager.hpp"


namespace statistics {

class StatisticsRecord
  : public Glib::Object
{
public:
  static Glib::RefPtr<StatisticsRecord> create(const Glib::ustring & stat, const Glib::ustring & value)
    {
      return Glib::make_refptr_for_instance(new StatisticsRecord(stat, value));
    }

  const Glib::ustring statistic;
  const Glib::ustring value;
private:
  StatisticsRecord(const Glib::ustring & stat, const Glib::ustring & value)
    : statistic(stat)
    , value(value)
    {
    }

};


class StatisticsModel
  : public Gtk::SingleSelection
{
public:
  typedef Glib::RefPtr<StatisticsModel> Ptr;
  static Ptr create(gnote::IGnote & g, gnote::NoteManager & nm)
    {
      return Glib::make_refptr_for_instance(new StatisticsModel(g, nm));
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
  StatisticsModel(gnote::IGnote & g, gnote::NoteManager & nm)
    : m_gnote(g)
    , m_note_manager(nm)
    , m_model(Gio::ListStore<StatisticsRecord>::create())
    , m_active(false)
    {
      set_model(m_model);
      nm.signal_note_added.connect(sigc::mem_fun(*this, &StatisticsModel::on_note_list_changed));
      nm.signal_note_deleted.connect(sigc::mem_fun(*this, &StatisticsModel::on_note_list_changed));
      g.notebook_manager().signal_note_added_to_notebook()
        .connect(sigc::mem_fun(*this, &StatisticsModel::on_notebook_note_list_changed));
      g.notebook_manager().signal_note_removed_from_notebook()
        .connect(sigc::mem_fun(*this, &StatisticsModel::on_notebook_note_list_changed));
    }

  void build_stats()
    {
      m_model->remove_all();
      gnote::NoteBase::List notes = m_note_manager.get_notes();

      m_model->append(StatisticsRecord::create(_("Total Notes"), TO_STRING(notes.size())));

      auto notebooks = m_gnote.notebook_manager().get_notebooks();
      m_model->append(StatisticsRecord::create(_("Total Notebooks"), TO_STRING(notebooks->children().size())));

      Gtk::TreeIter notebook = notebooks->children().begin();
      std::map<gnote::notebooks::Notebook::Ptr, int> notebook_notes;
      while(notebook) {
        gnote::notebooks::Notebook::Ptr nbook;
        notebook->get_value(0, nbook);
        notebook_notes[nbook] = 0;
        ++notebook;
      }
      gnote::Tag::Ptr template_tag = m_note_manager.tag_manager().get_or_create_system_tag(
        gnote::ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
      for(gnote::NoteBase::Ptr note : notes) {
        for(std::map<gnote::notebooks::Notebook::Ptr, int>::iterator nb = notebook_notes.begin();
            nb != notebook_notes.end(); ++nb) {
          if(note->contains_tag(nb->first->get_tag()) && !note->contains_tag(template_tag)) {
            ++nb->second;
          }
        }
      }
      std::map<Glib::ustring, int> notebook_stats;
      for(std::map<gnote::notebooks::Notebook::Ptr, int>::iterator nb = notebook_notes.begin();
          nb != notebook_notes.end(); ++nb) {
        notebook_stats[nb->first->get_name()] = nb->second;
      }
      for(auto nb : notebook_stats) {
        // TRANSLATORS: %1 is the format placeholder for the number of notes.
        char *fmt = ngettext("%1 note", "%1 notes", nb.second);
        m_model->append(StatisticsRecord::create("\t" + nb.first, Glib::ustring::compose(fmt, nb.second)));
      }

      DBG_OUT("Statistics updated");
    }

  void on_note_list_changed(const gnote::NoteBase::Ptr &)
    {
      update();
    }

  void on_notebook_note_list_changed(const gnote::Note &, const gnote::notebooks::Notebook::Ptr &)
    {
      update();
    }

  gnote::IGnote & m_gnote;
  gnote::NoteManager & m_note_manager;
  Glib::RefPtr<Gio::ListStore<StatisticsRecord>> m_model;
  bool m_active;
};


class StatisticsListItemFactory
  : public Gtk::SignalListItemFactory
{
public:
  static Glib::RefPtr<StatisticsListItemFactory> create()
    {
      return Glib::make_refptr_for_instance(new StatisticsListItemFactory);
    }
private:
  StatisticsListItemFactory()
    {
      signal_setup().connect(sigc::mem_fun(*this, &StatisticsListItemFactory::setup));
      signal_bind().connect(sigc::mem_fun(*this, &StatisticsListItemFactory::bind));
    }

  void setup(const Glib::RefPtr<Gtk::ListItem> & item)
    {
      auto label = Gtk::make_managed<Gtk::Label>();
      label->set_halign(Gtk::Align::START);
      item->set_child(*label);
    }

  void bind(const Glib::RefPtr<Gtk::ListItem> & item)
    {
      auto record = std::dynamic_pointer_cast<StatisticsRecord>(item->get_item());
      auto label = dynamic_cast<Gtk::Label*>(item->get_child());
      label->set_markup(Glib::ustring::compose("<b>%1:</b>\t%2", record->statistic, record->value));
    }
};


StatisticsWidget::StatisticsWidget(gnote::IGnote & g, gnote::NoteManager & nm)
  : Gtk::ListView(StatisticsModel::create(g, nm))
{
  set_hexpand(true);
  set_vexpand(true);
  StatisticsModel::Ptr model = std::dynamic_pointer_cast<StatisticsModel>(get_model());
  set_model(model);
  set_factory(StatisticsListItemFactory::create());
  model->update();
}

Glib::ustring StatisticsWidget::get_name() const
{
  return _("Statistics");
}

void StatisticsWidget::foreground()
{
  gnote::EmbeddableWidget::foreground();
  StatisticsModel::Ptr model = std::static_pointer_cast<StatisticsModel>(get_model());
  model->active(true);
  model->update();
}

void StatisticsWidget::background()
{
  gnote::EmbeddableWidget::background();
  std::static_pointer_cast<StatisticsModel>(get_model())->active(false);
}

}

