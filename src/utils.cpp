/*
 * gnote
 *
 * Copyright (C) 2010-2017 Aurimas Cernius
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

#include <gtk/gtk.h>

#include <glibmm/i18n.h>
#include <glibmm/stringutils.h>
#include <glibmm/threads.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/modelbutton.h>
#include <gtkmm/stock.h>
#include <gtkmm/textbuffer.h>

#include "sharp/xmlreader.hpp"
#include "sharp/xmlwriter.hpp"
#include "sharp/string.hpp"
#include "sharp/uri.hpp"
#include "sharp/datetime.hpp"
#include "preferences.hpp"
#include "note.hpp"
#include "utils.hpp"
#include "debug.hpp"

namespace gnote {
  namespace utils {

    namespace {
      void get_menu_position (Gtk::Menu * menu,
                              int & x,
                              int & y,
                              bool & push_in)
      {
        if (!menu->get_attach_widget() || !menu->get_attach_widget()->get_window()) {
          // Prevent null exception in weird cases
          x = 0;
          y = 0;
          push_in = true;
          return;
        }
      
        menu->get_attach_widget()->get_window()->get_origin(x, y);
        Gdk::Rectangle rect = menu->get_attach_widget()->get_allocation();
        x += rect.get_x();
        y += rect.get_y();
        
        Gtk::Requisition menu_req, unused;
        menu->get_preferred_size(unused, menu_req);
        if (y + menu_req.height >= menu->get_attach_widget()->get_screen()->get_height()) {
          y -= menu_req.height;
        }
        else {
          y += rect.get_height();
        }
        
        push_in = true;
      }


      void deactivate_menu(Gtk::Menu *menu)
      {
        menu->popdown();
        if(menu->get_attach_widget()) {
          menu->get_attach_widget()->set_state(Gtk::STATE_NORMAL);
        }
      }

      gboolean main_context_invoke_func(gpointer data)
      {
        sigc::slot<void> *slot = static_cast<sigc::slot<void>*>(data);
        (*slot)();
        delete slot;
        return FALSE;
      }

      void main_context_call_func(const sigc::slot<void> & slot,
                                  Glib::Threads::Cond * cond,
                                  Glib::Threads::Mutex * mutex)
      {
        mutex->lock();
        slot();
        cond->signal();
        mutex->unlock();
      }


      class PopoverSubmenuBox
        : public Gtk::Box
        , public PopoverSubmenu
      {
      public:
        PopoverSubmenuBox(const Glib::ustring & submenu)
          : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
          , PopoverSubmenu(submenu)
        {
          utils::set_common_popover_widget_props(*this);
        }
      };

      void set_common_popover_widget_props(Gtk::ModelButton & button)
      {
        button.set_use_underline(true);
        button.property_margin_top() = 3;
        button.property_margin_bottom() = 3;
        auto lbl = dynamic_cast<Gtk::Label*>(button.get_child());
        if(lbl) {
          lbl->set_xalign(0.0f);
        }
        utils::set_common_popover_widget_props(button);
      }
    }


    void popup_menu(Gtk::Menu &menu, const GdkEventButton * ev)
    {
      menu.signal_deactivate().connect(sigc::bind(&deactivate_menu, &menu));
      menu.popup([&menu](int & x, int & y, bool & push_in) {
                   get_menu_position(&menu, x, y, push_in);
                  },
                 (ev ? ev->button : 0), 
                 (ev ? ev->time : gtk_get_current_event_time()));
      if(menu.get_attach_widget()) {
        menu.get_attach_widget()->set_state(Gtk::STATE_SELECTED);
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
      GError *error = NULL;

      if(!gtk_show_uri_on_window(parent.gobj(), uri.c_str(), gtk_get_current_event_time (), &error)) {
        
        Glib::ustring message = _("The \"Gnote Manual\" could "
                                  "not be found.  Please verify "
                                  "that your installation has been "
                                  "completed successfully.");
        HIGMessageDialog dialog(&parent,
                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                Gtk::MESSAGE_ERROR,
                                Gtk::BUTTONS_OK,
                                _("Help not found"),
                                message);
        dialog.run();
        if(error) {
          g_error_free(error);
        }
      }
    }


    void open_url(Gtk::Window & parent, const Glib::ustring & url)
    {
      if(!url.empty()) {
        GError *err = NULL;
        DBG_OUT("Opening url '%s'...", url.c_str());
        gtk_show_uri_on_window(parent.gobj(), url.c_str(), GDK_CURRENT_TIME, &err);
        if(err) {
          throw Glib::Error(err, true);
        }
      }
    }


    void show_opening_location_error(Gtk::Window * parent,
                                     const Glib::ustring & url,
                                     const Glib::ustring & error)
    {
      Glib::ustring message = Glib::ustring::compose("%1: %2", url, error);

      HIGMessageDialog dialog(parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                              Gtk::MESSAGE_INFO,
                              Gtk::BUTTONS_OK,
                              _("Cannot open location"),
                              message);
      dialog.run ();
    }

    Glib::ustring get_pretty_print_date(const sharp::DateTime & date, bool show_time)
    {
      bool use_12h = false;
      if(show_time) {
        use_12h = Preferences::obj().get_schema_settings(
          Preferences::SCHEMA_DESKTOP_GNOME_INTERFACE)->get_string(
            Preferences::DESKTOP_GNOME_CLOCK_FORMAT) == "12h";
      }
      return get_pretty_print_date(date, show_time, use_12h);
    }

    Glib::ustring get_pretty_print_date(const sharp::DateTime & date, bool show_time, bool use_12h)
    {
      Glib::ustring pretty_str;
      sharp::DateTime now = sharp::DateTime::now();
      Glib::ustring short_time = use_12h
        /* TRANSLATORS: time in 12h format. */
        ? date.to_string("%l:%M %P")
        /* TRANSLATORS: time in 24h format. */
        : date.to_string("%H:%M");

      if (date.year() == now.year()) {
        if (date.day_of_year() == now.day_of_year()) {
          pretty_str = show_time ?
            /* TRANSLATORS: argument %1 is time. */
            Glib::ustring::compose(_("Today, %1"), short_time) :
            _("Today");
        }
        else if ((date.day_of_year() < now.day_of_year())
                 && (date.day_of_year() == now.day_of_year() - 1)) {
          pretty_str = show_time ?
            /* TRANSLATORS: argument %1 is time. */
            Glib::ustring::compose(_("Yesterday, %1"), short_time) :
            _("Yesterday");
        }
        else if (date.day_of_year() > now.day_of_year()
                 && date.day_of_year() == now.day_of_year() + 1) {
          pretty_str = show_time ?
            /* TRANSLATORS: argument %1 is time. */
            Glib::ustring::compose(_("Tomorrow, %1"), short_time) :
            _("Tomorrow");
        }
        else {
          /* TRANSLATORS: date in current year. */
          pretty_str = date.to_string(_("%b %d")); // "MMMM d"
          if(show_time) {
            /* TRANSLATORS: argument %1 is date, %2 is time. */
            pretty_str = Glib::ustring::compose(_("%1, %2"), pretty_str, short_time);
          }
        }
      } 
      else if (!date.is_valid()) {
        pretty_str = _("No Date");
      }
      else {
        /* TRANSLATORS: date in other than current year. */
        pretty_str = date.to_string(_("%b %d %Y")); // "MMMM d yyyy"
        if(show_time) {
          /* TRANSLATORS: argument %1 is date, %2 is time. */
          pretty_str = Glib::ustring::compose(_("%1, %2"), pretty_str, short_time);
        }
      }

      return pretty_str;
    }

    void main_context_invoke(const sigc::slot<void> & slot)
    {
      sigc::slot<void> *data = new sigc::slot<void>(slot);
      g_main_context_invoke(NULL, main_context_invoke_func, data);
    }


    void main_context_call(const sigc::slot<void> & slot)
    {
      Glib::Threads::Mutex mutex;
      Glib::Threads::Cond cond;

      mutex.lock();
      main_context_invoke([slot, &cond, &mutex]() {
        main_context_call_func(slot, &cond, &mutex);
      });
      cond.wait(mutex);
      mutex.unlock();
    }


    Gtk::Widget * create_popover_button(const Glib::ustring & action, const Glib::ustring & label)
    {
      Gtk::ModelButton *item = new Gtk::ModelButton;
      gtk_actionable_set_action_name(GTK_ACTIONABLE(item->gobj()), action.c_str());
      item->set_label(label);
      set_common_popover_widget_props(*item);
      return item;
    }


    Gtk::Widget * create_popover_submenu_button(const Glib::ustring & submenu, const Glib::ustring & label)
    {
      Gtk::ModelButton *button = new Gtk::ModelButton;
      button->property_menu_name() = submenu;
      button->set_label(label);
      set_common_popover_widget_props(*button);
      return button;
    }


    Gtk::Box * create_popover_submenu(const Glib::ustring & name)
    {
      return new PopoverSubmenuBox(name);
    }


    void set_common_popover_widget_props(Gtk::Widget & widget)
    {
      widget.property_hexpand() = true;
    }

    void set_common_popover_widget_props(Gtk::Box & widget)
    {
      widget.property_margin_top() = 9;
      widget.property_margin_bottom() = 9;
      widget.property_margin_left() = 12;
      widget.property_margin_right() = 12;
      set_common_popover_widget_props(static_cast<Gtk::Widget&>(widget));
    }


    void add_item_to_ordered_map(std::map<int, Gtk::Widget*> & dest, int order, Gtk::Widget *item)
    {
      for(; dest.find(order) != dest.end(); ++order);
      dest[order] = item;
    }


    void merge_ordered_maps(std::map<int, Gtk::Widget*> & dest, const std::map<int, Gtk::Widget*> & adds)
    {
      for(std::map<int, Gtk::Widget*>::const_iterator iter = adds.begin(); iter != adds.end(); ++iter) {
        add_item_to_ordered_map(dest, iter->first, iter->second);
      }
    }


    void GlobalKeybinder::add_accelerator(const sigc::slot<void> & handler, guint key, 
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
    }

    void GlobalKeybinder::enabled(bool enable)
    {
      m_fake_menu.set_sensitive(enable);
      std::vector<Gtk::Widget*> items = m_fake_menu.get_children();
      FOREACH(Gtk::Widget *item, items) {
        item->set_sensitive(enable);
      }
    }


    HIGMessageDialog::HIGMessageDialog(Gtk::Window *parent,
                                       GtkDialogFlags flags, Gtk::MessageType msg_type, 
                                       Gtk::ButtonsType btn_type, const Glib::ustring & header,
                                       const Glib::ustring & msg)
      : Gtk::Dialog()
      , m_extra_widget(NULL)
      , m_image(NULL)
    {
      set_border_width(5);
      set_resizable(false);
      set_title("");

      get_vbox()->set_spacing(12);
      get_action_area()->set_layout(Gtk::BUTTONBOX_END);

      m_accel_group = Glib::RefPtr<Gtk::AccelGroup>(Gtk::AccelGroup::create());
      add_accel_group(m_accel_group);

      Gtk::Grid *hbox = manage(new Gtk::Grid);
      hbox->set_column_spacing(12);
      hbox->set_border_width(5);
      hbox->show();
      int hbox_col = 0;
      get_vbox()->pack_start(*hbox, false, false, 0);

      switch (msg_type) {
      case Gtk::MESSAGE_ERROR:
        m_image = new Gtk::Image (Gtk::Stock::DIALOG_ERROR,
                                  Gtk::ICON_SIZE_DIALOG);
        break;
      case Gtk::MESSAGE_QUESTION:
        m_image = new Gtk::Image (Gtk::Stock::DIALOG_QUESTION,
                                  Gtk::ICON_SIZE_DIALOG);
        break;
      case Gtk::MESSAGE_INFO:
        m_image = new Gtk::Image (Gtk::Stock::DIALOG_INFO,
                                  Gtk::ICON_SIZE_DIALOG);
        break;
      case Gtk::MESSAGE_WARNING:
        m_image = new Gtk::Image (Gtk::Stock::DIALOG_WARNING,
                                  Gtk::ICON_SIZE_DIALOG);
        break;
      default:
        break;
      }

      if (m_image) {
        Gtk::manage(m_image);
        m_image->show();
        m_image->property_yalign().set_value(0);
        hbox->attach(*m_image, hbox_col++, 0, 1, 1);
      }

      Gtk::Grid *label_vbox = manage(new Gtk::Grid);
      label_vbox->show();
      int label_vbox_row = 0;
      label_vbox->set_hexpand(true);
      hbox->attach(*label_vbox, hbox_col++, 0, 1, 1);

      if(header != "") {
        Glib::ustring title = Glib::ustring::compose("<span weight='bold' size='larger'>%1</span>\n", header);
        Gtk::Label *label = manage(new Gtk::Label (title));
        label->set_use_markup(true);
        label->set_justify(Gtk::JUSTIFY_LEFT);
        label->set_line_wrap(true);
        label->set_alignment (0.0f, 0.5f);
        label->show();
        label_vbox->attach(*label, 0, label_vbox_row++, 1, 1);
      }

      if(msg != "") {
        Gtk::Label *label = manage(new Gtk::Label(msg));
        label->set_use_markup(true);
        label->set_justify(Gtk::JUSTIFY_LEFT);
        label->set_line_wrap(true);
        label->set_alignment (0.0f, 0.5f);
        label->show();
        label_vbox->attach(*label, 0, label_vbox_row++, 1, 1);
      }
      
      m_extra_widget_vbox = manage(new Gtk::Grid);
      m_extra_widget_vbox->show();
      m_extra_widget_vbox->set_margin_left(12);
      label_vbox->attach(*m_extra_widget_vbox, 0, label_vbox_row++, 1, 1);

      switch (btn_type) {
      case Gtk::BUTTONS_NONE:
        break;
      case Gtk::BUTTONS_OK:
        add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK, true);
        break;
      case Gtk::BUTTONS_CLOSE:
        add_button (Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE, true);
        break;
      case Gtk::BUTTONS_CANCEL:
        add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL, true);
        break;
      case Gtk::BUTTONS_YES_NO:
        add_button (Gtk::Stock::NO, Gtk::RESPONSE_NO, false);
        add_button (Gtk::Stock::YES, Gtk::RESPONSE_YES, true);
        break;
      case Gtk::BUTTONS_OK_CANCEL:
        add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL, false);
        add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK, true);
        break;
      }

      if (parent){
        set_transient_for(*parent);
      }

      if ((flags & GTK_DIALOG_MODAL) != 0) {
        set_modal(true);
      }

      if ((flags & GTK_DIALOG_DESTROY_WITH_PARENT) != 0) {
        property_destroy_with_parent().set_value(true);
      }
    }


    void HIGMessageDialog::add_button(const Gtk::BuiltinStockID& stock_id, 
                                       Gtk::ResponseType resp, bool is_default)
    {
      Gtk::Button *button = manage(new Gtk::Button (stock_id));
      button->property_can_default().set_value(true);
      
      add_button(button, resp, is_default);
    }

    void HIGMessageDialog::add_button (const Glib::RefPtr<Gdk::Pixbuf> & pixbuf, 
                                       const Glib::ustring & label_text, 
                                       Gtk::ResponseType resp, bool is_default)
    {
      Gtk::Button *button = manage(new Gtk::Button());
      Gtk::Image *image = manage(new Gtk::Image(pixbuf));
      // NOTE: This property is new to GTK+ 2.10, but we don't
      //       really need the line because we're just setting
      //       it to the default value anyway.
      //button.ImagePosition = Gtk::PositionType.Left;
      button->set_image(*image);
      button->set_label(label_text);
      button->set_use_underline(true);
      button->property_can_default().set_value(true);
      
      add_button (button, resp, is_default);
    }
    
    void HIGMessageDialog::add_button (Gtk::Button *button, Gtk::ResponseType resp, bool is_default)
    {
      button->show();

      add_action_widget (*button, resp);

      if (is_default) {
        set_default_response(resp);
        button->add_accelerator ("activate", m_accel_group,
                                 GDK_KEY_Escape, (Gdk::ModifierType)0,
                                 Gtk::ACCEL_VISIBLE);
      }
    }


    void HIGMessageDialog::set_extra_widget(Gtk::Widget *value)
    {
      if (m_extra_widget) {
          m_extra_widget_vbox->remove (*m_extra_widget);
      }
        
      m_extra_widget = value;
      m_extra_widget->show_all ();
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
        push_back(sharp::Uri(s));
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


    void UriList::get_local_paths(std::list<Glib::ustring> & paths) const
    {
      for(const_iterator iter = begin(); iter != end(); ++iter) {

        const sharp::Uri & uri(*iter);

        if(uri.is_file()) {
          paths.push_back(uri.local_path());
        }
      }
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

      if (!iter.begins_tag(m_tag)) {
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


    bool InterruptableTimeout::callback(InterruptableTimeout* self)
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

    ToolMenuButton::ToolMenuButton(Gtk::Widget & widget, Gtk::Menu *menu)
      : Gtk::ToggleToolButton(widget)
      ,  m_menu(menu)
    {
      _common_init();
    }

    ToolMenuButton::ToolMenuButton(Gtk::Toolbar& toolbar, const Gtk::BuiltinStockID& stock_image, 
                                   const Glib::ustring & label, 
                                   Gtk::Menu * menu)
      : Gtk::ToggleToolButton()
      ,  m_menu(menu)
    {
      _common_init(*manage(new Gtk::Image(stock_image, toolbar.get_icon_size())),
                   label);
    }

    ToolMenuButton::ToolMenuButton(Gtk::Image& image, 
                                   const Glib::ustring & label, 
                                   Gtk::Menu * menu)
      : Gtk::ToggleToolButton()
      ,  m_menu(menu)
    {
      _common_init(image, label);
    }


    void ToolMenuButton::_common_init()
    {
      property_can_focus() = true;
      gtk_menu_attach_to_widget(m_menu->gobj(), static_cast<Gtk::Widget*>(this)->gobj(),
                                NULL);
      m_menu->signal_deactivate().connect(sigc::mem_fun(*this, &ToolMenuButton::release_button));
      show_all();
    }


    void ToolMenuButton::_common_init(Gtk::Image& image, const Glib::ustring & label)
    {
      set_icon_widget(image);
      set_label_widget(*manage(new Gtk::Label(label, true)));
      _common_init();
    }


    bool ToolMenuButton::on_button_press_event(GdkEventButton *ev)
    {
      popup_menu(*m_menu, ev);
      return true;
    }

    void ToolMenuButton::on_clicked()
    {
      m_menu->select_first(true);
      popup_menu(*m_menu, NULL);
    }

    bool ToolMenuButton::on_mnemonic_activate(bool group_cycling)
    {
      // ToggleButton always grabs focus away from the editor,
      // so reimplement Widget's version, which only grabs the
      // focus if we are group cycling.
      if (!group_cycling) {
        activate();
      } 
      else if (get_can_focus()) {
        grab_focus();
      }

      return true;
    }


    void ToolMenuButton::release_button()
    {
      set_active(false);
    }


    CheckAction::CheckAction(const Glib::ustring & name)
      : Gtk::Action(name)
      , m_checked(false)
    {}

    Gtk::Widget *CheckAction::create_menu_item_vfunc()
    {
      Gtk::CheckMenuItem *item = new Gtk::CheckMenuItem;
      item->set_active(m_checked);
      return item;
    }

    void CheckAction::on_activate()
    {
      m_checked = !m_checked;
      Gtk::Action::on_activate();
    }

  }
}
