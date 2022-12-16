/*
 * gnote
 *
 * Copyright (C) 2011,2013,2017,2019-2020,2022 Aurimas Cernius
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





#ifndef __PROPERTYEDITOR_HPP_
#define __PROPERTYEDITOR_HPP_

#include <vector>

#include <giomm/settings.h>
#include <gtkmm/entry.h>
#include <gtkmm/checkbutton.h>

namespace sharp {

  template <typename GetterT, typename SetterT>
  class PropertyEditorBase
  {
  public:
    virtual ~PropertyEditorBase() = default;
    virtual void setup() = 0;

  protected:
    PropertyEditorBase(GetterT && getter, SetterT && setter, Gtk::Widget & w);

    Gtk::Widget &m_widget;
    sigc::connection m_connection;
    GetterT m_getter;
    SetterT m_setter;
  private:
    void static destroy_notify(gpointer data);
  };


  typedef std::function<Glib::ustring()> StringPropertyGetterT;
  typedef std::function<void(const Glib::ustring&)> StringPropertySetterT;

  class PropertyEditor
      : public PropertyEditorBase<StringPropertyGetterT, StringPropertySetterT>
  {
  public:
    PropertyEditor(StringPropertyGetterT && getter, StringPropertySetterT && setter, Gtk::Entry &entry);

    virtual void setup() override;

  private:
    void on_changed();
  };


  typedef sigc::slot<bool()> BoolPropertyGetterT;
  typedef sigc::slot<void(bool)> BoolPropertySetterT;

  class PropertyEditorBool
    : public PropertyEditorBase<BoolPropertyGetterT, BoolPropertySetterT>
  {
  public:
    PropertyEditorBool(BoolPropertyGetterT getter, BoolPropertySetterT setter, Gtk::CheckButton &button);
    void add_guard(Gtk::Widget* w)
      {
        m_guarded.push_back(w);
      }

    virtual void setup() override;

  private:
    void guard(bool v);
    void on_changed();
    std::vector<Gtk::Widget*> m_guarded;
  };

}


#endif
