/*
 * gnote
 *
 * Copyright (C) 2011-2017,2019,2021-2024 Aurimas Cernius
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

#include <gtkmm/checkbutton.h>
#include <gtkmm/grid.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/popover.h>
#include <gtkmm/textview.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/shortcutcontroller.h>

#include "mainwindowembeds.hpp"
#include "note.hpp"
#include "undo.hpp"
#include "utils.hpp"
#include "notebuffer.hpp"
#include "preferences.hpp"
#include "tag.hpp"

namespace gnote {

class IconManager;


class NoteTextMenu
  : public Gtk::Popover
{
public:
  NoteTextMenu(EmbeddableWidget & widget, const Glib::RefPtr<NoteBuffer> & buffer, Preferences &prefs);
private:
  void refresh_state(EmbeddableWidget & widget, const Glib::RefPtr<NoteBuffer> & buffer);
  void refresh_sizing_state(EmbeddableWidget & widget, const Glib::RefPtr<NoteBuffer> & buffer);
  Gtk::Widget *create_font_size_item(const char *label, const char *markup, const char *size);
  Gtk::Widget *create_font_item(const char *action, const char *icon_name);
};

class NoteFindHandler
{
public:
  NoteFindHandler(Note & );
  void perform_search(const Glib::ustring & text);
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
                              std::vector<Match> & matches);

  Note           & m_note;
  std::vector<Match> m_current_matches;
};

class NoteWindow 
  : public Gtk::Grid
  , public EmbeddableWidget
  , public SearchableItem
  , public HasEmbeddableToolbar
  , public HasActions
{
public:
  NoteWindow(Note &, IGnote &);
  ~NoteWindow();

  virtual Glib::ustring get_name() const override;
  void set_name(Glib::ustring && name);
  virtual void foreground() override;
  virtual void background() override;
  virtual void size_internals() override;

  virtual void perform_search(const Glib::ustring & text) override;
  virtual bool supports_goto_result() override;
  virtual bool goto_next_result() override;
  virtual bool goto_previous_result() override;

  // use co-variant return
  virtual Gtk::Grid *embeddable_toolbar() override;

  virtual std::vector<PopoverWidget> get_popover_widgets() override;

  void set_size(int width, int height)
    {
      m_width = width;
      m_height = height;
    }
  Gtk::TextView * editor() const
    {
      return m_editor;
    }
  NoteFindHandler & get_find_handler()
    {
      return m_find_handler;
    }
  void enabled(bool enable);
  bool enabled() const
    {
      return m_enabled;
    }
  virtual void set_initial_focus() override;
  Gtk::ShortcutController & shortcut_controller()
    {
      return *m_shortcut_controller;
    }

  sigc::signal<void(NoteTextMenu&)> signal_build_text_menu;
private:
  void connect_actions(EmbeddableWidgetHost *host);
  void disconnect_actions();
  void on_delete_button_clicked(const Glib::VariantBase&);
  Glib::RefPtr<Gio::MenuModel> editor_extra_menu();
  Gtk::Grid *make_toolbar();
  Gtk::Grid * make_template_bar();
  void on_untemplate_button_click();
  void on_save_selection_check_button_toggled();
  void on_save_title_check_button_toggled();
  void on_note_tag_added(const NoteBase&, const Tag&);
  void on_note_tag_removed(const NoteBase&, const Glib::ustring&);
  void link_button_clicked();
  bool open_help_activate(Gtk::Widget&, const Glib::VariantBase&);
  void change_depth_right_handler();
  void change_depth_left_handler();
  void add_shortcuts();
  void on_pin_status_changed(const Note &, bool);
  void on_pin_button_clicked(const Glib::VariantBase & state);
  void on_text_button_clicked(Gtk::Widget*);
  void undo_clicked(const Glib::VariantBase&);
  void redo_clicked(const Glib::VariantBase&);
  void link_clicked(const Glib::VariantBase&);
  void font_style_clicked(const char * tag);
  void bold_clicked(const Glib::VariantBase & state);
  void italic_clicked(const Glib::VariantBase & state);
  void strikeout_clicked(const Glib::VariantBase & state);
  void highlight_clicked(const Glib::VariantBase & state);
  void font_size_activated(const Glib::VariantBase & state);
  void toggle_bullets_clicked(const Glib::VariantBase&);
  void increase_indent_clicked(const Glib::VariantBase&);
  void decrease_indent_clicked(const Glib::VariantBase&);
  bool increase_font_clicked(Gtk::Widget&, const Glib::VariantBase&);
  bool decrease_font_clicked(Gtk::Widget&, const Glib::VariantBase&);
  void undo_changed();
  Tag &template_save_selection_tag();
  Tag &template_save_title_tag();

  Note                        & m_note;
  IGnote                      & m_gnote;
  Glib::ustring                 m_name;
  int                           m_height;
  int                           m_width;
  Gtk::TextView                *m_editor;
  Gtk::ScrolledWindow          *m_editor_window;
  NoteFindHandler              m_find_handler;
  Gtk::Grid                    *m_template_widget;
  Gtk::CheckButton             *m_save_selection_check_button;
  Gtk::CheckButton             *m_save_title_check_button;
  Glib::RefPtr<Gtk::ShortcutController> m_shortcut_controller;

  std::vector<sigc::connection> m_signal_cids;
  bool                         m_enabled;

  Glib::ustring m_template_tag;
  Glib::ustring m_template_save_selection_tag;
  Glib::ustring m_template_save_title_tag;
};





}

#endif
