/*
 * gnote
 *
 * Copyright (C) 2011,2013-2014,2017,2019-2024 Aurimas Cernius
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



#include <gtk/gtk.h>
#include <gtkmm/image.h>
#include <gtkmm/linkbutton.h>

#include "sharp/xmlreader.hpp"
#include "sharp/xmlwriter.hpp"
#include "debug.hpp"
#include "notetag.hpp"
#include "noteeditor.hpp"
#include "utils.hpp"

namespace gnote {

namespace {

  void change_highlight(NoteTagTable &tag_table, const std::function<void(Gtk::TextTag &tag)> &closure)
  {
    if(auto tag = tag_table.lookup("highlight")) {
      closure(*tag);
    }
    else {
      ERR_OUT("Tag 'highlight' not found!");
    }
  }
}

  NoteTag::NoteTag(Glib::ustring && tag_name, int flags)
    : Gtk::TextTag(tag_name)
    , m_element_name(std::move(tag_name))
    , m_widget(NULL)
    , m_flags(flags | CAN_SERIALIZE | CAN_SPLIT)
  {
    if(m_element_name.empty()) {
      throw sharp::Exception ("NoteTags must have a tag name.  Use "
                              "DynamicNoteTag for constructing "
                              "anonymous tags.");
    }
    
  }

  
  NoteTag::NoteTag()
    : Gtk::TextTag()
    , m_widget(NULL)
    , m_flags(0)
  {
  }


  void NoteTag::initialize(Glib::ustring && element_name)
  {
    m_element_name = std::move(element_name);
    m_flags = CAN_SERIALIZE | CAN_SPLIT;
    m_save_type = CONTENT;
  }
  


  void NoteTag::set_can_serialize(bool value)
  {
    if (value) {
      m_flags |= CAN_SERIALIZE;
    }
    else {
      m_flags &= ~CAN_SERIALIZE;
    }
  }

  void NoteTag::set_can_undo(bool value)
  {
    if (value) {
      m_flags |= CAN_UNDO;
    }
    else {
      m_flags &= ~CAN_UNDO;
    }
  }

  void NoteTag::set_can_grow(bool value)
  {
    if (value) {
      m_flags |= CAN_GROW;
    }
    else {
      m_flags &= ~CAN_GROW;
    }
  }

  void NoteTag::set_can_spell_check(bool value)
  {
    if (value) {
      m_flags |= CAN_SPELL_CHECK;
    }
    else {
      m_flags &= ~CAN_SPELL_CHECK;
    }
  }

  void NoteTag::set_can_activate(bool value)
  {
    if (value) {
      m_flags |= CAN_ACTIVATE;
    }
    else {
      m_flags &= ~CAN_ACTIVATE;
    }
  }

  void NoteTag::set_can_split(bool value)
  {
    if (value) {
      m_flags |= CAN_SPLIT;
    }
    else {
      m_flags &= ~CAN_SPLIT;
    }
  }

  void NoteTag::get_extents(const Gtk::TextIter & iter, Gtk::TextIter & start,
                            Gtk::TextIter & end)
  {
    Glib::RefPtr<Gtk::TextTag> this_ref = NoteTagTable::instance()->lookup(property_name());
    start = iter;
    if(!start.starts_tag(this_ref)) {
      start.backward_to_tag_toggle (this_ref);
    }
    end = iter;
    end.forward_to_tag_toggle (this_ref);
  }

  void NoteTag::write(sharp::XmlWriter & xml, bool start) const
  {
    if (can_serialize()) {
      if (start) {
        xml.write_start_element ("", m_element_name, "");
      } 
      else {
        xml.write_end_element();
      }
    }
  }

  void NoteTag::read(sharp::XmlReader & xml, bool start)
  {
    if (can_serialize()) {
      if (start) {
        m_element_name = xml.get_name();
      }
    }
  }

  bool NoteTag::activate(const NoteEditor & editor, const Gtk::TextIter & pos)
  {
    bool retval = false;
    if(!can_activate()) {
      return retval;
    }

    Gtk::TextIter start, end;
    get_extents(pos, start, end);
    retval = m_signal_activate(editor, start, end);

    return retval;
  }


  void NoteTag::set_widget(Gtk::Widget * value)
  {
    if ((value == NULL) && m_widget) {
      delete m_widget;
    }

    m_widget = value;

    try {
      m_signal_changed(*this, false);
    } catch (sharp::Exception & e) {
      DBG_OUT("Exception calling TagChanged from NoteTag.set_Widget: %s", e.what());
    }
  }

  void DynamicNoteTag::write(sharp::XmlWriter & xml, bool start) const
  {
    if (can_serialize()) {
      NoteTag::write (xml, start);

      if (start) {
        for(AttributeMap::const_iterator iter = m_attributes.begin();
            iter != m_attributes.end(); ++iter) {
          xml.write_attribute_string ("", iter->first, "", iter->second);
        }
      }
    }
  }

  void DynamicNoteTag::read(sharp::XmlReader & xml, bool start)
  {
    if (can_serialize()) {
      NoteTag::read (xml, start);

      if (start) {
          while (xml.move_to_next_attribute()) {
            Glib::ustring name = xml.get_name();

            xml.read_attribute_value();
            m_attributes [name] = xml.get_value();

            on_attribute_read (name);
            DBG_OUT("NoteTag: %s read attribute %s='%s'",
                    get_element_name().c_str(), name.c_str(), xml.get_value().c_str());
          }
      }
    }
  }
  
  DepthNoteTag::DepthNoteTag(int depth)
    : NoteTag("depth:" + TO_STRING(depth) + ":" + TO_STRING((int)Pango::Direction::LTR))
    , m_depth(depth)
  {
  }

  

  void DepthNoteTag::write(sharp::XmlWriter & xml, bool start) const
  {
    if (can_serialize()) {
      if (start) {
        xml.write_start_element ("", "list-item", "");

        // Write the list items writing direction
        xml.write_start_attribute ("dir");
        if (get_direction() == Pango::Direction::RTL) {
          xml.write_string ("rtl");
        }
        else {
          xml.write_string ("ltr");
        }
        xml.write_end_attribute ();
      } 
      else {
        xml.write_end_element ();
      }
    }
  }


  NoteTagTable::Ptr NoteTagTable::s_instance;

  void NoteTagTable::_init_common_tags()
  {
    NoteTag::Ptr tag;
    Gdk::RGBA active_link_color, visited_link_color;
    {
      Gtk::LinkButton link;
      auto style_ctx = link.get_style_context();
      style_ctx->set_state(Gtk::StateFlags::LINK);
      active_link_color = style_ctx->get_color();
      style_ctx->set_state(Gtk::StateFlags::VISITED);
      visited_link_color = style_ctx->get_color();
    }

    m_preferences.signal_highlight_background_color_changed
      .connect(sigc::mem_fun(*this, &NoteTagTable::on_highlight_background_setting_changed));
    m_preferences.signal_highlight_foreground_color_changed
      .connect(sigc::mem_fun(*this, &NoteTagTable::on_highlight_foreground_setting_changed));

    // Font stylings

    tag = NoteTag::create("centered", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_justification() = Gtk::Justification::CENTER;
    add(tag);

    tag = NoteTag::create("bold", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_weight() = PANGO_WEIGHT_BOLD;
    add(tag);

    tag = NoteTag::create("italic", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_style() = Pango::Style::ITALIC;
    add(tag);
    
    tag = NoteTag::create("strikethrough", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_strikethrough() = true;
    add(tag);

    tag = NoteTag::create("highlight", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_background() = m_preferences.highlight_background_color();
    tag->property_foreground() = m_preferences.highlight_foreground_color();
    add(tag);

    tag = NoteTag::create("find-match", NoteTag::CAN_SPELL_CHECK);
    tag->property_background() = "#57e389"; // libadwaita green_2
    tag->property_foreground() = "#241f31"; // libadwaita dark_4
    tag->set_can_serialize(false);
    tag->set_save_type(META);
    add(tag);

    tag = NoteTag::create("note-title", 0);
    tag->property_foreground_rgba().set_value(active_link_color);
    tag->property_foreground_set() = true;
    tag->property_scale() = Pango::SCALE_XX_LARGE;
    // FiXME: Hack around extra rewrite on open
    tag->set_can_serialize(false);
    tag->set_save_type(META);
    add(tag);
      
    tag = NoteTag::create("related-to", 0);
    tag->property_scale() = Pango::SCALE_SMALL;
    tag->property_left_margin() = 40;
    tag->property_editable() = false;
    tag->set_save_type(META);
    add(tag);

    // Used when inserting dropped URLs/text to Start Here
    tag = NoteTag::create("datetime", 0);
    tag->property_scale() = Pango::SCALE_SMALL;
    tag->property_style() = Pango::Style::ITALIC;
    tag->property_foreground_rgba().set_value(visited_link_color);
    tag->property_foreground_set() = true;
    tag->set_save_type(META);
    add(tag);

    // Font sizes

    tag = NoteTag::create("size:huge", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_scale() = Pango::SCALE_XX_LARGE;
    add(tag);

    tag = NoteTag::create("size:large", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_scale() = Pango::SCALE_X_LARGE;
    add(tag);

    tag = NoteTag::create("size:normal", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_scale() = Pango::SCALE_MEDIUM;
    add(tag);

    tag = NoteTag::create("size:small", NoteTag::CAN_UNDO | NoteTag::CAN_GROW | NoteTag::CAN_SPELL_CHECK);
    tag->property_scale() = Pango::SCALE_SMALL;
    add(tag);

    // Links

    tag = NoteTag::create("link:broken", NoteTag::CAN_ACTIVATE);
    tag->property_underline() = Pango::Underline::SINGLE;
    tag->property_foreground_rgba().set_value(visited_link_color);
    tag->property_foreground_set() = true;
    tag->set_save_type(META);
    add(tag);
    m_broken_link_tag = tag;

    tag = NoteTag::create("link:internal", NoteTag::CAN_ACTIVATE);
    tag->property_underline() = Pango::Underline::SINGLE;
    tag->property_foreground_rgba().set_value(active_link_color);
    tag->property_foreground_set() = true;
    tag->set_save_type(META);
    add(tag);
    m_link_tag = tag;

    tag = NoteTag::create("link:url", NoteTag::CAN_ACTIVATE);
    tag->property_underline() = Pango::Underline::SINGLE;
    tag->property_foreground_rgba().set_value(active_link_color);
    tag->property_foreground_set() = true;
    tag->set_save_type(META);
    add(tag);
    m_url_tag = tag;
  }

  void NoteTagTable::on_highlight_background_setting_changed()
  {
    change_highlight(*this, [this](Gtk::TextTag &tag) {
      tag.property_background() = m_preferences.highlight_background_color();
    });
  }

  void NoteTagTable::on_highlight_foreground_setting_changed()
  {
    change_highlight(*this, [this](Gtk::TextTag &tag) {
      tag.property_foreground() = m_preferences.highlight_foreground_color();
    });
  }

  bool NoteTagTable::tag_is_serializable(const Glib::RefPtr<const Gtk::TextTag> & tag)
  {
    NoteTag::ConstPtr note_tag = std::dynamic_pointer_cast<const NoteTag>(tag);
    if(note_tag) {
      return note_tag->can_serialize();
    }
    return false;
  }

  bool NoteTagTable::tag_is_growable(const Glib::RefPtr<Gtk::TextTag> & tag)
  {
    NoteTag::Ptr note_tag = std::dynamic_pointer_cast<NoteTag>(tag);
    if(note_tag) {
      return note_tag->can_grow();
    }
    return false;
  }

  bool NoteTagTable::tag_is_undoable(const Glib::RefPtr<Gtk::TextTag> & tag)
  {
    NoteTag::Ptr note_tag = std::dynamic_pointer_cast<NoteTag>(tag);
    if(note_tag) {
      return note_tag->can_undo();
    }
    return false;
  }


  bool NoteTagTable::tag_is_spell_checkable(const Glib::RefPtr<const Gtk::TextTag> & tag)
  {
    NoteTag::ConstPtr note_tag = std::dynamic_pointer_cast<const NoteTag>(tag);
    if(note_tag) {
      return note_tag->can_spell_check();
    }
    return false;
  }


  bool NoteTagTable::tag_is_activatable(const Glib::RefPtr<Gtk::TextTag> & tag)
  {
    NoteTag::Ptr note_tag = std::dynamic_pointer_cast<NoteTag>(tag);
    if(note_tag) {
      return note_tag->can_activate();
    }
    return false;
  }


  bool NoteTagTable::tag_has_depth(const Glib::RefPtr<Gtk::TextBuffer::Tag> & tag)
  {
    return (bool)std::dynamic_pointer_cast<DepthNoteTag>(tag);
  }


  bool NoteTagTable::has_link_tag(const Gtk::TextIter & iter)
  {
    return iter.has_tag(get_link_tag()) || iter.has_tag(get_url_tag()) || iter.has_tag(get_broken_link_tag());
  }


  ChangeType NoteTagTable::get_change_type(const Glib::RefPtr<Gtk::TextTag> &tag)
  {
    ChangeType change;

    // Use tag Name for Gtk.TextTags
    // For extensibility, add Gtk.TextTag names here
    change = OTHER_DATA_CHANGED;

    // Use SaveType for NoteTags
    Glib::RefPtr<NoteTag> note_tag = std::dynamic_pointer_cast<NoteTag>(tag);
    if(note_tag) {
      switch(note_tag->save_type()) {
        case META:
          change = OTHER_DATA_CHANGED;
          break;
        case CONTENT:
          change = CONTENT_CHANGED;
          break;
        case NO_SAVE:
        default:
          change = NO_CHANGE;
        break;
      }
    }

    return change;
  }
  

  DepthNoteTag::Ptr NoteTagTable::get_depth_tag(int depth)
  {
    Glib::ustring name = "depth:" + TO_STRING(depth) + ":" + TO_STRING((int)Pango::Direction::LTR);

    DepthNoteTag::Ptr tag = std::dynamic_pointer_cast<DepthNoteTag>(lookup(name));

    if(!tag) {
      tag = Glib::make_refptr_for_instance(new DepthNoteTag(depth));
      tag->property_indent().set_value(-14);
      tag->property_left_margin().set_value((depth+1) * 25);
      tag->property_pixels_below_lines().set_value(4);
      tag->property_scale().set_value(Pango::SCALE_MEDIUM);
      add(tag);
    }

    return tag;
  }
      
  DynamicNoteTag::Ptr NoteTagTable::create_dynamic_tag(Glib::ustring && tag_name)
  {
    auto iter = m_tag_types.find(tag_name);
    if(iter == m_tag_types.end()) {
      return DynamicNoteTag::Ptr();
    }
    DynamicNoteTag::Ptr tag(iter->second());
    tag->initialize(std::move(tag_name));
    add(tag);
    return tag;
  }

 
  void NoteTagTable::register_dynamic_tag(const Glib::ustring & tag_name, const Factory & factory)
  {
    m_tag_types[tag_name] = factory;
  }


  bool NoteTagTable::is_dynamic_tag_registered(const Glib::ustring & tag_name)
  {
    return m_tag_types.find(tag_name) != m_tag_types.end();
  }
}

