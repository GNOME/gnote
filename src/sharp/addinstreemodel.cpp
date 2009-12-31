/*
 * gnote
 *
 * Copyright (C) 2009 Hubert Figuiere
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */



#include <glibmm/i18n.h>

#include "sharp/addinstreemodel.hpp"



namespace sharp {


  AddinsTreeModel::Ptr AddinsTreeModel::create(Gtk::TreeView * treeview)
  {
    AddinsTreeModel::Ptr p(new AddinsTreeModel());
    if(treeview) {
      treeview->set_model(p);
      p->set_columns(treeview);
    }
    return p;
  }

  AddinsTreeModel::AddinsTreeModel()
    : Gtk::TreeStore()
  {
    set_column_types(m_columns);
  }

  sharp::DynamicModule * AddinsTreeModel::get_module(const Gtk::TreeIter & iter)
  {
    sharp::DynamicModule * module = NULL;
    if(iter) {
      iter->get_value(2, module);
    }
    return module;
  }


  void AddinsTreeModel::set_columns(Gtk::TreeView *treeview)
  {
      treeview->append_column(_("Name"), m_columns.name);
      treeview->append_column(_("Description"), m_columns.description);
  }


  Gtk::TreeIter AddinsTreeModel::append(const sharp::DynamicModule *module)
  {
    Gtk::TreeIter iter = Gtk::TreeStore::append();
    iter->set_value(0, std::string(module->name()));
    iter->set_value(1, std::string(module->description()));
    iter->set_value(2, module);
    return iter;
  }



}
