/*
 * gnote
 *
 * Copyright (C) 2010-2017,2019-2022 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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


#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <condition_variable>

#include <glibmm/i18n.h>
#include <glibmm/stringutils.h>
#include <gtkmm/label.h>
#include <gtkmm/textbuffer.h>

#include "sharp/xmlreader.hpp"
#include "sharp/xmlwriter.hpp"
#include "sharp/string.hpp"
#include "sharp/uri.hpp"
#include "preferences.hpp"
#include "note.hpp"
#include "utils.hpp"
#include "debug.hpp"

namespace gnote {
  namespace utils {

    namespace {
      void deactivate_menu(Gtk::Menu *menu)
      {
        menu->popdown();
        if(menu->get_attach_widget()) {
          menu->get_attach_widget()->set_state_flags(Gtk::STATE_FLAG_NORMAL);
        }
      }

      gboolean main_context_invoke_func(gpointer data)
      {
        sigc::slot<void()> *slot = static_cast<sigc::slot<void()>*>(data);
        (*slot)();
        delete slot;
        return FALSE;
      }
   }


    void popup_menu(Gtk::Menu &menu, const GdkEventButton *ev)
    {
      auto event = (const GdkEvent*)ev;
      menu.signal_deactivate().connect(sigc::bind(&deactivate_menu, &menu));
      if(!menu.get_attach_widget() || !menu.get_attach_widget()->get_window()) {
        menu.popup_at_pointer(event);
      }
      else {
        int x, y;
        menu.get_attach_widget()->get_window()->get_origin(x, y);
        menu.popup_at_rect(menu.get_attach_widget()->get_window(), Gdk::Rectangle(x, y, 0, 0), Gdk::GRAVITY_NORTH_WEST, Gdk::GRAVITY_NORTH_WEST, event);
      }
      if(menu.get_attach_widget()) {
        menu.get_attach_widget()->set_state_flags(Gtk::STATE_FLAG_SELECTED);
      }
    }


    void show_help(const Glib::ustring & filename, const Glib::ustring & link_id, Gtk::Window & parent)
    {
      // "help:" URIs are "help:document[/page][?query][#frag]"
      // See resolve_help_uri () at,
      // https://git.gnome.org/browse/yelp/tree/libyelp/yelp-uri.c#n811
      Glib::ustring uri = "help:" + filename;
      if(!link_id.empty()) {
        uri += "/" + link_id;
      }

      gtk_show_uri_full(parent.gobj(), uri.c_str(), GDK_CURRENT_TIME, nullptr, [](GObject *obj, GAsyncResult *res, gpointer data) {
        auto parent = static_cast<Gtk::Window*>(data);
        GError *error = NULL;
        if(gtk_show_uri_full_finish(parent->gobj(), res, &error)) {
          return;
        }
        if(error) {
          g_error_free(error);
        }
        
        Glib::ustring message = _("The \"Gnote Manual\" could "
                                  "not be found.  Please verify "
                                  "that your installation has been "
                                  "completed successfully.");
        auto dialog = Gtk::make_managed<HIGMessageDialog>(parent,
                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                Gtk::MessageType::ERROR,
                                Gtk::ButtonsType::OK,
                                _("Help not found"),
                                message);
        dialog->show();
        dialog->signal_response().connect([dialog](int) { dialog->hide(); });
      }, &parent);
    }


    void open_url(Gtk::Window & parent, const Glib::ustring & url)
    {
      if(!url.empty()) {
        DBG_OUT("Opening url '%s'...", url.c_str());
        gtk_show_uri(parent.gobj(), url.c_str(), GDK_CURRENT_TIME);
      }
    }


    void show_opening_location_error(Gtk::Window * parent,
                                     const Glib::ustring & url,
                                     const Glib::ustring & error)
    {
      Glib::ustring message = Glib::ustring::compose("%1: %2", url, error);

      auto dialog = Gtk::make_managed<HIGMessageDialog>(parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                              Gtk::MessageType::INFO,
                              Gtk::ButtonsType::OK,
                              _("Cannot open location"),
                              message);
      dialog->show();
      dialog->signal_response().connect([dialog](int) { dialog->hide(); });
    }

    Glib::ustring get_pretty_print_date(const Glib::DateTime & date, bool show_time, Preferences & preferences)
    {
      bool use_12h = false;
      if(show_time) {
        use_12h = preferences.desktop_gnome_clock_format() == "12h";
      }
      return get_pretty_print_date(date, show_time, use_12h);
    }

    Glib::ustring get_pretty_print_date(const Glib::DateTime & date, bool show_time, bool use_12h)
    {
      if(!date) {
        return _("No Date");
      }

      Glib::ustring pretty_str;
      auto now = Glib::DateTime::create_now_local();
      Glib::ustring short_time = use_12h
        /* TRANSLATORS: time in 12h format. */
        ? sharp::date_time_to_string(date, "%l:%M %P")
        /* TRANSLATORS: time in 24h format. */
        : sharp::date_time_to_string(date, "%H:%M");

      if(date.get_year() == now.get_year()) {
        if(date.get_day_of_year() == now.get_day_of_year()) {
          pretty_str = show_time ?
            /* TRANSLATORS: argument %1 is time. */
            Glib::ustring::compose(_("Today, %1"), short_time) :
            _("Today");
        }
        else if((date.get_day_of_year() < now.get_day_of_year())
                 && (date.get_day_of_year() == now.get_day_of_year() - 1)) {
          pretty_str = show_time ?
            /* TRANSLATORS: argument %1 is time. */
            Glib::ustring::compose(_("Yesterday, %1"), short_time) :
            _("Yesterday");
        }
        else if(date.get_day_of_year() > now.get_day_of_year()
                 && date.get_day_of_year() == now.get_day_of_year() + 1) {
          pretty_str = show_time ?
            /* TRANSLATORS: argument %1 is time. */
            Glib::ustring::compose(_("Tomorrow, %1"), short_time) :
            _("Tomorrow");
        }
        else {
          /* TRANSLATORS: date in current year. */
          pretty_str = sharp::date_time_to_string(date, _("%b %d")); // "MMMM d"
          if(show_time) {
            /* TRANSLATORS: argument %1 is date, %2 is time. */
            pretty_str = Glib::ustring::compose(_("%1, %2"), pretty_str, short_time);
          }
        }
      } 
      else {
        /* TRANSLATORS: date in other than current year. */
        pretty_str = sharp::date_time_to_string(date, _("%b %d %Y")); // "MMMM d yyyy"
        if(show_time) {
          /* TRANSLATORS: argument %1 is date, %2 is time. */
          pretty_str = Glib::ustring::compose(_("%1, %2"), pretty_str, short_time);
        }
      }

      return pretty_str;
    }

    void main_context_invoke(const sigc::slot<void()> & slot)
    {
      auto data = new sigc::slot<void()>(slot);
      g_main_context_invoke(NULL, main_context_invoke_func, data);
    }


    void main_context_call(const sigc::slot<void()> & slot)
    {
      std::mutex mutex;
      std::condition_variable cond;
      bool executed = false;
      std::exception_ptr ex;

      std::unique_lock<std::mutex> lock(mutex);
      main_context_invoke([slot, &cond, &mutex, &executed, &ex]() {
        std::unique_lock<std::mutex> lock(mutex);
        try {
          slot();
        }
        catch(...) {
          ex = std::current_exception();
        }
        executed = true;
        cond.notify_one();
      });
      while(!executed) {
        cond.wait(lock);
      }
      if(ex) {
        std::rethrow_exception(ex);
      }
    }


    void* GlobalKeybinder::add_accelerator(const sigc::slot<void> & handler, guint key,
                                          Gdk::ModifierType modifiers, Gtk::AccelFlags flags)
    {
      Gtk::MenuItem *foo = manage(new Gtk::MenuItem ());
      foo->signal_activate().connect(handler);
      foo->add_accelerator ("activate",
                          m_accel_group,
                          key,
                          modifiers,
                          flags);
      foo->show ();
      foo->set_sensitive(m_fake_menu.get_sensitive());
      m_fake_menu.append (*foo);
      return foo;
    }

    void GlobalKeybinder::remove_accelerator(void *accel)
    {
      auto widget = static_cast<Gtk::Widget*>(accel);
      m_fake_menu.remove(*widget);
      delete widget;
    }

    void GlobalKeybinder::enabled(bool enable)
    {
      m_fake_menu.set_sensitive(enable);
      std::vector<Gtk::Widget*> items = m_fake_menu.get_children();
      for(Gtk::Widget *item : items) {
        item->set_sensitive(enable);
      }
    }


    HIGMessageDialog::HIGMessageDialog(Gtk::Window *parent,
                                       GtkDialogFlags flags, Gtk::MessageType msg_type, 
                                       Gtk::ButtonsType btn_type, const Glib::ustring & header,
                                       const Glib::ustring & msg)
      : Gtk::Dialog()
      , m_extra_widget(NULL)
    {
      set_margin(5);
      set_resizable(false);
      set_title("");

      get_content_area()->set_spacing(12);

      Gtk::Grid *hbox = Gtk::make_managed<Gtk::Grid>();
      hbox->set_column_spacing(12);
      hbox->set_margin(5);
      int hbox_col = 0;
      get_content_area()->append(*hbox);

      Gtk::Grid *label_vbox = Gtk::make_managed<Gtk::Grid>();
      label_vbox->show();
      int label_vbox_row = 0;
      label_vbox->set_hexpand(true);
      hbox->attach(*label_vbox, hbox_col++, 0, 1, 1);

      if(header != "") {
        Glib::ustring title = Glib::ustring::compose("<span weight='bold' size='larger'>%1</span>\n", header);
        Gtk::Label *label = Gtk::make_managed<Gtk::Label>(title);
        label->set_use_markup(true);
        label->set_justify(Gtk::Justification::LEFT);
        label->set_halign(Gtk::Align::START);
        label->set_valign(Gtk::Align::CENTER);
        label_vbox->attach(*label, 0, label_vbox_row++, 1, 1);
      }

      if(msg != "") {
        Gtk::Label *label = Gtk::make_managed<Gtk::Label>(msg);
        label->set_use_markup(true);
        label->set_justify(Gtk::Justification::LEFT);
        label->set_halign(Gtk::Align::START);
        label->set_valign(Gtk::Align::CENTER);
        label_vbox->attach(*label, 0, label_vbox_row++, 1, 1);
      }
      
      m_extra_widget_vbox = Gtk::make_managed<Gtk::Grid>();
      m_extra_widget_vbox->set_margin_start(12);
      label_vbox->attach(*m_extra_widget_vbox, 0, label_vbox_row++, 1, 1);

      switch(btn_type) {
      case Gtk::ButtonsType::NONE:
        break;
      case Gtk::ButtonsType::OK:
        add_button(_("_OK"), Gtk::ResponseType::OK, true);
        break;
      case Gtk::ButtonsType::CLOSE:
        add_button(_("_Close"), Gtk::ResponseType::CLOSE, true);
        break;
      case Gtk::ButtonsType::CANCEL:
        add_button(_("_Cancel"), Gtk::ResponseType::CANCEL, true);
        break;
      case Gtk::ButtonsType::YES_NO:
        add_button(_("_No"), Gtk::ResponseType::NO, false);
        add_button(_("_Yes"), Gtk::ResponseType::YES, true);
        break;
      case Gtk::ButtonsType::OK_CANCEL:
        add_button(_("_Cancel"), Gtk::ResponseType::CANCEL, false);
        add_button(_("_OK"), Gtk::ResponseType::OK, true);
        break;
      }

      if (parent){
        set_transient_for(*parent);
      }

      if ((flags & GTK_DIALOG_DESTROY_WITH_PARENT) != 0) {
        property_destroy_with_parent().set_value(true);
      }
    }


    void HIGMessageDialog::add_button(const Glib::ustring & label, Gtk::ResponseType resp, bool is_default)
    {
      Gtk::Button *button = Gtk::make_managed<Gtk::Button>(label, true);
      add_button(button, resp, is_default);
    }

    void HIGMessageDialog::add_button(Gtk::Button *button, Gtk::ResponseType resp, bool is_default)
    {
      add_action_widget(*button, resp);

      if (is_default) {
        set_default_response(resp);
      }
    }


    void HIGMessageDialog::set_extra_widget(Gtk::Widget *value)
    {
      if (m_extra_widget) {
          m_extra_widget_vbox->remove(*m_extra_widget);
      }

      m_extra_widget = value;
      m_extra_widget_vbox->attach(*m_extra_widget, 0, 0, 1, 1);
    }


