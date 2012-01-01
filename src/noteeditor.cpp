/*
 * gnote
 *
 * Copyright (C) 2010-2012 Aurimas Cernius
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


#include <string.h>

#include "notebuffer.hpp"
#include "noteeditor.hpp"
#include "preferences.hpp"
#include "utils.hpp"
#include "debug.hpp"
#include "sharp/string.hpp"

namespace gnote {

  NoteEditor::NoteEditor(const Glib::RefPtr<Gtk::TextBuffer> & buffer)
    : Gtk::TextView(buffer)
  {
    set_wrap_mode(Gtk::WRAP_WORD);
    set_left_margin(default_margin());
    set_right_margin(default_margin());
    property_can_default().set_value(true);

    Glib::RefPtr<Gio::Settings> settings = Preferences::obj().get_schema_settings(Preferences::SCHEMA_GNOTE);
    //Set up the schema to watch the default document font
    Glib::RefPtr<Gio::Settings> desktop_settings = Preferences::obj()
      .get_schema_settings(Preferences::SCHEMA_DESKTOP_GNOME_INTERFACE);
    if(desktop_settings) {
      desktop_settings->signal_changed().connect(
        sigc::mem_fun(*this, &NoteEditor::on_font_setting_changed));
    }

    // Set Font from preference
    if (settings->get_boolean(Preferences::ENABLE_CUSTOM_FONT)) {
      std::string font_string = settings->get_string(Preferences::CUSTOM_FONT_FACE);
      override_font (Pango::FontDescription(font_string));
    }
    else {
      override_font (get_gnome_document_font_description ());
    }

    settings->signal_changed().connect(sigc::mem_fun(*this, &NoteEditor::on_font_setting_changed));

    // Set extra editor drag targets supported (in addition
    // to the default TextView's various text formats)...
    Glib::RefPtr<Gtk::TargetList> list = drag_dest_get_target_list();

    
    list->add ("text/uri-list", (Gtk::TargetFlags)0, 1);
    list->add ("_NETSCAPE_URL", (Gtk::TargetFlags)0, 1);

    signal_key_press_event().connect(sigc::mem_fun(*this, &NoteEditor::key_pressed), false);
    signal_button_press_event().connect(sigc::mem_fun(*this, &NoteEditor::button_pressed), false);
  }


  Pango::FontDescription NoteEditor::get_gnome_document_font_description()
  {
    try {
      Glib::RefPtr<Gio::Settings> desktop_settings = Preferences::obj()
        .get_schema_settings(Preferences::SCHEMA_DESKTOP_GNOME_INTERFACE);
      if(desktop_settings) {
        std::string doc_font_string =
          desktop_settings->get_string(Preferences::DESKTOP_GNOME_FONT);
        return Pango::FontDescription(doc_font_string);
      }
    } 
    catch (...) {

    }

    return Pango::FontDescription();
  }


  void NoteEditor::on_font_setting_changed (const Glib::ustring & key)
  {
    if(key == Preferences::ENABLE_CUSTOM_FONT || key == Preferences::CUSTOM_FONT_FACE) {
      update_custom_font_setting ();
    }
    else if(key == Preferences::DESKTOP_GNOME_FONT) {
      if (!Preferences::obj().get_schema_settings(
          Preferences::SCHEMA_GNOTE)->get_boolean(Preferences::ENABLE_CUSTOM_FONT)) {
        Glib::RefPtr<Gio::Settings> desktop_settings = Preferences::obj()
          .get_schema_settings(Preferences::SCHEMA_DESKTOP_GNOME_INTERFACE);
        if(desktop_settings) {
          std::string value = desktop_settings->get_string(key);
          modify_font_from_string(value);
        }
      }
    }
  }


  void NoteEditor::update_custom_font_setting()
  {
    Glib::RefPtr<Gio::Settings> settings = Preferences::obj()
      .get_schema_settings(Preferences::SCHEMA_GNOTE);

    if (settings->get_boolean(Preferences::ENABLE_CUSTOM_FONT)) {
      std::string fontString = settings->get_string(Preferences::CUSTOM_FONT_FACE);
      DBG_OUT( "Switching note font to '%s'...", fontString.c_str());
      modify_font_from_string (fontString);
    } 
    else {
      DBG_OUT("Switching back to the default font");
      override_font (get_gnome_document_font_description());
    }
  }

  
  void NoteEditor::modify_font_from_string (const std::string & fontString)
  {
    DBG_OUT("Switching note font to '%s'...", fontString.c_str());
    override_font (Pango::FontDescription(fontString));
  }

  

    //
    // DND Drop handling
    //
  void NoteEditor::on_drag_data_received (Glib::RefPtr<Gdk::DragContext> & context,
                                          int x, int y,
                                          const Gtk::SelectionData & selection_data,
                                          guint info,  guint time)
  {
    bool has_url = false;

    std::vector<std::string> targets = context->list_targets();
    for(std::vector<std::string>::const_iterator iter = targets.begin();
        iter != targets.end(); ++iter) {
      const std::string & target(*iter);
      if (target == "text/uri-list" ||
          target == "_NETSCAPE_URL") {
        has_url = true;
        break;
      }
    }

    if (has_url) {
      utils::UriList uri_list(selection_data);
      bool more_than_one = false;

      // Place the cursor in the position where the uri was
      // dropped, adjusting x,y by the TextView's VisibleRect.
      Gdk::Rectangle rect;
      get_visible_rect(rect);
      int adjustedX = x + rect.get_x();
      int adjustedY = y + rect.get_y();
      Gtk::TextIter cursor;
      get_iter_at_location (cursor, adjustedX, adjustedY);
      get_buffer()->place_cursor (cursor);

      Glib::RefPtr<Gtk::TextTag> link_tag = get_buffer()->get_tag_table()->lookup ("link:url");

      for(utils::UriList::const_iterator iter = uri_list.begin();
          iter != uri_list.end(); ++iter) {
        const sharp::Uri & uri(*iter);
        DBG_OUT("Got Dropped URI: %s", uri.to_string().c_str());
        std::string insert;
        if (uri.is_file()) {
          // URL-escape the path in case
          // there are spaces (bug #303902)
          insert = sharp::Uri::escape_uri_string(uri.local_path());
        } 
        else {
          insert = uri.to_string ();
        }

        if (insert.empty() || sharp::string_trim(insert).empty())
          continue;

        if (more_than_one) {
          cursor = get_buffer()->get_iter_at_mark (get_buffer()->get_insert());

          // FIXME: The space here is a hack
          // around a bug in the URL Regex which
          // matches across newlines.
          if (cursor.get_line_offset() == 0) {
            get_buffer()->insert (cursor, " \n");
          }
          else {
            get_buffer()->insert (cursor, ", ");
          }
        }

        get_buffer()->insert_with_tag(cursor, insert, link_tag);
        more_than_one = true;
      }

      context->drag_finish(more_than_one, false, time);
    } 
    else {
      Gtk::TextView::on_drag_data_received (context, x, y, selection_data, info, time);
    }
  }

  bool NoteEditor::key_pressed (GdkEventKey * ev)
  {
      bool ret_value = false;

      switch (ev->keyval)
      {
      case GDK_KEY_KP_Enter:
      case GDK_KEY_Return:
        // Allow opening notes with Ctrl + Enter
        if (ev->state != Gdk::CONTROL_MASK) {
          if (ev->state & Gdk::SHIFT_MASK) {
            ret_value = NoteBuffer::Ptr::cast_static(get_buffer())->add_new_line (true);
          } 
          else {
            ret_value = NoteBuffer::Ptr::cast_static(get_buffer())->add_new_line (false);
          }          
          scroll_to (get_buffer()->get_insert());
        }
        break;
      case GDK_KEY_Tab:
        ret_value = NoteBuffer::Ptr::cast_static(get_buffer())->add_tab ();
        scroll_to (get_buffer()->get_insert());
        break;
      case GDK_KEY_ISO_Left_Tab:
        ret_value = NoteBuffer::Ptr::cast_static(get_buffer())->remove_tab ();
        scroll_to (get_buffer()->get_insert());
        break;
      case GDK_KEY_Delete:
        if (Gdk::SHIFT_MASK != (ev->state & Gdk::SHIFT_MASK)) {
          ret_value = NoteBuffer::Ptr::cast_static(get_buffer())->delete_key_handler();
          scroll_to (get_buffer()->get_insert());
        }
        break;
      case GDK_KEY_BackSpace:
        ret_value = NoteBuffer::Ptr::cast_static(get_buffer())->backspace_key_handler();
        break;
      case GDK_KEY_Left:
      case GDK_KEY_Right:
      case GDK_KEY_Up:
      case GDK_KEY_Down:
      case GDK_KEY_End:
        ret_value = false;
        break;
      default:
        NoteBuffer::Ptr::cast_static(get_buffer())->check_selection();
        break;
      }

      return ret_value;
    }


  bool NoteEditor::button_pressed (GdkEventButton * )
  {
    NoteBuffer::Ptr::cast_static(get_buffer())->check_selection();
    return false;
  }



}
