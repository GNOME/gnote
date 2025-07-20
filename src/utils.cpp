/*
 * gnote
 *
 * Copyright (C) 2010-2017,2019-2025 Aurimas Cernius
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

#include <glibmm/i18n.h>
#include <glibmm/stringutils.h>
#include <gtkmm/label.h>
#include <gtkmm/urilauncher.h>

#include "base/monitor.hpp"
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
      gboolean main_context_invoke_func(gpointer data)
      {
        sigc::slot<void()> *slot = static_cast<sigc::slot<void()>*>(data);
        (*slot)();
        delete slot;
        return FALSE;
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

      auto launcher = Gtk::UriLauncher::create(uri);
      launcher->launch(parent, [launcher, &parent](const Glib::RefPtr<Gio::AsyncResult> & ready) {
        try {
          launcher->launch_finish(ready);
        }
        catch(const Glib::Error & error) {
          ERR_OUT(_("Failed to show help: %s"), error.what());
          Glib::ustring message = _("The \"Gnote Manual\" could "
                                    "not be found.  Please verify "
                                    "that your installation has been "
                                    "completed successfully.");
          auto dialog = Gtk::make_managed<HIGMessageDialog>(const_cast<Gtk::Window*>(&parent),
            GTK_DIALOG_DESTROY_WITH_PARENT, Gtk::MessageType::ERROR, Gtk::ButtonsType::OK,
            _("Help not found"), message);
          dialog->show();
          dialog->signal_response().connect([dialog](int) { dialog->hide(); });
        }
      });
    }


    void open_url(Gtk::Window & parent, const Glib::ustring & url)
    {
      if(!url.empty()) {
        DBG_OUT("Opening url '%s'...", url.c_str());
        auto launcher = Gtk::UriLauncher::create(url);
        launcher->launch(parent, [launcher](const Glib::RefPtr<Gio::AsyncResult> & ready) {
          try {
            launcher->launch_finish(ready);
          }
          catch(const Glib::Error & error) {
            ERR_OUT(_("Failed to open URL: %s"), error.what());
          }
        });
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

    // separate function for testing purposes
    Glib::ustring get_pretty_print_date(const Glib::DateTime& date, bool show_time, bool use_12h, const Glib::DateTime& now)
    {
      Glib::ustring short_time = use_12h
        /* TRANSLATORS: time in 12h format. */
        ? sharp::date_time_to_string(date, "%l:%M %P")
        /* TRANSLATORS: time in 24h format. */
        : sharp::date_time_to_string(date, "%H:%M");

      enum class Format
      {
        TODAY,
        TOMORROW,
        YESTERDAY,
        CURRENT_YEAR,
        OTHER_YEAR,
      } format = Format::OTHER_YEAR;

      if(date.get_year() == now.get_year()) {
        if(date.get_day_of_year() == now.get_day_of_year()) {
          format = Format::TODAY;
        }
        else if((date.get_day_of_year() == now.get_day_of_year() - 1)) {
          format = Format::YESTERDAY;
        }
        else if(date.get_day_of_year() == now.get_day_of_year() + 1) {
          format = Format::TOMORROW;
        }
        else {
          format = Format::CURRENT_YEAR;
        }
      }
      else if(date.get_year() + 1 == now.get_year() && date.get_month() == 12 && date.get_day_of_month() == 31
              && now.get_month() == 1 && now.get_day_of_month() == 1) {
        format = Format::YESTERDAY;
      }
      else if(date.get_year() == now.get_year() + 1 && date.get_month() == 1 && date.get_day_of_month() == 1
              && now.get_month() == 12 && now.get_day_of_month() == 31) {
        format = Format::TOMORROW;
      }

      Glib::ustring pretty_str;
      switch(format) {
      case Format::TODAY:
        pretty_str = show_time
          /* TRANSLATORS: argument %1 is time. */
          ? Glib::ustring::compose(_("Today, %1"), short_time)
          : _("Today");
        break;
      case Format::TOMORROW:
        pretty_str = show_time
          /* TRANSLATORS: argument %1 is time. */
          ? Glib::ustring::compose(_("Tomorrow, %1"), short_time)
          : _("Tomorrow");
        break;
      case Format::YESTERDAY:
        pretty_str = show_time
          /* TRANSLATORS: argument %1 is time. */
          ? Glib::ustring::compose(_("Yesterday, %1"), short_time)
          : _("Yesterday");
        break;
      case Format::CURRENT_YEAR:
        /* TRANSLATORS: date in current year. */
        /* xgettext:no-c-format */
        pretty_str = sharp::date_time_to_string(date, _("%b %d")); // "MMMM d"
        goto DATE_AND_TIME;
      case Format::OTHER_YEAR:
        /* TRANSLATORS: date in other than current year. */
        pretty_str = sharp::date_time_to_string(date, _("%b %d %Y")); // "MMMM d yyyy"
        goto DATE_AND_TIME;
      DATE_AND_TIME:
        if(show_time) {
          /* TRANSLATORS: argument %1 is date, %2 is time. */
          pretty_str = Glib::ustring::compose(_("%1, %2"), pretty_str, short_time);
        }
        break;
      }

      return pretty_str;
    }

    Glib::ustring get_pretty_print_date(const Glib::DateTime & date, bool show_time, bool use_12h)
    {
      if(!date) {
        return _("No Date");
      }

      return get_pretty_print_date(date, show_time, use_12h, Glib::DateTime::create_now_local());
    }

    void main_context_invoke(const sigc::slot<void()> & slot)
    {
      auto data = new sigc::slot<void()>(slot);
      g_main_context_invoke(NULL, main_context_invoke_func, data);
    }


    void main_context_call(const sigc::slot<void()> & slot)
    {
      CompletionMonitor cond;
      std::exception_ptr ex;

      {
        CompletionMonitor::WaitLock lock(cond);
        main_context_invoke([slot, &cond, &ex]() {
          CompletionMonitor::NotifyLock lock(cond);
          try {
            slot();
          }
          catch(...) {
            ex = std::current_exception();
          }
        });
      }

      if(ex) {
        std::rethrow_exception(ex);
      }
    }


    void timeout_add_once(guint interval, std::function<void()> func)
    {
      auto f = new std::function<void()>(std::move(func));
      g_timeout_add_once(interval, [](gpointer data) {
        auto func = static_cast<std::function<void()>*>(data);
        (*func)();
        delete func;
      }, f);
    }


    HIGMessageDialog::HIGMessageDialog(Gtk::Window *parent,
                                       GtkDialogFlags flags, Gtk::MessageType msg_type, 
                                       Gtk::ButtonsType btn_type, const Glib::ustring & header,
                                       const Glib::ustring & msg)
      : Gtk::Dialog("", false, true)
      , m_extra_widget_vbox(nullptr)
      , m_extra_widget(NULL)
    {
      set_resizable(false);

      Gtk::Grid *hbox = Gtk::make_managed<Gtk::Grid>();
      hbox->set_column_spacing(12);
      hbox->set_margin(12);
      int hbox_col = 0;
      get_content_area()->append(*hbox);

      Gtk::Grid *label_vbox = Gtk::make_managed<Gtk::Grid>();
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
        label->set_wrap(TRUE);
        label->set_max_width_chars(60);
        label_vbox->attach(*label, 0, label_vbox_row++, 1, 1);
      }

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
        set_modal(true);
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

      if(value && m_extra_widget_vbox == nullptr) {
        m_extra_widget_vbox = Gtk::make_managed<Gtk::Grid>();
        if(auto hbox = get_content_area()->get_first_child()) {
          if(auto label_vbox = dynamic_cast<Gtk::Grid*>(hbox->get_first_child())) {
            label_vbox->attach_next_to(*m_extra_widget_vbox, Gtk::PositionType::BOTTOM, 1, 1);
          }
        }
      }

      m_extra_widget = value;
      m_extra_widget_vbox->attach(*m_extra_widget, 0, 0, 1, 1);
    }


    LabelFactory::LabelFactory()
    {
      signal_setup().connect(sigc::mem_fun(*this, &LabelFactory::on_setup));
      signal_bind().connect(sigc::mem_fun(*this, &LabelFactory::on_bind));
    }

    void LabelFactory::set_text(Gtk::Label & label, const Glib::ustring & text)
    {
      label.set_text(text);
    }

    void LabelFactory::on_setup(const Glib::RefPtr<Gtk::ListItem> & item)
    {
      auto label = Gtk::make_managed<Gtk::Label>();
      label->set_halign(Gtk::Align::START);
      item->set_child(*label);
    }

    void LabelFactory::on_bind(const Glib::RefPtr<Gtk::ListItem> & item)
    {
      auto label = static_cast<Gtk::Label*>(item->get_child());
      set_text(*label, get_text(*item));
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
        case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
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
