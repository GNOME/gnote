/*
 * gnote
 *
 * Copyright (C) 2011-2017 Aurimas Cernius
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
#include <gtkmm/checkbutton.h>
#include <gtkmm/grid.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/textview.h>
#include <gtkmm/scrolledwindow.h>

#include "base/macros.hpp"
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
  : public Gtk::PopoverMenu
{
public:
  NoteTextMenu(EmbeddableWidget & widget, const Glib::RefPtr<NoteBuffer> & buffer, UndoManager& undo_manager);
  void set_accels(utils::GlobalKeybinder & keybinder);
  void refresh_state();

  sigc::signal<void, utils::GlobalKeybinder> signal_set_accels;
protected:
  virtual void on_show() override;

private:
  void on_widget_foregrounded();
  void on_widget_backgrounded();
  void refresh_sizing_state();
  void link_clicked();
  void font_clicked(const char *action, const Glib::VariantBase & state, void (NoteTextMenu::*func)());
  void bold_clicked(const Glib::VariantBase & state);
  void bold_pressed();
  void italic_clicked(const Glib::VariantBase & state);
  void italic_pressed();
  void strikeout_clicked(const Glib::VariantBase & state);
  void strikeout_pressed();
  void highlight_clicked(const Glib::VariantBase & state);
  void highlight_pressed();
  Gtk::Widget *create_font_size_item(const char *label, const char *markup, const char *size);
  void font_style_clicked(const char * tag);
  void font_size_activated(const Glib::VariantBase & state);
  void undo_clicked();
  void redo_clicked();
  void undo_changed();
  void toggle_bullets_clicked(const Glib::VariantBase&);
  void increase_indent_clicked(const Glib::VariantBase&);
  void increase_indent_pressed();
  void decrease_indent_clicked(const Glib::VariantBase&);
  void decrease_indent_pressed();
  void increase_font_clicked();
  void decrease_font_clicked();
  Gtk::Widget *create_font_item(const char *action, const char *label, const char *markup);

  EmbeddableWidget     &m_widget;
  Glib::RefPtr<NoteBuffer> m_buffer;
  UndoManager          &m_undo_manager;
  bool                  m_event_freeze;
  std::vector<sigc::connection> m_signal_cids;
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
                              std::list<Match> & matches);

  Note           & m_note;
  std::list<Match> m_current_matches;
};

class NoteWindow 
  : public Gtk::Grid
  , public EmbeddableWidget
  , public SearchableItem
  , public HasEmbeddableToolbar
  , public HasActions
{
public:
  NoteWindow(Note &);
  ~NoteWindow();

  virtual Glib::ustring get_name() const override;
  void set_name(const Glib::ustring & name);
  virtual void foreground() override;
  virtual void background() override;
  virtual void hint_size(int & width, int & height) override;
  virtual void size_internals() override;

  virtual void perform_search(const Glib::ustring & text) override;
  virtual bool supports_goto_result() override;
  virtual bool goto_next_result() override;
  virtual bool goto_previous_result() override;

  // use co-variant return
  virtual Gtk::Grid *embeddable_toolbar() override;

  class NonModifyingNoteAction
  {
  public:
    virtual ~NonModifyingNoteAction() {}

    // these are required to make RefPtr
    virtual void reference() const = 0;
    virtual void unreference() const = 0;
  };
  class NonModifyingAction
    : public Gtk::Action
    , public NonModifyingNoteAction
  {
  public:
    static Glib::RefPtr<NonModifyingAction> create()
      {
        return Glib::RefPtr<NonModifyingAction>(new NonModifyingAction);
      }
    static Glib::RefPtr<NonModifyingAction> create(const Glib::ustring & name,
                                                   const Gtk::StockID & stock_id = Gtk::StockID(),
                                                   const Glib::ustring & label = Glib::ustring(),
                                                   const Glib::ustring & tooltip = Glib::ustring())
      {
        return Glib::RefPtr<NonModifyingAction>(new NonModifyingAction(name, stock_id, label, tooltip));
      }
    static Glib::RefPtr<NonModifyingAction> create(const Glib::ustring & name,
                                                   const Glib::ustring & icon_name,
                                                   const Glib::ustring & label = Glib::ustring(),
                                                   const Glib::ustring & tooltip = Glib::ustring())
      {
        return Glib::RefPtr<NonModifyingAction>(new NonModifyingAction(name, icon_name, label, tooltip));
      }
    NonModifyingAction();
    explicit NonModifyingAction(const Glib::ustring & name, const Gtk::StockID & stock_id = Gtk::StockID(),
                                const Glib::ustring & label = Glib::ustring(),
                                const Glib::ustring & tooltip = Glib::ustring());
    NonModifyingAction(const Glib::ustring & name, const Glib::ustring & icon_name,
                       const Glib::ustring & label = Glib::ustring(),
                       const Glib::ustring & tooltip = Glib::ustring());
    virtual void reference() const override;
    virtual void unreference() const override;
  };
  virtual std::vector<Gtk::Widget*> get_popover_widgets() override;
  virtual std::vector<MainWindowAction::Ptr> get_widget_actions() override;

  void set_size(int width, int height)
    {
      m_width = width;
      m_height = height;
    }
  Gtk::TextView * editor() const
    {
      return m_editor;
    }
  Gtk::PopoverMenu * text_menu() const
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
  void enabled(bool enable);
  bool enabled() const
    {
      return m_enabled;
    }
private:
  static Glib::RefPtr<Gio::Icon> get_icon_pin_active();
  static Glib::RefPtr<Gio::Icon> get_icon_pin_down();

  void on_delete_button_clicked(const Glib::VariantBase&);
  void on_selection_mark_set(const Gtk::TextIter&, const Glib::RefPtr<Gtk::TextMark>&);
  void on_populate_popup(Gtk::Menu*);
  Gtk::Grid *make_toolbar();
  Gtk::Grid * make_template_bar();
  void on_untemplate_button_click();
  void on_save_size_check_button_toggled();
  void on_save_selection_check_button_toggled();
  void on_save_title_check_button_toggled();
  void on_note_tag_added(const NoteBase&, const Tag::Ptr&);
  void on_note_tag_removed(const NoteBase::Ptr&, const Glib::ustring&);
  void link_button_clicked();
  void open_help_activate();
  void change_depth_right_handler();
  void change_depth_left_handler();
  void add_accel_group(Gtk::Window &);
  void remove_accel_group(Gtk::Window &);
  void on_pin_status_changed(const Note &, bool);
  void on_pin_button_clicked(const Glib::VariantBase & state);
  void on_text_button_clicked();

  Note                        & m_note;
  Glib::ustring                 m_name;
  int                           m_height;
  int                           m_width;
  Glib::RefPtr<Gtk::AccelGroup> m_accel_group;
  Gtk::Grid                    *m_embeddable_toolbar;
  NoteTextMenu                 *m_text_menu;
  Gtk::TextView                *m_editor;
  Gtk::ScrolledWindow          *m_editor_window;
  NoteFindHandler              m_find_handler;
  sigc::connection              m_delete_note_slot;
  sigc::connection              m_important_note_slot;
  Gtk::Grid                    *m_template_widget;
  Gtk::CheckButton             *m_save_size_check_button;
  Gtk::CheckButton             *m_save_selection_check_button;
  Gtk::CheckButton             *m_save_title_check_button;

  utils::GlobalKeybinder       *m_global_keys;
  utils::InterruptableTimeout  *m_mark_set_timeout;
  bool                         m_enabled;

  Tag::Ptr m_template_tag;
  Tag::Ptr m_template_save_size_tag;
  Tag::Ptr m_template_save_selection_tag;
  Tag::Ptr m_template_save_title_tag;
};





}

#endif
