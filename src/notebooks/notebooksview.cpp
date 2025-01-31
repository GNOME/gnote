/*
 * gnote
 *
 * Copyright (C) 2011-2014,2019,2022-2025 Aurimas Cernius
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



#include <glibmm/i18n.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/expression.h>
#include <gtkmm/droptarget.h>
#include <gtkmm/multisorter.h>
#include <gtkmm/numericsorter.h>
#include <gtkmm/singleselection.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/sortlistmodel.h>
#include <gtkmm/stringsorter.h>

#include "debug.hpp"
#include "iconmanager.hpp"
#include "notebook.hpp"
#include "notebookmanager.hpp"
#include "notebooknamepopover.hpp"
#include "notebooksview.hpp"
#include "notemanagerbase.hpp"
#include "specialnotebooks.hpp"

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


    Glib::RefPtr<Gtk::SingleSelection> make_selection_model(const Glib::RefPtr<Gio::ListModel> & list_model)
    {
      auto type_expr = Gtk::ClosureExpression<guint>::create([](const Glib::RefPtr<Glib::ObjectBase>& obj) {
        if(auto nb = std::dynamic_pointer_cast<notebooks::Notebook>(obj)) {
          if(auto snb = std::dynamic_pointer_cast<notebooks::SpecialNotebook>(nb)) {
            if(std::dynamic_pointer_cast<notebooks::AllNotesNotebook>(snb)) {
              return 0;
            }
            return 1;
          }
          return 100;
        }
        return INT_MAX;
      });
      auto type_sorter = Gtk::NumericSorter<guint>::create(type_expr);
      auto name_expr = Gtk::ClosureExpression<Glib::ustring>::create([](const Glib::RefPtr<Glib::ObjectBase>& obj) {
        if(auto nb = std::dynamic_pointer_cast<notebooks::Notebook>(obj)) {
          return nb->get_name();
        }
        return Glib::ustring();
      });
      auto name_sorter = Gtk::StringSorter::create(name_expr);
      auto sorter = Gtk::MultiSorter::create();
      sorter->append(type_sorter);
      sorter->append(name_sorter);
      auto sort_model = Gtk::SortListModel::create(list_model, sorter);
      return Gtk::SingleSelection::create(sort_model);
    }

  }

  namespace notebooks {

    NotebooksView::NotebooksView(NoteManagerBase & manager, const Glib::RefPtr<Gio::ListModel> & model)
      : Gtk::Box(Gtk::Orientation::VERTICAL)
      , m_note_manager(manager)
      , m_list(make_selection_model(model))
    {
      auto column = Gtk::ColumnViewColumn::create(_("Notebook"), NotebookFactory::create());
      column->set_expand(true);
      m_list.append_column(column);

      m_new_button.set_icon_name("list-add-symbolic");
      m_new_button.set_tooltip_text(_("New Notebook"));
      m_new_button.set_has_frame(false);
      m_new_button.signal_clicked().connect(sigc::mem_fun(*this, &NotebooksView::on_create_new_notebook));
      m_rename_button.set_icon_name("document-edit-symbolic");
      m_rename_button.set_tooltip_text(_("Rename Notebook"));
      m_rename_button.set_has_frame(false);
      m_rename_button.set_sensitive(false);
      m_rename_button.signal_clicked().connect(sigc::mem_fun(*this, &NotebooksView::on_rename_notebook));
      m_open_template_note.set_icon_name("emblem-system-symbolic");
      m_open_template_note.set_tooltip_text(_("Open Template Note"));
      m_open_template_note.set_has_frame(false);
      m_open_template_note.signal_clicked().connect(sigc::mem_fun(*this, &NotebooksView::on_open_template_note));
      m_delete_button.set_icon_name("edit-delete-symbolic");
      m_delete_button.set_tooltip_text(_("Delete Notebook"));
      m_delete_button.set_has_frame(false);
      m_delete_button.set_sensitive(false);
      m_delete_button.signal_clicked().connect(sigc::mem_fun(*this, &NotebooksView::on_delete_notebook));

      auto key_ctrl = Gtk::EventControllerKey::create();
      key_ctrl->signal_key_pressed().connect(sigc::mem_fun(*this, &NotebooksView::on_notebooks_key_pressed), false);
      m_list.add_controller(key_ctrl);

      Gtk::ScrolledWindow *sw = new Gtk::ScrolledWindow();
      sw->property_hscrollbar_policy() = Gtk::PolicyType::AUTOMATIC;
      sw->property_vscrollbar_policy() = Gtk::PolicyType::AUTOMATIC;
      sw->set_expand(true);
      sw->set_child(m_list);
      append(*sw);
      auto actions = Gtk::make_managed<Gtk::Box>();
      actions->append(m_new_button);
      actions->append(m_rename_button);
      actions->append(m_open_template_note);
      actions->append(m_delete_button);
      append(*actions);

      m_list.get_model()->signal_selection_changed().connect(sigc::mem_fun(*this, &NotebooksView::on_selection_changed));
    }

    Notebook::ORef NotebooksView::get_selected_notebook() const
    {
      auto selection = std::dynamic_pointer_cast<const Gtk::SingleSelection>(m_list.get_model());
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
      auto model = std::dynamic_pointer_cast<Gtk::SingleSelection>(m_list.get_model());
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
      auto model = std::dynamic_pointer_cast<Gtk::SingleSelection>(m_list.get_model());
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
      auto selection = std::dynamic_pointer_cast<Gtk::SingleSelection>(m_list.get_model());
      DBG_ASSERT(selection, "selection is NULL");
      if(!selection) {
        ERR_OUT("Expected SingleSelection. This is a bug, please, report it!");
        return;
      }
      auto sort_model = std::dynamic_pointer_cast<Gtk::SortListModel>(selection->get_model());
      DBG_ASSERT(sort_model, "sort model is null");
      if(!sort_model) {
        ERR_OUT("Expected sort model. This is a bug, please, rerpot it!");
        return;
      }
      sort_model->set_model(model);
    }

    void NotebooksView::on_selection_changed(guint, guint)
    {
      if(auto notebook = get_selected_notebook()) {
        const Notebook& nb = notebook.value();
        on_selected_notebook_changed(nb);
        signal_selected_notebook_changed(nb);
      }
      else {
        select_all_notes_notebook();
      }
    }

    bool NotebooksView::on_notebooks_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state)
    {
      switch(keyval) {
      case GDK_KEY_F2:
        on_rename_notebook();
        return true;
      }

      return false;
    }

    void NotebooksView::on_selected_notebook_changed(const Notebook& notebook)
    {
      bool allow_edit = nullptr == dynamic_cast<const SpecialNotebook*>(&notebook);
      m_rename_button.set_sensitive(allow_edit);
      m_delete_button.set_sensitive(allow_edit);
    }

    void NotebooksView::on_create_new_notebook()
    {
      auto& popover = NotebookNamePopover::create(m_new_button, m_note_manager.notebook_manager());
      popover.set_position(Gtk::PositionType::BOTTOM);
      popover.popup();
    }

    void NotebooksView::on_rename_notebook()
    {
      auto selected = get_selected_notebook();
      if(!selected) {
        return;
      }
      Notebook & selected_notebook = selected.value();
      if(dynamic_cast<SpecialNotebook*>(&selected_notebook)) {
        return;
      }

      NotebookNamePopover::create(m_rename_button, selected_notebook, sigc::mem_fun(*this, &NotebooksView::rename_notebook)).popup();
    }

    void NotebooksView::rename_notebook(const Notebook& old_notebook, const Glib::ustring& new_name)
    {
      NotebookManager & notebook_manager = m_note_manager.notebook_manager();
      auto & new_notebook = notebook_manager.get_or_create_notebook(new_name);
      DBG_OUT("Renaming notebook '{%s}' to '{%s}'", old_notebook.get_name().c_str(), new_name.c_str());
      if(auto t = old_notebook.get_tag()) {
        Tag &tag = t.value();
        for(NoteBase *note : tag.get_notes()) {
          notebook_manager.move_note_to_notebook(static_cast<Note&>(*note), new_notebook);
        }
      }
      notebook_manager.delete_notebook(const_cast<Notebook&>(old_notebook));
      select_notebook(new_notebook);
    }

    void NotebooksView::on_open_template_note()
    {
      auto notebook = get_selected_notebook();
      if(!notebook) {
        return;
      }

      Notebook& nb = notebook.value();
      if(dynamic_cast<SpecialNotebook*>(&nb)) {
        signal_open_template_note(static_cast<Note&>(m_note_manager.get_or_create_template_note()));
      }
      else {
        signal_open_template_note(nb.get_template_note());
      }
    }

    void NotebooksView::on_delete_notebook()
    {
      auto notebook = get_selected_notebook();
      if(!notebook) {
        return;
      }

      Gtk::Widget* widget = get_parent();
      if(!widget) {
        return;
      }
      while(true) {
        auto parent = widget->get_parent();
        if(!parent) {
          break;
        }
        else {
          widget = parent;
        }
      }

      auto window = dynamic_cast<Gtk::Window*>(widget);
      if(!window) {
        return;
      }

      NotebookManager::prompt_delete_notebook(m_note_manager.gnote(), window, *notebook);
    }
  }
}
