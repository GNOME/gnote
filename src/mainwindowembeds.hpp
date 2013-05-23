/*
 * gnote
 *
 * Copyright (C) 2013 Aurimas Cernius
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


#ifndef _MAINWINDOWEMBEDS_HPP_
#define _MAINWINDOWEMBEDS_HPP_


#include <gtkmm/widget.h>


namespace gnote {

class EmbeddableWidget;
class EmbeddableWidgetHost
{
public:
  virtual void embed_widget(EmbeddableWidget &) = 0;
  virtual void unembed_widget(EmbeddableWidget &) = 0;
  virtual void foreground_embedded(EmbeddableWidget &) = 0;
  virtual void background_embedded(EmbeddableWidget &) = 0;
  virtual bool running() = 0;
};

class EmbeddableWidget
{
public:
  EmbeddableWidget() : m_host(NULL) {}
  virtual std::string get_name() const = 0;
  virtual void embed(EmbeddableWidgetHost *h);
  virtual void unembed();
  virtual void foreground();
  virtual void background();
  virtual void hint_position(int & x, int & y);
  virtual void hint_size(int & width, int & height);
  virtual void size_internals();
  EmbeddableWidgetHost *host() const
    {
      return m_host;
    }

  sigc::signal<void, const std::string &> signal_name_changed;
  sigc::signal<void> signal_embedded;
  sigc::signal<void> signal_unembedded;
  sigc::signal<void> signal_foregrounded;
  sigc::signal<void> signal_backgrounded;
private:
  EmbeddableWidgetHost *m_host;
};


class SearchableItem
{
public:
  virtual void perform_search(const std::string & search_text) = 0;
  virtual bool supports_goto_result();
  virtual bool goto_next_result();
  virtual bool goto_previous_result();
};


class HasEmbeddableToolbar
{
public:
  virtual Gtk::Widget *embeddable_toolbar() = 0;
};

}


#endif

