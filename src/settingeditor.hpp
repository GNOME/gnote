/*
 * gnote
 *
 * Copyright (C) 2026 Aurimas Cernius
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


#ifndef __SETTING_EDITOR_HPP_
#define __SETTING_EDITOR_HPP_

#include <vector>

#include <giomm/settings.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/entry.h>

#include "preferences.hpp"


namespace gnote {

template <typename T>
class SettingEditorBase
{
public:
  virtual ~SettingEditorBase() = default;

protected:
  explicit SettingEditorBase(Preferences::Setting<T> &setting);

  Preferences::Setting<T> &m_setting;
};


class SettingEditor
  : public Gtk::Entry
  , public SettingEditorBase<Glib::ustring>
{
public:
  explicit SettingEditor(Preferences::Setting<Glib::ustring> &setting);
private:
  void on_changed();
};


class SettingEditorBool
  : public Gtk::CheckButton
  , public SettingEditorBase<bool>
{
public:
  SettingEditorBool(Preferences::Setting<bool> &setting);
  SettingEditorBool(Preferences::Setting<bool> &setting, const Glib::ustring &label, bool mnemonic = false);
  void add_guard(Gtk::Widget &w)
    {
      m_guarded.push_back(w);
    }
private:
  void ctor();
  void guard(bool v);
  void on_changed();
  std::vector<std::reference_wrapper<Gtk::Widget>> m_guarded;
};

}

#endif

