/*
 * gnote
 *
 * Copyright (C) 2011,2017,2020,2022 Aurimas Cernius
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



#include <sigc++/functors/ptr_fun.h>

#include "propertyeditor.hpp"


namespace sharp {


  template <typename GetterT, typename SetterT>
  PropertyEditorBase<GetterT, SetterT>::PropertyEditorBase(GetterT && getter, SetterT && setter, Gtk::Widget & w)
    : m_widget(w)
    , m_getter(std::move(getter))
    , m_setter(std::move(setter))
  {
    w.set_data(Glib::Quark("sharp::property-editor"), (gpointer)this,
               &PropertyEditorBase::destroy_notify);
  }

  template <typename GetterT, typename SetterT>
  void PropertyEditorBase<GetterT, SetterT>::destroy_notify(gpointer data)
  {
    PropertyEditorBase * self = (PropertyEditorBase*)data;
    delete self;
  }


  PropertyEditor::PropertyEditor(StringPropertyGetterT && getter, StringPropertySetterT && setter, Gtk::Entry &entry)
    : PropertyEditorBase(std::move(getter), std::move(setter), entry)
  {
    m_connection = entry.property_text().signal_changed().connect(
      sigc::mem_fun(*this, &PropertyEditor::on_changed));
  }

  void PropertyEditor::setup()
  {
    m_connection.block();
    static_cast<Gtk::Entry &>(m_widget).set_text(m_getter());
    m_connection.unblock();        
  }

  void PropertyEditor::on_changed()
  {
    Glib::ustring txt = static_cast<Gtk::Entry &>(m_widget).get_text();
    m_setter(txt);
  }


  PropertyEditorBool::PropertyEditorBool(BoolPropertyGetterT getter, BoolPropertySetterT setter, Gtk::CheckButton &button)
    : PropertyEditorBase(std::move(getter), std::move(setter), button)
  {
    m_connection = button.property_active().signal_changed().connect(
      sigc::mem_fun(*this, &PropertyEditorBool::on_changed));
  }

  void PropertyEditorBool::guard(bool v)
  {
    for(std::vector<Gtk::Widget*>::iterator iter = m_guarded.begin();
        iter != m_guarded.end(); ++iter) {
      (*iter)->set_sensitive(v);
    }
  }


  void PropertyEditorBool::setup()
  {
    m_connection.block();
    static_cast<Gtk::CheckButton &>(m_widget).set_active(m_getter());
    m_connection.unblock();        
  }

  void PropertyEditorBool::on_changed()
  {
    bool active = static_cast<Gtk::CheckButton &>(m_widget).get_active();
    m_setter(active);
    guard(active);
  }

}

