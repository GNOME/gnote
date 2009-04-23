/*
 * gnote
 *
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






#ifndef __PRINTNOTES_NOTEADDIN_HPP_
#define __PRINTNOTES_NOTEADDIN_HPP_

#include <vector>

#include <gtkmm/menuitem.h>
#include <pangomm/layout.h>

#include "sharp/dynamicmodule.hpp"
#include "noteaddin.hpp"

namespace printnotes {


class PrintNotesModule
  : public sharp::DynamicModule
{
public:
  PrintNotesModule();
  virtual const char * id() const;
  virtual const char * name() const;
  virtual const char * description() const;
  virtual const char * authors() const;
  virtual const char * category() const;
  virtual const char * version() const;
};


DECLARE_MODULE(PrintNotesModule);

struct PrintMargins
{
  int top;
  int left;
  int right;
  int bottom;

  PrintMargins()
    : top(0), left(0), right(0), bottom(0)
    {
    }
  void clear()
    {
      top = left = right = bottom = 0;
    }
  int vertical_margins()
    {
      return top + bottom;
    }
  int horizontal_margins()
    {
      return left + right;
    }
};

class PrintNotesNoteAddin
  : public gnote::NoteAddin
{
public:
  static PrintNotesNoteAddin* create()
    {
      return new PrintNotesNoteAddin;
    }
  virtual void initialize();
  virtual void shutdown();
  virtual void on_note_opened();

  static int cm_to_pixel(double cm, double dpi)
		{
			return (int) (cm * dpi / 2.54);
		}

private:
  void get_paragraph_attributes(const Glib::RefPtr<Pango::Layout> & layout,
                                double dpiX, PrintMargins & margins,
                                Gtk::TextIter & position, 
                                Gtk::TextIter & limit,
                                std::list<Pango::Attribute> & attributes);
  Glib::RefPtr<Pango::Layout> create_layout_for_paragraph(const Glib::RefPtr<Gtk::PrintContext> & context, 
                                                          Gtk::TextIter p_start,
                                                          Gtk::TextIter p_end,
                                                          PrintMargins & margins);
  Glib::RefPtr<Pango::Layout> create_layout_for_pagenumbers(const Glib::RefPtr<Gtk::PrintContext> & context, int page_number, int total_pages);
  Glib::RefPtr<Pango::Layout> create_layout_for_timestamp(const Glib::RefPtr<Gtk::PrintContext> & context);
  void on_begin_print(const Glib::RefPtr<Gtk::PrintContext>&);
  void print_footer(const Glib::RefPtr<Gtk::PrintContext>&, guint);

  void on_draw_page(const Glib::RefPtr<Gtk::PrintContext>&, guint);
  void on_end_print(const Glib::RefPtr<Gtk::PrintContext>&);
/////
  void print_button_clicked();
  Gtk::ImageMenuItem * m_item;
  PrintMargins         m_page_margins;
  int                  m_footer_offset;
  std::vector<int>     m_page_breaks;
  Glib::RefPtr<Gtk::PrintOperation> m_print_op;
  std::string          m_timestamp;
};

}

#endif