#if 0
    UriList::UriList(const NoteList & notes)
    {
      foreach(const Note::Ptr & note, notes) {
        push_back(sharp::Uri(note->uri()));
      }
    }
#endif

    void UriList::load_from_string(const Glib::ustring & data)
    {
      std::vector<Glib::ustring> items;
      sharp::string_split(items, data, "\n");
      load_from_string_list(items);
    }

    void UriList::load_from_string_list(const std::vector<Glib::ustring> & items)
    {
      for(const Glib::ustring & i : items) {
        if(Glib::str_has_prefix(i, "#")) {
          continue;
        }

        Glib::ustring s = i;
        if(Glib::str_has_suffix(i, "\r")) {
          s.resize(s.size() - 1);
        }

        // Handle evo's broken file urls
        if(Glib::str_has_prefix(s, "file:////")) {
          s = sharp::string_replace_first(s, "file:////", "file:///");
        }
        DBG_OUT("uri = %s", s.c_str());
        push_back(sharp::Uri(std::move(s)));
      }
    }

    UriList::UriList(const Glib::ustring & data)
    {
      load_from_string(data);
    }

    
    UriList::UriList(const Gtk::SelectionData & selection)
    {
      if(selection.get_length() > 0) {
        load_from_string_list(selection.get_uris());
      }
    }


    Glib::ustring UriList::to_string() const
    {
      Glib::ustring s;
      for(const_iterator iter = begin(); iter != end(); ++iter) {
        s += iter->to_string() + "\r\n";
      }
      return s;
    }


    std::vector<Glib::ustring> UriList::get_local_paths() const
    {
      std::vector<Glib::ustring> paths;
      for(const_iterator iter = begin(); iter != end(); ++iter) {

        const sharp::Uri & uri(*iter);

        if(uri.is_file()) {
          paths.push_back(uri.local_path());
        }
      }

      return paths;
    }


    Glib::ustring XmlEncoder::encode(const Glib::ustring & source)
    {
      sharp::XmlWriter xml;
      //need element so that source is properly escaped
      xml.write_start_element("", "x", "");
      xml.write_string(source);
      xml.write_end_element();

      xml.close();
      Glib::ustring result = xml.to_string();
      Glib::ustring::size_type end_pos = result.find("</x>");
      if(end_pos == result.npos) {
        return "";
      }
      result.resize(end_pos);
      return result.substr(3);
    }


    Glib::ustring XmlDecoder::decode(const Glib::ustring & source)
    {
      Glib::ustring builder;

      sharp::XmlReader xml;
      xml.load_buffer(source);

      while (xml.read ()) {
        switch (xml.get_node_type()) {
        case XML_READER_TYPE_TEXT:
        case XML_READER_TYPE_WHITESPACE:
          builder += xml.get_value();
          break;
        default:
          break;
        }
      }

      xml.close ();

      return builder;
    }


    TextRange::TextRange()
    {
    }


    TextRange::TextRange(const Gtk::TextIter & _start, const Gtk::TextIter & _end)
    {
      if(_start.get_buffer() != _end.get_buffer()) {
        throw(sharp::Exception("Start buffer and end buffer do not match"));
      }
      m_buffer = _start.get_buffer();
      m_start_mark = m_buffer->create_mark(_start, true);
      m_end_mark = m_buffer->create_mark(_end, true);
    }

    Gtk::TextIter TextRange::start() const
    {
      return m_buffer->get_iter_at_mark(m_start_mark);
    }


    void TextRange::set_start(const Gtk::TextIter & value)
    {
      m_buffer->move_mark(m_start_mark, value);
    }

    Gtk::TextIter TextRange::end() const
    {
      return m_buffer->get_iter_at_mark(m_end_mark);
    }

    void TextRange::set_end(const Gtk::TextIter & value)
    {
      m_buffer->move_mark(m_end_mark, value);
    }

    void TextRange::erase()
    {
      Gtk::TextIter start_iter = start();
      Gtk::TextIter end_iter = end();
      m_buffer->erase(start_iter, end_iter);
    }

    void TextRange::destroy()
    {
      m_buffer->delete_mark(m_start_mark);
      m_buffer->delete_mark(m_end_mark);
    }

    void TextRange::remove_tag(const Glib::RefPtr<Gtk::TextTag> & tag)
    {
      m_buffer->remove_tag(tag, start(), end());
    }


    TextTagEnumerator::TextTagEnumerator(const Glib::RefPtr<Gtk::TextBuffer> & buffer, 
                                         const Glib::RefPtr<Gtk::TextTag> & tag)
      : m_buffer(buffer)
      , m_tag(tag)
      , m_mark(buffer->create_mark(buffer->begin(), true))
      , m_range(buffer->begin(), buffer->begin())
    {
    }

    bool TextTagEnumerator::move_next()
    {
      Gtk::TextIter iter = m_buffer->get_iter_at_mark(m_mark);

      if (iter == m_buffer->end()) {
        m_range.destroy();
        m_buffer->delete_mark(m_mark);
        return false;
      }

      if (!iter.forward_to_tag_toggle(m_tag)) {
        m_range.destroy();
        m_buffer->delete_mark(m_mark);
        return false;
      }

      if(!iter.starts_tag(m_tag)) {
        m_buffer->move_mark(m_mark, iter);
        return move_next();
      }

      m_range.set_start(iter);

      if (!iter.forward_to_tag_toggle(m_tag)) {
        m_range.destroy();
        m_buffer->delete_mark(m_mark);
        return false;
      }

      if (!iter.ends_tag(m_tag)) {
        m_buffer->move_mark(m_mark, iter);
        return move_next();
      }

      m_range.set_end(iter);

      m_buffer->move_mark(m_mark, iter);

      return true;
    }

    

    InterruptableTimeout::~InterruptableTimeout()
    {
      cancel();
    }


    int InterruptableTimeout::callback(InterruptableTimeout* self)
    {
      if(self)
        return self->timeout_expired();
      return false;
    }

    void InterruptableTimeout::reset(guint timeout_millis)
    {
      cancel();
      m_timeout_id = g_timeout_add(timeout_millis, (GSourceFunc)callback, this);
    }

    void InterruptableTimeout::cancel()
    {
      if(m_timeout_id != 0) {
        g_source_remove(m_timeout_id);
        m_timeout_id = 0;
      }
    }

    bool InterruptableTimeout::timeout_expired()
    {
      signal_timeout();
      m_timeout_id = 0;
      return false;
    }

  }
}
