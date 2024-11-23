/*
 * gnote
 *
 * Copyright (C) 2013-2014,2017,2019,2023-2024 Aurimas Cernius
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
#include "utils.hpp"
#include "base/hash.hpp"
#include "notebooks/notebookmanager.hpp"


namespace statistics {

struct StatisticsRow
{
  Glib::ustring statistic;
  Glib::ustring value;
};

typedef gnote::utils::ModelRecord<StatisticsRow> StatisticsRecord;


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
      g.notebook_manager().signal_note_added_to_notebook
        .connect(sigc::mem_fun(*this, &StatisticsModel::on_notebook_note_list_changed));
      g.notebook_manager().signal_note_removed_from_notebook
        .connect(sigc::mem_fun(*this, &StatisticsModel::on_notebook_note_list_changed));
    }

  void build_stats()
    {
      m_model->remove_all();

      m_model->append(StatisticsRecord::create({_("Total Notes"), TO_STRING(m_note_manager.note_count())}));

      struct NotebookHash
      {
        std::size_t operator()(const gnote::notebooks::Notebook::Ref & nb) const noexcept
          {
            gnote::Hash<Glib::ustring> hash;
            return hash(nb.get().get_normalized_name());
          }
      };
      struct NotebookEq
      {
        bool operator()(const gnote::notebooks::Notebook::Ref& x, const gnote::notebooks::Notebook::Ref& y) const noexcept
          {
            return &x.get() == &y.get();
          }
      };
      typedef std::unordered_map<gnote::notebooks::Notebook::Ref, unsigned, NotebookHash, NotebookEq> NotebookMap;
      NotebookMap notebooks;
      m_gnote.notebook_manager().get_notebooks([&notebooks](const gnote::notebooks::Notebook::Ptr& nb) { notebooks[*nb] = 0; });
      m_model->append(StatisticsRecord::create({_("Total Notebooks"), TO_STRING(notebooks.size())}));

      auto &template_tag = m_note_manager.tag_manager().get_or_create_system_tag(
        gnote::ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);
      m_note_manager.for_each([&notebooks, template_tag](gnote::NoteBase & note) {
        for(auto & nb : notebooks) {
          if(note.contains_tag(*nb.first.get().get_tag()) && !note.contains_tag(template_tag)) {
            ++nb.second;
          }
        }
      });
      for(const auto& nb : notebooks) {
        // TRANSLATORS: %1 is the format placeholder for the number of notes.
        char *fmt = ngettext("%1 note", "%1 notes", nb.second);
        m_model->append(StatisticsRecord::create({"\t" + nb.first.get().get_name(), Glib::ustring::compose(fmt, nb.second)}));
      }

      DBG_OUT("Statistics updated");
    }

  void on_note_list_changed(gnote::NoteBase &)
    {
      update();
    }

  void on_notebook_note_list_changed(const gnote::Note &, const gnote::notebooks::Notebook &)
    {
      update();
    }

  gnote::IGnote & m_gnote;
  gnote::NoteManager & m_note_manager;
  Glib::RefPtr<Gio::ListStore<StatisticsRecord>> m_model;
  bool m_active;
};


class StatisticsListItemFactory
  : public gnote::utils::LabelFactory
{
public:
  static Glib::RefPtr<StatisticsListItemFactory> create()
    {
      return Glib::make_refptr_for_instance(new StatisticsListItemFactory);
    }
protected:
  Glib::ustring get_text(Gtk::ListItem & item) override
  {
    auto record = std::dynamic_pointer_cast<StatisticsRecord>(item.get_item());
    return Glib::ustring::compose("<b>%1:</b>\t%2", record->value.statistic, record->value.value);
  }

  void set_text(Gtk::Label & label, const Glib::ustring & text) override
  {
    label.set_markup(text);
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

