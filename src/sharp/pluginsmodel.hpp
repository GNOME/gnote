/*
 * gnote
 *
 * Copyright (C) 2010,2012-2013,2017,2019,2022-2023 Aurimas Cernius
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




#ifndef __SHARP_ADDINSTREEMODEL_HPP_
#define __SHARP_ADDINSTREEMODEL_HPP_

#include <giomm/liststore.h>
#include <gtkmm/columnview.h>

#include "addininfo.hpp"
#include "sharp/dynamicmodule.hpp"

namespace sharp {

class Plugin
  : public Glib::Object
{
public:
  static Glib::RefPtr<Plugin> create(const gnote::AddinInfo &, const sharp::DynamicModule *);

  const gnote::AddinInfo info;
  const DynamicModule* module() const
    {
      return m_module;
    }
  void set_module(DynamicModule *mod);
private:
  Plugin(const gnote::AddinInfo &, const sharp::DynamicModule *);

  const DynamicModule* m_module;
};

class AddinsModel
  : public Gio::ListStore<Plugin>
{
public:
  typedef Glib::RefPtr<AddinsModel> Ptr;
  static Ptr create(Gtk::ColumnView *view);

  Glib::RefPtr<Plugin> get_selected_plugin();

  void append(const gnote::AddinInfo &, const sharp::DynamicModule *);

  sigc::signal<void(const Glib::RefPtr<Plugin>&)> signal_selection_changed;

  static Glib::ustring get_addin_category_name(gnote::AddinCategory category);
protected:
  void set_columns(Gtk::ColumnView *v);
private:
  AddinsModel();
  void on_selection_changed(guint, guint);

  Glib::RefPtr<Gtk::SelectionModel> m_selection;
};

}


#endif
