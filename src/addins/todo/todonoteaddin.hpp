/*
 * gnote
 *
 * Copyright (C) 2013,2017 Aurimas Cernius
 * Copyright (c) 2009 Romain Tartière <romain@blogreen.org>
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


#ifndef _TODO_NOTE_ADDIN_
#define _TODO_NOTE_ADDIN_

#include "noteaddin.hpp"
#include "sharp/dynamicmodule.hpp"

namespace todo {

class TodoModule
  : public sharp::DynamicModule
{
public:
  TodoModule();
};

DECLARE_MODULE(TodoModule);

class Todo
  : public gnote::NoteAddin
{
public:
  static Todo* create()
    { 
      return new Todo;
    }
  virtual void initialize() override;
  virtual void shutdown() override;
  virtual void on_note_opened() override;
private:
  void on_insert_text(const Gtk::TextIter & pos, const Glib::ustring & text, int bytes);
  void on_delete_range(const Gtk::TextBuffer::iterator & start, const Gtk::TextBuffer::iterator & end);
  void highlight_note();
  void highlight_region(Gtk::TextIter start, Gtk::TextIter end);
  void highlight_region(const Glib::ustring & pattern, Gtk::TextIter start, Gtk::TextIter end);
};

}

#endif
