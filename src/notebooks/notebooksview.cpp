/*
 * gnote
 *
 * Copyright (C) 2011-2014,2019,2022-2023 Aurimas Cernius
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



#include <gtkmm/droptarget.h>
#include <gtkmm/singleselection.h>

#include "debug.hpp"
#include "iconmanager.hpp"
#include "notebooks/notebook.hpp"
#include "notebooks/notebookmanager.hpp"
#include "notebooks/notebooksview.hpp"
#include "notebooks/specialnotebooks.hpp"
#include "notemanagerbase.hpp"

namespace gnote {

  namespace {

    class NotebookBox
      : public Gtk::Box
    {
    public:
      NotebookBox()
        {
          auto drop_target = Gtk::DropTarget::create(G_TYPE_INVALID, Gdk::DragAction::COPY);
          std::vector<GType> types;
          types.push_back(Glib::Value<Glib::ustring>::value_type());
          types.push_back(Glib::Value<std::vector<Glib::ustring>>::value_type());
          drop_target->set_gtypes(types);
          drop_target->signal_drop().connect(sigc::mem_fun(*this, &NotebookBox::on_drag_data_received), false);
          add_controller(drop_target);
        }

      void set_notebook(const notebooks::Notebook::Ptr& nb)
        {
          m_notebook = nb;
        }
    private:
      bool on_drag_data_received(const Glib::ValueBase & value, double x, double y)
        {
          auto dest_notebook = m_notebook;
          if(!dest_notebook) {
            return false;
          }
          if(std::dynamic_pointer_cast<notebooks::AllNotesNotebook>(dest_notebook)) {
            return false;
          }

          auto drop = [this, &dest_notebook](const Glib::ustring & uri) {
            return dest_notebook->note_manager().find_by_uri(uri, [dest_notebook](NoteBase & note) {
              DBG_OUT("Dropped into notebook: %s", note.get_title().c_str());
              dest_notebook->add_note(static_cast<Note&>(note));
            });
          };

          if(G_VALUE_HOLDS_STRING(value.gobj())) {
            Glib::ustring val = static_cast<const Glib::Value<Glib::ustring>&>(value).get();
            return drop(val);
          }
          else if(G_VALUE_HOLDS(value.gobj(), Glib::Value<std::vector<Glib::ustring>>::value_type())) {
            auto uris = static_cast<const Glib::Value<std::vector<Glib::ustring>>&>(value).get();
            bool ret = false;
            for(const auto & uri : uris) {
              ret = drop(uri) || ret;
            }

            return ret;
          }

          return false;
        }

      notebooks::Notebook::Ptr m_notebook;
    };

    class NotebookFactory
      : public Gtk::SignalListItemFactory
    {
    public:
      static Glib::RefPtr<NotebookFactory> create()
        {
          return Glib::make_refptr_for_instance(new NotebookFactory);
        }
    private:
      NotebookFactory()
        {
          signal_setup().connect(sigc::mem_fun(*this, &NotebookFactory::on_setup));
          signal_bind().connect(sigc::mem_fun(*this, &NotebookFactory::on_bind));
          signal_unbind().connect(sigc::mem_fun(*this, &NotebookFactory::on_unbind));
        }

      void on_setup(const Glib::RefPtr<Gtk::ListItem>& item)
        {
          auto box = Gtk::make_managed<NotebookBox>();
          auto icon = Gtk::make_managed<Gtk::Image>();
          box->append(*icon);
          auto label = Gtk::make_managed<Gtk::Label>();
          label->set_halign(Gtk::Align::START);
          box->append(*label);
          item->set_child(*box);
        }

      void on_bind(const Glib::RefPtr<Gtk::ListItem>& item)
        {
          auto box = dynamic_cast<NotebookBox*>(item->get_child());
          if(!box) {
            ERR_OUT("Not notebook box. This is a bug, please report it!");
          }
          auto icon = static_cast<Gtk::Image*>(box->get_first_child());
          auto label = static_cast<Gtk::Label*>(box->get_last_child());
          if(auto nb = std::dynamic_pointer_cast<notebooks::SpecialNotebook>(item->get_item())) {
            icon->property_icon_name() = nb->get_icon_name();
            label->set_markup(Glib::ustring::compose("<b>%1</b>", nb->get_name()));
            box->set_notebook(nb);
          }
          else if(auto nb = std::dynamic_pointer_cast<notebooks::Notebook>(item->get_item())) {
            icon->property_icon_name() = IconManager::NOTEBOOK;
            label->set_text(nb->get_name());
            box->set_notebook(nb);
          }
          else {
            ERR_OUT("Note a notebook. This should not happen, please report a bug!");
          }
        }

      void on_unbind(const Glib::RefPtr<Gtk::ListItem>& item)
        {
          if(auto box = dynamic_cast<NotebookBox*>(item->get_child())) {
            box->set_notebook(nullptr);
          }
          else {
            ERR_OUT("Not notebook box. This is a bug, please report it!");
          }
        }
    };

  }

  namespace notebooks {

    NotebooksView::NotebooksView(NoteManagerBase & manager, const Glib::RefPtr<Gio::ListModel> & model)
      : Gtk::ListView(Gtk::SingleSelection::create(model), NotebookFactory::create())
      , m_note_manager(manager)
    {
      get_model()->signal_selection_changed().connect(sigc::mem_fun(*this, &NotebooksView::on_selection_changed));
    }

    Notebook::ORef NotebooksView::get_selected_notebook() const
    {
      auto selection = std::dynamic_pointer_cast<const Gtk::SingleSelection>(get_model());
      if(!selection) {
        return Notebook::ORef();
      }
      auto item = selection->get_selected_item();
      if(!item) {
        return Notebook::ORef();
      }
      auto obj = const_cast<ObjectBase*>(item.get());
      auto notebook = dynamic_cast<Notebook*>(obj);
      if(!notebook) {
        return Notebook::ORef(); // Nothing selected
      }

      return *notebook;
    }

    void NotebooksView::select_all_notes_notebook()
    {
      auto model = std::dynamic_pointer_cast<Gtk::SingleSelection>(get_model());
      DBG_ASSERT(model, "model is NULL");
      if(!model) {
        return;
      }
      if(auto all_notes = std::dynamic_pointer_cast<AllNotesNotebook>(model->get_selected_item())) {
        return;
      }

      const auto n_items = model->get_n_items();
      auto list = model->get_model();
      for(guint i = 0; i < n_items; ++i) {
        if(std::dynamic_pointer_cast<notebooks::AllNotesNotebook>(list->get_object(i))) {
          model->set_selected(i);
          break;
        }
      }
    }

    void NotebooksView::select_notebook(Notebook& notebook)
    {
      auto model = std::dynamic_pointer_cast<Gtk::SingleSelection>(get_model());
      DBG_ASSERT(model, "model is NULL");
      if(!model) {
        return;
      }

      auto list = model->get_model();
      const auto n_items = list->get_n_items();
      for(guint i = 0; i < n_items; ++i) {
        auto obj = std::dynamic_pointer_cast<Notebook>(list->get_object(i));
        if(&*obj == &notebook) {
          model->set_selected(i);
          break;
        }
      }
    }

    void NotebooksView::set_notebooks(const Glib::RefPtr<Gio::ListModel> & model)
    {
      auto selection = std::dynamic_pointer_cast<Gtk::SingleSelection>(get_model());
      DBG_ASSERT(selection, "selection is NULL");
      if(!selection) {
        return;
      }
      selection->set_model(model);
    }

    void NotebooksView::on_selection_changed(guint, guint)
    {
      if(auto notebook = get_selected_notebook()) {
        signal_selected_notebook_changed(notebook.value());
      }
      else {
        select_all_notes_notebook();
      }
    }
  }
}
