/*
 * gnote
 *
 * Copyright (C) 2010-2013 Aurimas Cernius
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




#ifndef __WATCHERS_HPP_
#define __WATCHERS_HPP_

#if HAVE_CONFIG_H
#include <config.h>
#endif

#if FIXED_GTKSPELL
extern "C" {
#include <gtkspell/gtkspell.h>
}
#endif

#include <gdkmm/cursor.h>
#include <gtkmm/textiter.h>
#include <gtkmm/texttag.h>

#include "base/macros.hpp"
#include "noteaddin.hpp"
#include "triehit.hpp"
#include "utils.hpp"

namespace gnote {

  class Preferences;
  class NoteEditor;
  class NoteTag;

  class NoteRenameWatcher
    : public NoteAddin
  {
  public:
    static NoteAddin * create();
    ~NoteRenameWatcher();
    virtual void initialize() override;
    virtual void shutdown() override;
    virtual void on_note_opened() override;

  protected:
    NoteRenameWatcher()
      : m_editing_title(false)
      , m_title_taken_dialog(NULL)
      {}
  private:
    Gtk::TextIter get_title_end() const;
    Gtk::TextIter get_title_start() const;
    bool on_editor_focus_out(GdkEventFocus *);
    void on_mark_set(const Gtk::TextIter &, const Glib::RefPtr<Gtk::TextMark>&);
    void on_insert_text(const Gtk::TextIter &, const Glib::ustring &, int);
    void on_delete_range(const Gtk::TextIter &,const Gtk::TextIter &);
    void update();
    void changed();
    std::string get_unique_untitled();
    bool update_note_title(bool only_warn);
    void show_name_clash_error(const std::string &, bool);
    void on_dialog_response(int);
    void on_window_backgrounded();

    bool                       m_editing_title;
    Glib::RefPtr<Gtk::TextTag> m_title_tag;
    utils::HIGMessageDialog   *m_title_taken_dialog;
  };

#if FIXED_GTKSPELL
  class NoteSpellChecker 
    : public NoteAddin
  {
  public:
    static NoteAddin * create();    
    virtual void initialize() override;
    virtual void shutdown() override;
    virtual void on_note_opened() override;

    static bool gtk_spell_available()
      { return true; }
  protected:
    NoteSpellChecker()
      : m_obj_ptr(NULL)
      {}
  private:
    void attach();
    void detach();
    void on_enable_spellcheck_changed(const Glib::ustring & key);
    void tag_applied(const Glib::RefPtr<const Gtk::TextTag> &,
                     const Gtk::TextIter &, const Gtk::TextIter &);

    GtkSpellChecker *m_obj_ptr;
    sigc::connection  m_tag_applied_cid;
  };
#else
  class NoteSpellChecker 
    : public NoteAddin
  {
  public:
    static NoteAddin * create();    
    virtual void initialize() override;
    virtual void shutdown() override
      {}
    virtual void on_note_opened() override
      {}

    static bool gtk_spell_available()
      { return false; }
  };
#endif


  class NoteUrlWatcher
    : public NoteAddin 
  {
  public:
    static NoteAddin * create();    
    virtual void initialize() override;
    virtual void shutdown() override;
    virtual void on_note_opened() override;

  protected:
    NoteUrlWatcher();
  private:
    std::string get_url(const Gtk::TextIter & start, const Gtk::TextIter & end);
    bool on_url_tag_activated(const NoteEditor &,
                              const Gtk::TextIter &, const Gtk::TextIter &);
    void apply_url_to_block (Gtk::TextIter start, Gtk::TextIter end);
    void on_apply_tag(const Glib::RefPtr<Gtk::TextBuffer::Tag> & tag,
                      const Gtk::TextIter & start, const Gtk::TextIter &end);
    void on_delete_range(const Gtk::TextIter &,const Gtk::TextIter &);
    void on_insert_text(const Gtk::TextIter &, const Glib::ustring &, int);
    bool on_button_press(GdkEventButton *);
    void on_populate_popup(Gtk::Menu *);
    bool on_popup_menu();
    void copy_link_activate();
    void open_link_activate();

    NoteTag::Ptr                m_url_tag;
    Glib::RefPtr<Gtk::TextMark> m_click_mark;
    Glib::RefPtr<Glib::Regex>   m_regex;
    static const char * URL_REGEX;
    static bool  s_text_event_connected;
  };


  class NoteLinkWatcher
    : public NoteAddin
  {
  public:
    static NoteAddin * create();    
    virtual void initialize() override;
    virtual void shutdown() override;
    virtual void on_note_opened() override;

  private:
    bool contains_text(const std::string & text);
    void on_note_added(const Note::Ptr &);
    void on_note_deleted(const Note::Ptr &);
    void on_note_renamed(const Note::Ptr&, const std::string&);
    void do_highlight(const TrieHit<Note::WeakPtr> & , const Gtk::TextIter &,const Gtk::TextIter &);
    void highlight_note_in_block (const Note::Ptr &, const Gtk::TextIter &,
                                  const Gtk::TextIter &);
    void highlight_in_block(const Gtk::TextIter &,const Gtk::TextIter &);
    void unhighlight_in_block(const Gtk::TextIter &,const Gtk::TextIter &);
    void on_delete_range(const Gtk::TextIter &,const Gtk::TextIter &);
    void on_insert_text(const Gtk::TextIter &, const Glib::ustring &, int);
    void on_apply_tag(const Glib::RefPtr<Gtk::TextBuffer::Tag> & tag,
                      const Gtk::TextIter & start, const Gtk::TextIter &end);
    void remove_link_tag(const Glib::RefPtr<Gtk::TextTag> & tag,
                         const Gtk::TextIter & start, const Gtk::TextIter & end);

    bool open_or_create_link(const NoteEditor &, const Gtk::TextIter &,const Gtk::TextIter &);
    bool on_link_tag_activated(const NoteEditor &,
                               const Gtk::TextIter &, const Gtk::TextIter &);

    NoteTag::Ptr m_link_tag;
    NoteTag::Ptr m_broken_link_tag;

    sigc::connection m_on_note_deleted_cid;
    sigc::connection m_on_note_added_cid;
    sigc::connection m_on_note_renamed_cid;
    static bool s_text_event_connected;
  };


  class NoteWikiWatcher
    : public NoteAddin
  {
  public:
    static NoteAddin * create();    
    virtual void initialize() override;
    virtual void shutdown() override;
    virtual void on_note_opened() override;

  protected:
    NoteWikiWatcher()
      : m_regex(Glib::Regex::create(WIKIWORD_REGEX))
      {
      }
  private:
    void on_enable_wikiwords_changed(const Glib::ustring & key);
    void apply_wikiword_to_block (Gtk::TextIter start, Gtk::TextIter end);
    void on_delete_range(const Gtk::TextIter &,const Gtk::TextIter &);
    void on_insert_text(const Gtk::TextIter &, const Glib::ustring &, int);


    static const char * WIKIWORD_REGEX;
    Glib::RefPtr<Gtk::TextTag>   m_broken_link_tag;
    Glib::RefPtr<Glib::Regex>    m_regex;
    sigc::connection    m_on_insert_text_cid;
    sigc::connection    m_on_delete_range_cid;
  };


  class MouseHandWatcher
    : public NoteAddin
  {
  public:
    static NoteAddin * create();    
    virtual void initialize() override;
    virtual void shutdown() override;
    virtual void on_note_opened() override;

  protected:
    MouseHandWatcher()
      : m_hovering_on_link(false)
      {
        _init_static();
      }
  private:
    void _init_static();
    bool on_editor_key_press(GdkEventKey*);
    bool on_editor_key_release(GdkEventKey*);
    bool on_editor_motion(GdkEventMotion *);
    bool m_hovering_on_link;
    static bool s_static_inited;
    static Glib::RefPtr<Gdk::Cursor> s_normal_cursor;
    static Glib::RefPtr<Gdk::Cursor> s_hand_cursor;

  };


  class NoteTagsWatcher 
    : public NoteAddin
  {
  public:
    static NoteAddin * create();
    virtual void initialize() override;
    virtual void shutdown() override;
    virtual void on_note_opened() override;

  private:
    void on_tag_added(const Note&, const Tag::Ptr&);
    void on_tag_removing(const Note&, const Tag &);
    void on_tag_removed(const Note::Ptr&, const std::string&);

    sigc::connection m_on_tag_added_cid;
    sigc::connection m_on_tag_removing_cid;
    sigc::connection m_on_tag_removed_cid;
  };

}


#endif

