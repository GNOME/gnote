/*
 * gnote
 *
 * Copyright (C) 2010-2013,2016-2017,2019-2024 Aurimas Cernius
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

#include <gtkmm/settings.h>

#include "notebuffer.hpp"
#include "noteeditor.hpp"
#include "preferences.hpp"
#include "undo.hpp"
#include "utils.hpp"
#include "debug.hpp"
#include "sharp/string.hpp"
#include "sharp/uri.hpp"

namespace gnote {

  NoteEditor::NoteEditor(Glib::RefPtr<Gtk::TextBuffer> && buffer, Preferences & preferences)
    : Gtk::TextView(std::move(buffer))
    , m_preferences(preferences)
  {
    set_wrap_mode(Gtk::WrapMode::WORD);
    set_left_margin(default_margin());
    set_right_margin(default_margin());

    m_preferences.signal_enable_custom_font_changed.connect(sigc::mem_fun(*this, &NoteEditor::update_custom_font_setting));
    m_preferences.signal_custom_font_face_changed.connect(sigc::mem_fun(*this, &NoteEditor::update_custom_font_setting));

    // query all monitored settings to get change notifications
    bool enable_custom_font = m_preferences.enable_custom_font();
    auto font_string = m_preferences.custom_font_face();

    // Set Font from preference
    if(enable_custom_font) {
      modify_font_from_string(font_string);
    }

    m_drop_target = Gtk::DropTarget::create(G_TYPE_INVALID, Gdk::DragAction::COPY);
    std::vector<GType> drop_types;
    drop_types.push_back(Glib::Value<Glib::ustring>::value_type());
    drop_types.push_back(Glib::Value<std::vector<Glib::ustring>>::value_type());
    m_drop_target->set_gtypes(drop_types);
    m_drop_target->signal_drop().connect(sigc::mem_fun(*this, &NoteEditor::on_drag_data_received), false);
    add_controller(m_drop_target);

    m_key_controller = Gtk::EventControllerKey::create();
    m_key_controller->signal_key_pressed().connect(sigc::mem_fun(*this, &NoteEditor::key_pressed), false);
    add_controller(m_key_controller);

    g_signal_connect(gobj(), "paste-clipboard", G_CALLBACK(paste_started), this);
    g_signal_connect_after(gobj(), "paste-clipboard", G_CALLBACK(paste_ended), this);
  }


  void NoteEditor::update_custom_font_setting()
  {
    if (m_preferences.enable_custom_font()) {
      auto fontString = m_preferences.custom_font_face();
      DBG_OUT( "Switching note font to '%s'...", fontString.c_str());
      modify_font_from_string (fontString);
    } 
    else {
      DBG_OUT("Switching back to the default font");
      Gtk::Settings::get_default()->reset_property("gtk-font-name");
    }
  }

  
  void NoteEditor::modify_font_from_string (const Glib::ustring & fontString)
  {
    DBG_OUT("Switching note font to '%s'...", fontString.c_str());
    Gtk::Settings::get_default()->property_gtk_font_name() = fontString;
  }

  

    //
    // DND Drop handling
    //
  bool NoteEditor::on_drag_data_received(const Glib::ValueBase & value, double x, double y)
  {
    std::vector<Glib::ustring> values;
    if(G_VALUE_HOLDS_STRING(value.gobj())) {
      Glib::ustring val = static_cast<const Glib::Value<Glib::ustring>&>(value).get();
      if(signal_drop_string(val, x, y)) {
        return true;
      }
      values.emplace_back(std::move(val));
    }
    else if(G_VALUE_HOLDS(value.gobj(), Glib::Value<std::vector<Glib::ustring>>::value_type())) {
      values = static_cast<const Glib::Value<std::vector<Glib::ustring>>&>(value).get();
    }
    else {
      return false;
    }

    auto drop_fmt = m_drop_target->get_current_drop()->get_formats();
    bool has_url = false;
    if(drop_fmt->contain_mime_type("_NETSCAPE_URL")) {
      has_url = true;
    }

    // Place the cursor in the position where the uri was
    // dropped, adjusting x,y by the TextView's VisibleRect.
    Gdk::Rectangle rect;
    get_visible_rect(rect);
    int adjustedX = x + rect.get_x();
    int adjustedY = y + rect.get_y();
    Gtk::TextIter cursor;
    get_iter_at_location(cursor, adjustedX, adjustedY);
    get_buffer()->place_cursor(cursor);

    bool more_than_one = false;
    for(const auto & str : values) {
      sharp::Uri uri{Glib::ustring(str)};

      Glib::ustring insert;
      if(uri.is_file()) {
        // URL-escape the path in case
        // there are spaces (bug #303902)
        insert = sharp::Uri::escape_uri_string(uri.local_path());
      }
      else {
        insert = str;
      }

      if(insert.empty() || sharp::string_trim(insert).empty()) {
        continue;
      }

      if(more_than_one) {
        // FIXME: The space here is a hack
        // around a bug in the URL Regex which
        // matches across newlines.
        if(cursor.get_line_offset() == 0) {
          cursor = get_buffer()->insert(cursor, " \n");
        }
        else {
          cursor = get_buffer()->insert(cursor, ", ");
        }
      }

      if(has_url) {
        Glib::RefPtr<Gtk::TextTag> link_tag = get_buffer()->get_tag_table()->lookup("link:url");
        cursor = get_buffer()->insert_with_tag(cursor, insert, link_tag);
      }
      else {
        cursor = get_buffer()->insert(cursor, insert);
      }
      more_than_one = true;
    }

    return true;
  }

  bool NoteEditor::key_pressed(guint keyval, guint keycode, Gdk::ModifierType state)
  {
    bool ret_value = false;
    if(!get_editable()) {
      return ret_value;
    }

    switch(keyval)
    {
    case GDK_KEY_KP_Enter:
    case GDK_KEY_Return:
      // Allow opening notes with Ctrl + Enter
      if(state != Gdk::ModifierType::CONTROL_MASK) {
        if((state & Gdk::ModifierType::SHIFT_MASK) == Gdk::ModifierType::SHIFT_MASK) {
          ret_value = std::static_pointer_cast<NoteBuffer>(get_buffer())->add_new_line(true);
        }
        else {
          ret_value = std::static_pointer_cast<NoteBuffer>(get_buffer())->add_new_line(false);
        }
        scroll_to(get_buffer()->get_insert());
      }
      break;
    case GDK_KEY_Tab:
      ret_value = std::static_pointer_cast<NoteBuffer>(get_buffer())->add_tab();
      scroll_to(get_buffer()->get_insert());
      break;
    case GDK_KEY_ISO_Left_Tab:
      ret_value = std::static_pointer_cast<NoteBuffer>(get_buffer())->remove_tab();
      scroll_to(get_buffer()->get_insert());
      break;
    case GDK_KEY_Delete:
      if(Gdk::ModifierType::SHIFT_MASK != (state & Gdk::ModifierType::SHIFT_MASK)) {
        ret_value = std::static_pointer_cast<NoteBuffer>(get_buffer())->delete_key_handler();
        scroll_to(get_buffer()->get_insert());
      }
      break;
    case GDK_KEY_BackSpace:
      ret_value = std::static_pointer_cast<NoteBuffer>(get_buffer())->backspace_key_handler();
      break;
    case GDK_KEY_Left:
    case GDK_KEY_Right:
    case GDK_KEY_Up:
    case GDK_KEY_Down:
    case GDK_KEY_End:
      ret_value = false;
      break;
    default:
      std::static_pointer_cast<NoteBuffer>(get_buffer())->check_selection();
      break;
    }

    return ret_value;
  }

  void NoteEditor::paste_started(GtkTextView*, NoteEditor *_this)
  {
    _this->on_paste_start();
  }

  void NoteEditor::paste_ended(GtkTextView*, NoteEditor *_this)
  {
    _this->on_paste_end();
  }

  void NoteEditor::on_paste_start()
  {
    auto buffer = std::dynamic_pointer_cast<NoteBuffer>(get_buffer());
    buffer->undoer().add_undo_action(new EditActionGroup(true));
  }

  void NoteEditor::on_paste_end()
  {
    auto buffer = std::dynamic_pointer_cast<NoteBuffer>(get_buffer());
    buffer->undoer().add_undo_action(new EditActionGroup(false));
  }


}
