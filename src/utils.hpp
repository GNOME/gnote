/*
 * gnote
 *
 * Copyright (C) 2011-2013,2015-2017,2019-2023,2026 Aurimas Cernius
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



#ifndef _GNOTE_UTILS_HPP__
#define _GNOTE_UTILS_HPP__

#include <sigc++/signal.h>

#include <glibmm/datetime.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/textiter.h>
#include <gtkmm/textmark.h>

#include "sharp/exception.hpp"
#include "sharp/uri.hpp"


namespace gnote {

class Preferences;


  namespace utils {

    void show_help(const Glib::ustring & filename, const Glib::ustring & link_id, Gtk::Window & parent);
    void open_url(Gtk::Window & parent, const Glib::ustring & url);
    void show_opening_location_error(Gtk::Window * parent,
                                     const Glib::ustring & url,
                                     const Glib::ustring & error);
    Glib::ustring get_pretty_print_date(const Glib::DateTime &, bool show_time, Preferences & preferences);
    Glib::ustring get_pretty_print_date(const Glib::DateTime &, bool show_time, bool use_12h);

    void main_context_invoke(const sigc::slot<void()> & slot);
    void main_context_call(const sigc::slot<void()> & slot);
    void timeout_add_once(guint interval, std::function<void()> func);

    template <typename T>
    bool remove_swap_back(std::vector<T> & v, const T & e)
    {
      for(typename std::vector<T>::iterator iter = v.begin(); iter != v.end(); ++iter) {
        if(*iter == e) {
          *iter = v.back();
          v.pop_back();
          return true;
        }
      }

      return false;
    }


    class HIGMessageDialog
      : public Gtk::Dialog 
    {
    public:
      HIGMessageDialog(Gtk::Window *, GtkDialogFlags flags, Gtk::MessageType msg_type, 
                       Gtk::ButtonsType btn_type, const Glib::ustring & header = Glib::ustring(),
                       const Glib::ustring & msg = Glib::ustring());
      void add_button(const Glib::ustring & label, Gtk::ResponseType response, bool is_default);
      void add_button(Gtk::Button *button, Gtk::ResponseType response, bool is_default);
      Gtk::Widget * get_extra_widget() const
        {
          return m_extra_widget;
        }
      void set_extra_widget(Gtk::Widget *);
    private:
      Gtk::Grid *m_extra_widget_vbox;
      Gtk::Widget *m_extra_widget;
    };


    template <typename T>
    class ModelRecord
      : public Glib::Object
    {
    public:
      static Glib::RefPtr<ModelRecord> create(const T & value)
        {
          return Glib::make_refptr_for_instance(new ModelRecord(value));
        }

      const T value;
    private:
      explicit ModelRecord(const T & value)
        : value(value)
      {}
    };


    class LabelFactory
      : public Gtk::SignalListItemFactory
    {
    public:
      LabelFactory();
    protected:
      virtual Glib::ustring get_text(Gtk::ListItem & item) = 0;
      virtual void set_text(Gtk::Label & label, const Glib::ustring & text);
    private:
      void on_setup(const Glib::RefPtr<Gtk::ListItem> & item);
      void on_bind(const Glib::RefPtr<Gtk::ListItem> & item);
    };


    class XmlEncoder
    {
    public:
      static Glib::ustring encode(const Glib::ustring & source);
    };

    class XmlDecoder
    {
    public:
      static Glib::ustring decode(const Glib::ustring & source);

    };


    class TextRange
    {
    public:
      TextRange();
      TextRange(const Gtk::TextIter &start, const Gtk::TextIter &end);
      TextRange(const TextRange &other);
      TextRange(TextRange &&other);
      const TextRange &operator=(const TextRange &other);
      const TextRange &operator=(TextRange &&other);
      ~TextRange();
      const Glib::RefPtr<Gtk::TextBuffer> & buffer() const
        {
          return m_buffer;
        }
      const Glib::ustring text() const
        {
          return start().get_text(end());
        }
      int length() const
        {
          return text().size();
        }
      Gtk::TextIter start() const;
      void set_start(const Gtk::TextIter &);
      Gtk::TextIter end() const;
      void set_end(const Gtk::TextIter &);  
      void erase();
      void remove_tag(const Glib::RefPtr<Gtk::TextTag> & tag);
    private:
      void copy_from(const TextRange &other);
      void move_from(TextRange &&other);

      Glib::RefPtr<Gtk::TextBuffer> m_buffer;
      Glib::RefPtr<Gtk::TextMark>   m_start_mark;
      Glib::RefPtr<Gtk::TextMark>   m_end_mark;
    };


    class TextTagEnumerator
    {
    public:
      class iterator
      {
      public:
        friend TextTagEnumerator;

        [[nodiscard]] bool operator==(const iterator &other) const;
        [[nodiscard]] bool operator!=(const iterator &other) const;
        [[nodiscard]] const TextRange &operator*() const;
        [[nodiscard]] const TextRange *operator->() const;
        iterator &operator++();
        [[nodiscard]] iterator operator++(int);
      private:
        iterator(TextTagEnumerator&, bool end);

        TextTagEnumerator &m_enumerator;
        bool m_end;
      };

      TextTagEnumerator(Gtk::TextBuffer &buffer, const Glib::RefPtr<Gtk::TextTag> &tag);
      ~TextTagEnumerator();
      iterator begin();
      iterator end();
    private:
      bool move_next();

      Gtk::TextBuffer &m_buffer;
      Glib::RefPtr<Gtk::TextTag>    m_tag;
      Glib::RefPtr<Gtk::TextMark>   m_mark;
      TextRange                     m_range;
    };


    class InterruptableTimeout
    {
    public:
      InterruptableTimeout()
        : m_timeout_id(0)
        {
        }
      ~InterruptableTimeout();
      void reset(guint timeout_millis);
      void cancel();
      sigc::signal<void()> signal_timeout;
    private:
      static int callback(InterruptableTimeout*);
      bool timeout_expired();
      guint m_timeout_id;
    };
  }
}

#endif
