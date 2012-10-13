/*
 * gnote
 *
 * Copyright (C) 2010-2012 Aurimas Cernius
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



#ifndef __NOTE_RECENT_CHANGES_HPP_
#define __NOTE_RECENT_CHANGES_HPP_

#include <map>
#include <string>

#include <gtkmm/radiomenuitem.h>
#include <gtkmm/applicationwindow.h>

#include "note.hpp"
#include "searchnoteswidget.hpp"
#include "utils.hpp"

namespace gnote {
  class NoteManager;

typedef utils::ForcedPresentWindow NoteRecentChangesParent;

class NoteRecentChanges
  : public NoteRecentChangesParent
  , public std::tr1::enable_shared_from_this<NoteRecentChanges>
  , public utils::EmbedableWidgetHost
{
public:
  typedef std::tr1::shared_ptr<NoteRecentChanges> Ptr;
  typedef std::tr1::weak_ptr<NoteRecentChanges> WeakPtr;

  static Ptr create(NoteManager& m)
    {
      return Ptr(new NoteRecentChanges(m));
    }
  static Ptr get_owning(Gtk::Widget & widget);

  virtual ~NoteRecentChanges();
  void set_search_text(const std::string & value);
  void present_note(const Note::Ptr & note);
  void new_note();

  virtual void embed_widget(utils::EmbedableWidget &);
  virtual void unembed_widget(utils::EmbedableWidget &);
  virtual void foreground_embeded(utils::EmbedableWidget &);
  virtual void background_embeded(utils::EmbedableWidget &);
protected:
  NoteRecentChanges(NoteManager& m);
  virtual void on_show();
private:
  void on_open_note(const Note::Ptr &);
  void on_open_note_new_window(const Note::Ptr &);
  void on_delete_note();
  void on_close_window();
  bool on_delete(GdkEventAny *);
  bool on_key_pressed(GdkEventKey *);
  bool is_foreground(utils::EmbedableWidget &);
  void on_embeded_widget_menu_item_toggled(utils::EmbedableWidget *);

  NoteManager        &m_note_manager;
  SearchNotesWidget   m_search_notes_widget;
  Gtk::VBox           m_content_vbox;
  Gtk::VBox           m_embed_box;
  std::map<utils::EmbedableWidget*, Gtk::RadioMenuItem*> m_embeded_widgets;
  Gtk::Menu          *m_menu;
  Gtk::RadioMenuItem::Group m_tool_menu_group;
};


}

#endif
