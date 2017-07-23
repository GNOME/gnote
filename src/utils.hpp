/*
 * gnote
 *
 * Copyright (C) 2011-2013,2015-2017 Aurimas Cernius
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

#include <gdkmm/pixbuf.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/dialog.h>
#include <gtkmm/grid.h>
#include <gtkmm/image.h>
#include <gtkmm/menu.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/textiter.h>
#include <gtkmm/textmark.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbar.h>

#include "base/macros.hpp"
#include "sharp/exception.hpp"
#include "sharp/uri.hpp"

namespace sharp {
  class DateTime;
}

namespace gnote {
  namespace utils {

    void popup_menu(Gtk::Menu &menu, const GdkEventButton *);

    void show_help(const Glib::ustring & filename, const Glib::ustring & link_id, Gtk::Window & parent);
    void open_url(Gtk::Window & parent, const Glib::ustring & url);
    void show_opening_location_error(Gtk::Window * parent,
                                     const Glib::ustring & url,
                                     const Glib::ustring & error);
    Glib::ustring get_pretty_print_date(const sharp::DateTime &, bool show_time);
    Glib::ustring get_pretty_print_date(const sharp::DateTime &, bool show_time, bool use_12h);

    void main_context_invoke(const sigc::slot<void> & slot);
    void main_context_call(const sigc::slot<void> & slot);

    Gtk::Widget * create_popover_button(const Glib::ustring & action, const Glib::ustring & label);
    Gtk::Widget * create_popover_submenu_button(const Glib::ustring & submenu, const Glib::ustring & label);
    Gtk::Box * create_popover_submenu(const Glib::ustring & name);
    void set_common_popover_widget_props(Gtk::Widget & widget);
    void set_common_popover_widget_props(Gtk::Box & widget);

    void add_item_to_ordered_map(std::map<int, Gtk::Widget*> & dest, int order, Gtk::Widget *item);
    void merge_ordered_maps(std::map<int, Gtk::Widget*> & dest, const std::map<int, Gtk::Widget*> & adds);

    class PopoverSubmenu
    {
    public:
      PopoverSubmenu(const Glib::ustring & name)
        : m_name(name)
      {}

      const Glib::ustring & name() const
        {
          return m_name;
        }
    private:
      const Glib::ustring m_name;
    };

    class GlobalKeybinder
    {
    public:
      
      GlobalKeybinder(const Glib::RefPtr<Gtk::AccelGroup> & accel_group)
        : m_accel_group(accel_group)
        {
          m_fake_menu.set_accel_group(accel_group);
        }

      void add_accelerator(const sigc::slot<void> & , guint, Gdk::ModifierType, 
                           Gtk::AccelFlags);
      void enabled(bool enable);
      bool enabled() const
        {
          return m_fake_menu.get_sensitive();
        }
    private:
      Glib::RefPtr<Gtk::AccelGroup> m_accel_group;
      Gtk::Menu m_fake_menu;
    };


    class HIGMessageDialog
      : public Gtk::Dialog 
    {
    public:
      HIGMessageDialog(Gtk::Window *, GtkDialogFlags flags, Gtk::MessageType msg_type, 
                       Gtk::ButtonsType btn_type, const Glib::ustring & header = Glib::ustring(),
                       const Glib::ustring & msg = Glib::ustring());
      void add_button(const Gtk::BuiltinStockID& stock_id, 
                       Gtk::ResponseType response, bool is_default);
      void add_button(const Glib::RefPtr<Gdk::Pixbuf> & pixbuf, 
                      const Glib::ustring & label_text, 
                      Gtk::ResponseType response, bool is_default);
      void add_button(Gtk::Button *button, Gtk::ResponseType response, bool is_default);
      Gtk::Widget * get_extra_widget() const
        {
          return m_extra_widget;
        }
      void set_extra_widget(Gtk::Widget *);
    private:
      Glib::RefPtr<Gtk::AccelGroup> m_accel_group;
      Gtk::Grid *m_extra_widget_vbox;
      Gtk::Widget *m_extra_widget;
      Gtk::Image *m_image;

    };


    class UriList
      : public std::list<sharp::Uri>
    {
    public:
//      UriList(const NoteList & notes);
      UriList(const Glib::ustring & data);
      UriList(const Gtk::SelectionData & selection);
      Glib::ustring to_string() const;
      void get_local_paths(std::list<Glib::ustring> &) const;
      
    private:
      void load_from_string(const Glib::ustring & data);
      void load_from_string_list(const std::vector<Glib::ustring> & items);
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
      TextRange(const Gtk::TextIter & start, const Gtk::TextIter & end);
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
      void destroy();
      void remove_tag(const Glib::RefPtr<Gtk::TextTag> & tag);
    private:
      Glib::RefPtr<Gtk::TextBuffer> m_buffer;
      Glib::RefPtr<Gtk::TextMark>   m_start_mark;
      Glib::RefPtr<Gtk::TextMark>   m_end_mark;
    };


    class TextTagEnumerator
    {
    public:
      TextTagEnumerator(const Glib::RefPtr<Gtk::TextBuffer> & buffer, 
                        const Glib::RefPtr<Gtk::TextTag> & tag);
      const TextRange & current() const
        {
          return m_range;
        }
      bool move_next();
      void reset()
        {
          m_buffer->move_mark(m_mark, m_buffer->begin());
        }
    private:
      Glib::RefPtr<Gtk::TextBuffer> m_buffer;
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
      sigc::signal<void> signal_timeout;
    private:
      static bool callback(InterruptableTimeout*);
      bool timeout_expired();
      guint m_timeout_id;
    };

    class ToolMenuButton
      : public Gtk::ToggleToolButton
    {
    public:
      ToolMenuButton(Gtk::Widget & widget, Gtk::Menu *menu);
      ToolMenuButton(Gtk::Toolbar& toolbar, 
                     const Gtk::BuiltinStockID& stock_image, 
                     const Glib::ustring & label, Gtk::Menu * menu);
      ToolMenuButton(Gtk::Image& image, 
                     const Glib::ustring & label, Gtk::Menu * menu);
      virtual bool on_button_press_event(GdkEventButton *) override;
      virtual void on_clicked() override;
      virtual bool on_mnemonic_activate(bool group_cycling) override;

    private:
      void _common_init();
      void _common_init(Gtk::Image& image, const Glib::ustring & l);
      // managed by gtkmm
      Gtk::Menu *m_menu;
      void release_button();        
    };

    class CheckAction
      : public Gtk::Action
    {
    public:
      typedef Glib::RefPtr<CheckAction> Ptr;
      static Ptr create(const Glib::ustring & name)
        {
          return Ptr(new CheckAction(name));
        }
      void checked(bool check)
        {
          m_checked = check;
        }
      bool checked() const
        {
          return m_checked;
        }
    protected:
      CheckAction(const Glib::ustring & name);
      virtual Gtk::Widget *create_menu_item_vfunc() override;
      virtual void on_activate() override;
    private:
      bool m_checked;
    };
  }
}

#endif
