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

#include "debug.hpp"
#include "notebooks/notebook.hpp"
#include "notebooks/notebookmanager.hpp"
#include "notebooks/notebooksview.hpp"
#include "notebooks/specialnotebooks.hpp"
#include "notemanagerbase.hpp"

namespace gnote {
  namespace notebooks {

    NotebooksTreeView::NotebooksTreeView(NoteManagerBase & manager, const Glib::RefPtr<Gtk::TreeModel> & model)
      : Gtk::TreeView(model)
      , m_note_manager(manager)
    {
      auto drop_target = Gtk::DropTarget::create(G_TYPE_INVALID, Gdk::DragAction::COPY);
      std::vector<GType> types;
      types.push_back(Glib::Value<Glib::ustring>::value_type());
      types.push_back(Glib::Value<std::vector<Glib::ustring>>::value_type());
      drop_target->set_gtypes(types);
      drop_target->signal_drop().connect(sigc::mem_fun(*this, &NotebooksTreeView::on_drag_data_received), false);
      add_controller(drop_target);
      // TODO: add some visual when hovering over target notebook
    }

    bool NotebooksTreeView::on_drag_data_received(const Glib::ValueBase & value, double x, double y)
    {
      Gtk::TreePath treepath;
      Gtk::TreeView::DropPosition pos;
      if(get_dest_row_at_pos(x, y, treepath, pos) == false) {
        return false;
      }
      
      Gtk::TreeIter iter = get_model()->get_iter(treepath);
      if(!iter) {
        return false;
      }
      
      Notebook::Ptr destNotebook;
      iter->get_value(0, destNotebook);
      if(std::dynamic_pointer_cast<AllNotesNotebook>(destNotebook)) {
        return false;
      }

      auto drop = [this, &destNotebook](const Glib::ustring & uri) {
        NoteBase::Ptr note = m_note_manager.find_by_uri(uri);
        if(!note) {
          return false;
        }

        DBG_OUT ("Dropped into notebook: %s", note->get_title().c_str());
        destNotebook->add_note(std::static_pointer_cast<Note>(note));
        return true;
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
  }
}
