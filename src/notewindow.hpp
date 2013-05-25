/*
 * gnote
 *
 * Copyright (C) 2011-2013 Aurimas Cernius
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




#ifndef _NOTEWINDOW_HPP__
#define _NOTEWINDOW_HPP__

#include <gtkmm/accelgroup.h>
#include <gtkmm/grid.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/menu.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/textview.h>
#include <gtkmm/scrolledwindow.h>

#include "mainwindowembeds.hpp"
#include "note.hpp"
#include "undo.hpp"
#include "utils.hpp"
#include "notebuffer.hpp"
#include "preferences.hpp"
#include "tag.hpp"

namespace gnote {

  class Note;

class NoteTextMenu
  : public Gtk::Menu
{
public:
  NoteTextMenu(const Glib::RefPtr<NoteBuffer> & buffer, UndoManager& undo_manager);
  void set_accel_group(const Glib::RefPtr<Gtk::AccelGroup> &);

  static void markup_label (Gtk::MenuItem & item);
protected:
  virtual void on_show();

private:
  void refresh_sizing_state();
  void refresh_state();
  void font_style_clicked(Gtk::CheckMenuItem * item);
  void font_size_activated(Gtk::RadioMenuItem *item);
  void undo_clicked();
  void redo_clicked();
  void undo_changed();
  void toggle_bullets_clicked();
  void increase_indent_clicked();
  void decrease_indent_clicked();
  void increase_font_clicked();
  void decrease_font_clicked();

  Glib::RefPtr<NoteBuffer> m_buffer;
  UndoManager          &m_undo_manager;
  bool                  m_event_freeze;
  Gtk::ImageMenuItem   *m_undo;
  Gtk::ImageMenuItem   *m_redo;
  Gtk::CheckMenuItem    m_bold;
  Gtk::CheckMenuItem    m_italic;
  Gtk::CheckMenuItem    m_strikeout;
  Gtk::CheckMenuItem    m_highlight;
  Gtk::RadioButtonGroup m_fontsize_group;
  Gtk::RadioMenuItem    m_normal;
  Gtk::RadioMenuItem    m_huge;
  Gtk::RadioMenuItem    m_large;
  Gtk::RadioMenuItem    m_small;
  // Active when the text size is indeterminable, such as when in
  // the note's title line.
  Gtk::RadioMenuItem    m_hidden_no_size;
  Gtk::CheckMenuItem    m_bullets;
  Gtk::ImageMenuItem    m_increase_indent;
  Gtk::ImageMenuItem    m_decrease_indent;
  Gtk::MenuItem         m_increase_font;
  Gtk::MenuItem         m_decrease_font;
  sigc::connection      m_bullets_clicked_cid;
};

class NoteFindHandler
{
public:
  NoteFindHandler(Note & );
  void perform_search(const std::string & text);
  bool goto_next_result();
  bool goto_previous_result();
private:
  struct Match
  {
    Glib::RefPtr<NoteBuffer>     buffer;
    Glib::RefPtr<Gtk::TextMark>  start_mark;
    Glib::RefPtr<Gtk::TextMark>  end_mark;
    bool                         highlighting;
  };

  void jump_to_match(const Match & match);
  void perform_search (bool scroll_to_hit);
  void update_sensitivity();
  void update_search();
  void note_changed_timeout();
  void highlight_matches(bool);
  void cleanup_matches();
  void find_matches_in_buffer(const Glib::RefPtr<NoteBuffer> & buffer, 
                              const std::vector<Glib::ustring> & words,
                              std::list<Match> & matches);

  Note           & m_note;
  std::list<Match> m_current_matches;
};

class NoteWindow 
  : public Gtk::Grid
  , public EmbeddableWidget
  , public SearchableItem
  , public HasEmbeddableToolbar
{
public:
  NoteWindow(Note &);
  ~NoteWindow();

  virtual std::string get_name() const;
  void set_name(const std::string & name);
  virtual void foreground();
  virtual void background();
  virtual void hint_position(int & x, int & y);
  virtual void hint_size(int & width, int & height);
  virtual void size_internals();

  virtual void perform_search(const std::string & text);
  virtual bool supports_goto_result();
  virtual bool goto_next_result();
  virtual bool goto_previous_result();

  // use co-variant return
  virtual Gtk::Grid *embeddable_toolbar();

  void set_size(int width, int height)
    {
      m_width = width;
      m_height = height;
    }
  void set_position(int x, int y)
    {
      m_x = x;
      m_y = y;
    }
  Gtk::TextView * editor() const
    {
      return m_editor;
    }
  Gtk::ToolButton * delete_button() const
    {
      return m_delete_button;
    }
  Gtk::Menu * plugin_menu() const
    {
      return m_plugin_menu;
    }
  Gtk::Menu * text_menu() const
    {
      return m_text_menu;
    }
  const Glib::RefPtr<Gtk::AccelGroup> & get_accel_group()
    {
      return m_accel_group;
    }
  NoteFindHandler & get_find_handler()
    {
      return m_find_handler;
    }
private:
  static Glib::RefPtr<Gio::Icon> get_icon_pin_active();
  static Glib::RefPtr<Gio::Icon> get_icon_pin_down();

  void on_delete_button_clicked();
  void on_selection_mark_set(const Gtk::TextIter&, const Glib::RefPtr<Gtk::TextMark>&);
  void update_link_button_sensitivity();
  void on_populate_popup(Gtk::Menu*);
  Gtk::Grid *make_toolbar();
  Gtk::Menu * make_plugin_menu();
  Gtk::Grid * make_template_bar();
  void on_untemplate_button_click();
  void on_save_size_check_button_toggled();
  void on_save_selection_check_button_toggled();
  void on_save_title_check_button_toggled();
  void on_note_tag_added(const Note&, const Tag::Ptr&);
  void on_note_tag_removed(const Note::Ptr&, const std::string&);
  void link_button_clicked();
  void open_help_activate();
  void change_depth_right_handler();
  void change_depth_left_handler();
  void add_accel_group(Gtk::Window &);
  void remove_accel_group(Gtk::Window &);
  void on_pin_status_changed(const Note &, bool);
  void on_pin_button_clicked();

  Note                        & m_note;
  std::string                   m_name;
  int                           m_height;
  int                           m_width;
  int                           m_x;
  int                           m_y;
  Glib::RefPtr<Gtk::AccelGroup> m_accel_group;
  Gtk::Grid                    *m_embeddable_toolbar;
  Gtk::Image                   *m_pin_image;
  Gtk::ToolButton              *m_pin_button;
  Gtk::ToolButton              *m_link_button;
  NoteTextMenu                 *m_text_menu;
  Gtk::Menu                    *m_plugin_menu;
  Gtk::TextView                *m_editor;
  Gtk::ScrolledWindow          *m_editor_window;
  NoteFindHandler              m_find_handler;
  Gtk::ToolButton              *m_delete_button;
  Gtk::Grid                    *m_template_widget;
  Gtk::CheckButton             *m_save_size_check_button;
  Gtk::CheckButton             *m_save_selection_check_button;
  Gtk::CheckButton             *m_save_title_check_button;

  utils::GlobalKeybinder       *m_global_keys;
  utils::InterruptableTimeout  *m_mark_set_timeout;

  Tag::Ptr m_template_tag;
  Tag::Ptr m_template_save_size_tag;
  Tag::Ptr m_template_save_selection_tag;
  Tag::Ptr m_template_save_title_tag;
};





}

#endif
