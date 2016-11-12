/*
 * gnote
 *
 * Copyright (C) 2010-2015 Aurimas Cernius
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

#include <string.h>

#include <boost/bind.hpp>
#include <boost/format.hpp>

#include <glibmm/i18n.h>
#include <gtkmm/separatormenuitem.h>

#include "sharp/string.hpp"
#include "debug.hpp"
#include "iactionmanager.hpp"
#include "mainwindow.hpp"
#include "noteeditor.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferences.hpp"
#include "itagmanager.hpp"
#include "triehit.hpp"
#include "watchers.hpp"

namespace gnote {


  NoteAddin * NoteRenameWatcher::create() 
  {
    return new NoteRenameWatcher;
  }

  NoteRenameWatcher::~NoteRenameWatcher()
  {
    delete m_title_taken_dialog;
  }

  void NoteRenameWatcher::initialize ()
  {
    m_title_tag = get_note()->get_tag_table()->lookup("note-title");
  }

  void NoteRenameWatcher::shutdown ()
  {
    // Do nothing.
  }

  Gtk::TextIter NoteRenameWatcher::get_title_end() const
  {
    Gtk::TextIter line_end = get_buffer()->begin();
    line_end.forward_to_line_end();
    return line_end;
  }


  Gtk::TextIter NoteRenameWatcher::get_title_start() const
  {
    return get_buffer()->begin();
  }

  
  void NoteRenameWatcher::on_note_opened ()
  {
    const NoteBuffer::Ptr & buffer(get_buffer());

    buffer->signal_mark_set().connect(
      sigc::mem_fun(*this, &NoteRenameWatcher::on_mark_set));
    buffer->signal_insert().connect(
      sigc::mem_fun(*this, &NoteRenameWatcher::on_insert_text));
    buffer->signal_erase().connect(
      sigc::mem_fun(*this, &NoteRenameWatcher::on_delete_range));

    get_window()->editor()->signal_focus_out_event().connect(
      sigc::mem_fun(*this, &NoteRenameWatcher::on_editor_focus_out));
    get_window()->signal_backgrounded.connect(
      sigc::mem_fun(*this, &NoteRenameWatcher::on_window_backgrounded));

    // FIXME: Needed because we hide on delete event, and
    // just hide on accelerator key, so we can't use delete
    // event.  This means the window will flash if closed
    // with a name clash.

    // Clean up title line
    buffer->remove_all_tags (get_title_start(), get_title_end());
    buffer->apply_tag (m_title_tag, get_title_start(), get_title_end());
  }


  bool NoteRenameWatcher::on_editor_focus_out(GdkEventFocus *)
  {
    // TODO: Duplicated from Update(); refactor instead
    if (m_editing_title) {
      changed ();
      update_note_title(false);
      m_editing_title = false;
    }
    return false;
  }


  void NoteRenameWatcher::on_mark_set(const Gtk::TextIter &, 
                                      const Glib::RefPtr<Gtk::TextMark>& mark)
  {
    if (mark == get_buffer()->get_insert()) {
      update ();
    }
  }


  void NoteRenameWatcher::on_insert_text(const Gtk::TextIter & pos, const Glib::ustring &, int)
  {
    update ();

    Gtk::TextIter end = pos;
    end.forward_to_line_end ();

    // Avoid lingering note-title after a multi-line insert...
    get_buffer()->remove_tag (m_title_tag, get_title_end(), end);
      
    //In the case of large copy and paste operations, show the end of the block
    get_window()->editor()->scroll_to (get_buffer()->get_insert());
  }
  

  void NoteRenameWatcher::on_delete_range(const Gtk::TextIter &,const Gtk::TextIter &)
  {
    update();
  }

  void NoteRenameWatcher::update()
  {
    Gtk::TextIter insert = get_buffer()->get_iter_at_mark (get_buffer()->get_insert());
    Gtk::TextIter selection = get_buffer()->get_iter_at_mark (get_buffer()->get_selection_bound());

    // FIXME: Handle middle-click paste when insert or
    // selection isn't on line 0, which means we won't know
    // about the edit.

    if (insert.get_line() == 0 || selection.get_line() == 0) {
      if (!m_editing_title) {
        m_editing_title = true;
      }
      changed ();
    } 
    else {
      if (m_editing_title) {
        changed ();
        update_note_title(false);
        m_editing_title = false;
      }
    }

  }


  void NoteRenameWatcher::changed()
  {
      // Make sure the title line is big and red...
    get_buffer()->remove_all_tags (get_title_start(), get_title_end());
    get_buffer()->apply_tag (m_title_tag, get_title_start(), get_title_end());

    // NOTE: Use "(Untitled #)" for empty first lines...
    std::string title = sharp::string_trim(get_title_start().get_slice (get_title_end()));
    if (title.empty()) {
      title = get_unique_untitled ();
    }
    // Only set window title here, to give feedback that we
    // are indeed changing the title.
    get_window()->set_name(title);
  }


  std::string NoteRenameWatcher::get_unique_untitled()
  {
    int new_num = manager().get_notes().size();
    std::string temp_title;

    while (true) {
      // TRANSLATORS: %1%: boost format placeholder for the number.
      temp_title = str(boost::format(_("(Untitled %1%)")) % ++new_num);
      if (!manager().find (temp_title)) {
        return temp_title;
      }
    }
    return "";
  }


  bool NoteRenameWatcher::update_note_title(bool only_warn)
  {
    std::string title = get_window()->get_name();

    NoteBase::Ptr existing = manager().find (title);
    if (existing && (existing != get_note())) {
      show_name_clash_error (title, only_warn);
      return false;
    }

    DBG_OUT ("Renaming note from %s to %s", get_note()->get_title().c_str(), title.c_str());
    get_note()->set_title(title, true);
    return true;
  }

  void NoteRenameWatcher::show_name_clash_error(const std::string & title, bool only_warn)
  {
    // Select text from TitleStart to TitleEnd
    get_buffer()->move_mark (get_buffer()->get_selection_bound(), get_title_start());
    get_buffer()->move_mark (get_buffer()->get_insert(), get_title_end());

    // TRANSLATORS: %1%: boost format placeholder for the title.
    std::string message = str(boost::format(
                                _("A note with the title "
                                  "<b>%1%</b> already exists. "
                                  "Please choose another name "
                                  "for this note before "
                                  "continuing.")) % title);

    /// Only pop open a warning dialog when one isn't already present
    /// Had to add this check because this method is being called twice.
    if (m_title_taken_dialog == NULL) {
      Gtk::Window *parent = only_warn ? NULL : get_host_window();
      m_title_taken_dialog =
        new utils::HIGMessageDialog (parent,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     Gtk::MESSAGE_WARNING,
                                     Gtk::BUTTONS_OK,
                                     _("Note title taken"),
                                     message);
      m_title_taken_dialog->signal_response().connect(
        sigc::mem_fun(*this, &NoteRenameWatcher::on_dialog_response));
      m_title_taken_dialog->present();
      get_window()->editor()->set_editable(false);
    }
  }


  void NoteRenameWatcher::on_dialog_response(int)
  {
    delete m_title_taken_dialog;
    m_title_taken_dialog = NULL;
    get_window()->editor()->set_editable(true);
  }

  void NoteRenameWatcher::on_window_backgrounded()
  {
    update_note_title(true);
    m_editing_title = false;
  }


  ////////////////////////////////////////////////////////////////////////

  NoteAddin * NoteSpellChecker::create()
  {
    return new NoteSpellChecker();
  }

  
  void NoteSpellChecker::initialize ()
  {
    // Do nothing.
  }


#if FIXED_GTKSPELL
  const char *NoteSpellChecker::LANG_PREFIX = "spellchecklang:";
  const char *NoteSpellChecker::LANG_DISABLED = "disabled";

  void NoteSpellChecker::shutdown ()
  {
    detach();
  }

  void NoteSpellChecker::on_note_opened ()
  {
    Glib::RefPtr<Gio::Settings> settings = Preferences::obj()
      .get_schema_settings(Preferences::SCHEMA_GNOTE);
    settings->signal_changed()
      .connect(sigc::mem_fun(*this, &NoteSpellChecker::on_enable_spellcheck_changed));
    if(settings->get_boolean(Preferences::ENABLE_SPELLCHECKING)) {
      attach ();
    }
    else {
      m_enabled = false;
    }

    NoteWindow *window = get_note()->get_window();
    window->signal_foregrounded.connect(sigc::mem_fun(*this, &NoteSpellChecker::on_note_window_foregrounded));
    window->signal_backgrounded.connect(sigc::mem_fun(*this, &NoteSpellChecker::on_note_window_backgrounded));
  }

  std::map<int, Gtk::Widget*> NoteSpellChecker::get_actions_popover_widgets() const
  {
    std::map<int, Gtk::Widget*> widgets = NoteAddin::get_actions_popover_widgets();
    if(m_enabled) {
      utils::add_item_to_ordered_map(widgets, SPELL_CHECK_ORDER,
        utils::create_popover_button("win.enable-spell-check", _("Check spelling")));
    }
    return widgets;
  }

  void NoteSpellChecker::attach ()
  {
    attach_checker();
    get_note()->get_window()->signal_popover_widgets_changed();
  }


  void NoteSpellChecker::attach_checker()
  {
    // Make sure we add this tag before attaching, so
    // gtkspell will use our version.
    if (!get_note()->get_tag_table()->lookup ("gtkspell-misspelled")) {
      NoteTag::Ptr tag = NoteTag::create ("gtkspell-misspelled", NoteTag::CAN_SPELL_CHECK);
      tag->set_can_serialize(false);
      tag->property_underline() = Pango::UNDERLINE_ERROR;
      get_note()->get_tag_table()->add (tag);
    }

    m_tag_applied_cid = get_buffer()->signal_apply_tag().connect(
      sigc::mem_fun(*this, &NoteSpellChecker::tag_applied), false);  // connect before

    std::string lang = get_language();

    if (!m_obj_ptr && lang != LANG_DISABLED) {
      m_obj_ptr = gtk_spell_checker_new();
      if(lang != "") {
        gtk_spell_checker_set_language(m_obj_ptr, lang.c_str(), NULL);
      }
      g_signal_connect(G_OBJECT(m_obj_ptr), "language-changed", G_CALLBACK(language_changed), this);
      gtk_spell_checker_attach(m_obj_ptr, get_window()->editor()->gobj());
      m_enabled = true;
    }
    else {
      m_enabled = false;
    }
  }


  void NoteSpellChecker::detach ()
  {
    detach_checker();
    m_enabled = false;
    get_note()->get_window()->signal_popover_widgets_changed();
  }


  void NoteSpellChecker::detach_checker()
  {
    m_tag_applied_cid.disconnect();
    
    if(m_obj_ptr) {
      gtk_spell_checker_detach(m_obj_ptr);
      m_obj_ptr = NULL;
    }
  }
  

  void NoteSpellChecker::on_enable_spellcheck_changed(const Glib::ustring & key)
  {
    if (key != Preferences::ENABLE_SPELLCHECKING) {
      return;
    }
    bool value = Preferences::obj()
      .get_schema_settings(Preferences::SCHEMA_GNOTE)->get_boolean(key);
    
    if (value) {
      attach ();
    } 
    else {
      detach ();
    }
  }


  void NoteSpellChecker::tag_applied(const Glib::RefPtr<const Gtk::TextTag> & tag,
                                     const Gtk::TextIter & start_char, 
                                     const Gtk::TextIter & end_char)
  {
    bool remove = false;

    if (tag->property_name() == "gtkspell-misspelled") {
        // Remove misspelled tag for links & title
      Glib::SListHandle<Glib::RefPtr<const Gtk::TextTag> > tag_list = start_char.get_tags();
      for(Glib::SListHandle<Glib::RefPtr<const Gtk::TextTag> >::const_iterator tag_iter = tag_list.begin();
          tag_iter != tag_list.end(); ++tag_iter) {
        const Glib::RefPtr<const Gtk::TextTag>& atag(*tag_iter);
        if ((tag != atag) &&
            !NoteTagTable::tag_is_spell_checkable (atag)) {
          // cancel attempt to add misspelled tag on non-spell-check place
          get_buffer()->signal_apply_tag().emission_stop();
          break;
        }
      }
    } 
    else if (!NoteTagTable::tag_is_spell_checkable (tag)) {
      remove = true;
    }

    if (remove) {
      // adding non-spell-check tag on misspelled text, remove the spell-check first
      get_buffer()->remove_tag_by_name("gtkspell-misspelled",
                               start_char, end_char);
    }
  }

  void NoteSpellChecker::language_changed(GtkSpellChecker*, gchar *lang, NoteSpellChecker *checker)
  {
    try {
      checker->on_language_changed(lang);
    }
    catch(...) {
    }
  }

  void NoteSpellChecker::on_language_changed(const gchar *lang)
  {
    std::string tag_name = LANG_PREFIX;
    tag_name += lang;
    Tag::Ptr tag = get_language_tag();
    if(tag && tag->name() != tag_name) {
      get_note()->remove_tag(tag);
    }
    tag = ITagManager::obj().get_or_create_tag(tag_name);
    get_note()->add_tag(tag);
    DBG_OUT("Added language tag %s", tag_name.c_str());
  }

  Tag::Ptr NoteSpellChecker::get_language_tag()
  {
    Tag::Ptr lang_tag;
    std::list<Tag::Ptr> tags;
    get_note()->get_tags(tags);
    FOREACH(Tag::Ptr tag, tags) {
      if(sharp::string_index_of(tag->name(), LANG_PREFIX) == 0) {
        lang_tag = tag;
        break;
      }
    }
    return lang_tag;
  }

  std::string NoteSpellChecker::get_language()
  {
    Tag::Ptr tag = get_language_tag();
    std::string lang;
    if(tag) {
      lang = sharp::string_replace_first(tag->name(), LANG_PREFIX, "");
    }
    return lang;
  }

  void NoteSpellChecker::on_spell_check_enable_action(const Glib::VariantBase & state)
  {
    Tag::Ptr tag = get_language_tag();
    if(tag) {
      get_note()->remove_tag(tag);
    }
    Glib::Variant<bool> new_state = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(state);
    MainWindow *main_window = dynamic_cast<MainWindow*>(get_note()->get_window()->host());
    MainWindowAction::Ptr enable_action = main_window->find_action("enable-spell-check");
    enable_action->set_state(new_state);
    if(new_state.get()) {
      attach_checker();
    }
    else {
      std::string tag_name = LANG_PREFIX;
      tag_name += LANG_DISABLED;
      tag = ITagManager::obj().get_or_create_tag(tag_name);
      get_note()->add_tag(tag);
      detach_checker();
    }
  }

  void NoteSpellChecker::on_note_window_foregrounded()
  {
    MainWindow *win = dynamic_cast<MainWindow*>(get_note()->get_window()->host());
    MainWindowAction::Ptr enable_action = win->find_action("enable-spell-check");
    enable_action->change_state(Glib::Variant<bool>::create(m_enabled));
    m_enable_cid = enable_action->signal_change_state()
      .connect(sigc::mem_fun(*this, &NoteSpellChecker::on_spell_check_enable_action));
  }

  void NoteSpellChecker::on_note_window_backgrounded()
  {
    m_enable_cid.disconnect();
  }
#endif
  
  ////////////////////////////////////////////////////////////////////////


  const char * NoteUrlWatcher::URL_REGEX = "((\\b((news|http|https|ftp|file|irc|ircs)://|mailto:|(www|ftp)\\.|\\S*@\\S*\\.)|(?<=^|\\s)/\\S+/|(?<=^|\\s)~/\\S+)\\S*\\b/?)";
  bool NoteUrlWatcher::s_text_event_connected = false;
  

  NoteUrlWatcher::NoteUrlWatcher()
    : m_regex(Glib::Regex::create(URL_REGEX, Glib::REGEX_CASELESS))
  {
  }

  NoteAddin * NoteUrlWatcher::create()
  {
    return new NoteUrlWatcher();
  }


  void NoteUrlWatcher::initialize ()
  {
    m_url_tag = get_note()->get_tag_table()->get_url_tag();
  }


  void NoteUrlWatcher::shutdown ()
  {
    // Do nothing.
  }


  void NoteUrlWatcher::on_note_opened ()
  {
    // NOTE: This hack helps avoid multiple URL opens
    // now that Notes always perform
    // TagTable sharing.  This is because if the TagTable is
    // shared, we will connect to the same Tag's event
    // source each time a note is opened, and get called
    // multiple times for each button press.  Fixes bug
    // #305813.
    if (!s_text_event_connected) {
      m_url_tag->signal_activate().connect(
        sigc::mem_fun(*this, &NoteUrlWatcher::on_url_tag_activated));
      s_text_event_connected = true;
    }

    m_click_mark = get_buffer()->create_mark(get_buffer()->begin(), true);

    get_buffer()->signal_insert().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_insert_text));
    get_buffer()->signal_apply_tag().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_apply_tag));
    get_buffer()->signal_erase().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_delete_range));

    Gtk::TextView * editor(get_window()->editor());
    editor->signal_button_press_event().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_button_press), false);
    editor->signal_populate_popup().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_populate_popup));
    editor->signal_popup_menu().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_popup_menu), false);
  }

  std::string NoteUrlWatcher::get_url(const Gtk::TextIter & start, const Gtk::TextIter & end)
  {
    std::string url = start.get_slice (end);

    // FIXME: Needed because the file match is greedy and
    // eats a leading space.
    url = sharp::string_trim(url);

    // Simple url massaging.  Add to 'http://' to the front
    // of www.foo.com, 'mailto:' to alex@foo.com, 'file://'
    // to /home/alex/foo.
    if (Glib::str_has_prefix(url, "www.")) {
      url = "http://" + url;
    }
    else if (Glib::str_has_prefix(url, "/") &&
             sharp::string_last_index_of(url, "/") > 1) {
      url = "file://" + url;
    }
    else if (Glib::str_has_prefix(url, "~/")) {
      const char * home = getenv("HOME");
      if(home) {
        url = std::string("file://") + home + "/" +
          sharp::string_substring(url, 2);
      }
    }
    else if (sharp::string_match_iregex(url, 
                                        "^(?!(news|mailto|http|https|ftp|file|irc):).+@.{2,}$")) {
      url = "mailto:" + url;
    }

    return url;
  }


  bool NoteUrlWatcher::on_url_tag_activated(const NoteEditor &,
                              const Gtk::TextIter & start, const Gtk::TextIter & end)

  {
    std::string url = get_url (start, end);
    try {
      utils::open_url (url);
    } 
    catch (Glib::Error & e) {
      utils::show_opening_location_error (get_host_window(), url, e.what());
    }

    // Kill the middle button paste...
    return true;
  }


  void NoteUrlWatcher::apply_url_to_block (Gtk::TextIter start, Gtk::TextIter end)
  {
    NoteBuffer::get_block_extents(start, end,
                                  256 /* max url length */,
                                  m_url_tag);

    get_buffer()->remove_tag (m_url_tag, start, end);

    Glib::ustring s(start.get_slice(end));
    Glib::MatchInfo match_info;
    while(m_regex->match(s, match_info)) {
      Glib::ustring match = match_info.fetch(0);
      Glib::ustring::size_type start_pos = s.find(match);

      Gtk::TextIter start_cpy = start;
      start_cpy.forward_chars(start_pos);

      Gtk::TextIter end_cpy = start_cpy;
      end_cpy.forward_chars(match.size());

      DBG_OUT("url is %s", start_cpy.get_slice(end_cpy).c_str());
      get_buffer()->apply_tag(m_url_tag, start_cpy, end_cpy);

      start = end_cpy;
      s = start.get_slice(end);
    }
  }


  void NoteUrlWatcher::on_delete_range(const Gtk::TextIter & start, const Gtk::TextIter &end)
  {
    apply_url_to_block(start, end);
  }


  void NoteUrlWatcher::on_insert_text(const Gtk::TextIter & pos, const Glib::ustring &, int len)
  {
    Gtk::TextIter start = pos;
    start.backward_chars (len);

    apply_url_to_block (start, pos);
  }

  void NoteUrlWatcher::on_apply_tag(const Glib::RefPtr<Gtk::TextBuffer::Tag> & tag,
                                    const Gtk::TextIter & start, const Gtk::TextIter & end)
  {
    if(tag != m_url_tag)
      return;
    Glib::ustring s(start.get_slice(end));
    if(!m_regex->match(s)) {
      get_buffer()->remove_tag(m_url_tag, start, end);
    }
  }



  bool NoteUrlWatcher::on_button_press(GdkEventButton *ev)
  {
    int x, y;

    get_window()->editor()->window_to_buffer_coords (Gtk::TEXT_WINDOW_TEXT,
                                                     ev->x, ev->y, x, y);
    Gtk::TextIter click_iter;
    get_window()->editor()->get_iter_at_location (click_iter, x, y);

    // Move click_mark to click location
    get_buffer()->move_mark (m_click_mark, click_iter);

    // Continue event processing
    return false;
  }


  void NoteUrlWatcher::on_populate_popup(Gtk::Menu *menu)
  {
    Gtk::TextIter click_iter = get_buffer()->get_iter_at_mark (m_click_mark);
    if (click_iter.has_tag (m_url_tag) || click_iter.ends_tag (m_url_tag)) {
      Gtk::MenuItem *item;

      item = manage(new Gtk::SeparatorMenuItem ());
      item->show ();
      menu->prepend (*item);

      item = manage(new Gtk::MenuItem (_("_Copy Link Address"), true));
      item->signal_activate().connect(
        sigc::mem_fun(*this, &NoteUrlWatcher::copy_link_activate));
      item->show ();
      menu->prepend (*item);

      item = manage(new Gtk::MenuItem (_("_Open Link"), true));
      item->signal_activate().connect(
        sigc::mem_fun(*this, &NoteUrlWatcher::open_link_activate));
      item->show ();
      menu->prepend (*item);
    }
  }


  bool NoteUrlWatcher::on_popup_menu()
  {
    Gtk::TextIter click_iter = get_buffer()->get_iter_at_mark (get_buffer()->get_insert());
    get_buffer()->move_mark (m_click_mark, click_iter);
    return false;
  }

  void NoteUrlWatcher::open_link_activate()
  {
    Gtk::TextIter click_iter = get_buffer()->get_iter_at_mark (m_click_mark);

    Gtk::TextIter start, end;
    m_url_tag->get_extents (click_iter, start, end);

    on_url_tag_activated(*(NoteEditor*)get_window()->editor(), start, end);
  }


  void NoteUrlWatcher::copy_link_activate()
  {
    Gtk::TextIter click_iter = get_buffer()->get_iter_at_mark (m_click_mark);

    Gtk::TextIter start, end;
    m_url_tag->get_extents (click_iter, start, end);

    std::string url = get_url (start, end);

    Glib::RefPtr<Gtk::Clipboard> clip 
      = get_window()->editor()->get_clipboard ("CLIPBOARD");
    clip->set_text(url);
  }


  ////////////////////////////////////////////////////////////////////////

  bool NoteLinkWatcher::s_text_event_connected = false;

  NoteAddin * NoteLinkWatcher::create()
  {
    return new NoteLinkWatcher;
  }


  void NoteLinkWatcher::initialize ()
  {
    m_on_note_deleted_cid = manager().signal_note_deleted.connect(
      sigc::mem_fun(*this, &NoteLinkWatcher::on_note_deleted));
    m_on_note_added_cid = manager().signal_note_added.connect(
      sigc::mem_fun(*this, &NoteLinkWatcher::on_note_added));
    m_on_note_renamed_cid = manager().signal_note_renamed.connect(
      sigc::mem_fun(*this, &NoteLinkWatcher::on_note_renamed));

    m_link_tag = get_note()->get_tag_table()->get_link_tag();
    m_broken_link_tag = get_note()->get_tag_table()->get_broken_link_tag();
  }


  void NoteLinkWatcher::shutdown ()
  {
    m_on_note_deleted_cid.disconnect();
    m_on_note_added_cid.disconnect();
    m_on_note_renamed_cid.disconnect();
  }


  void NoteLinkWatcher::on_note_opened ()
  {
    // NOTE: This avoid multiple link opens
    // now that notes always perform TagTable
    // sharing.  This is because if the TagTable is shared,
    // we will connect to the same Tag's event source each
    // time a note is opened, and get called multiple times
    // for each button press.  Fixes bug #305813.
    if (!s_text_event_connected) {
      m_link_tag->signal_activate().connect(
        sigc::mem_fun(*this, &NoteLinkWatcher::on_link_tag_activated));
      m_broken_link_tag->signal_activate().connect(
        sigc::mem_fun(*this, &NoteLinkWatcher::on_link_tag_activated));
      s_text_event_connected = true;
    }
    get_buffer()->signal_insert().connect(
      sigc::mem_fun(*this, &NoteLinkWatcher::on_insert_text));
    get_buffer()->signal_apply_tag().connect(
      sigc::mem_fun(*this, &NoteLinkWatcher::on_apply_tag));
    get_buffer()->signal_erase().connect(
      sigc::mem_fun(*this, &NoteLinkWatcher::on_delete_range));
  }

  
  bool NoteLinkWatcher::contains_text(const Glib::ustring & text)
  {
    Glib::ustring body = get_note()->text_content().lowercase();
    Glib::ustring match = text.lowercase();

    return sharp::string_index_of(body, match) > -1;
  }


  void NoteLinkWatcher::on_note_added(const NoteBase::Ptr & added)
  {
    if (added == get_note()) {
      return;
    }

    if (!contains_text (added->get_title())) {
      return;
    }

    // Highlight previously unlinked text
    highlight_in_block (get_buffer()->begin(), get_buffer()->end());
  }

  void NoteLinkWatcher::on_note_deleted(const NoteBase::Ptr & deleted)
  {
    if (deleted == get_note()) {
      return;
    }

    if (!contains_text (deleted->get_title())) {
      return;
    }

    std::string old_title_lower = deleted->get_title().lowercase();

    // Turn all link:internal to link:broken for the deleted note.
    utils::TextTagEnumerator enumerator(get_buffer(), m_link_tag);
    while (enumerator.move_next()) {
      const utils::TextRange & range(enumerator.current());
      if (enumerator.current().text().lowercase() != old_title_lower)
        continue;

      get_buffer()->remove_tag (m_link_tag, range.start(), range.end());
      get_buffer()->apply_tag (m_broken_link_tag, range.start(), range.end());
    }
  }


  void NoteLinkWatcher::on_note_renamed(const NoteBase::Ptr& renamed, const Glib::ustring& /*old_title*/)
  {
    if (renamed == get_note()) {
      return;
    }

    // Highlight previously unlinked text
    if (contains_text (renamed->get_title())) {
      highlight_note_in_block(static_pointer_cast<Note>(renamed), get_buffer()->begin(), get_buffer()->end());
    }
  }

  
  void NoteLinkWatcher::do_highlight(const TrieHit<NoteBase::WeakPtr> & hit,
                                     const Gtk::TextIter & start,
                                     const Gtk::TextIter &)
  {
    // Some of these checks should be replaced with fixes to
    // TitleTrie.FindMatches, probably.
    if (hit.value().expired()) {
      DBG_OUT("DoHighlight: null pointer error for '%s'." , hit.key().c_str());
      return;
    }
      
    if (!manager().find(hit.key())) {
      DBG_OUT ("DoHighlight: '%s' links to non-existing note." ,
               hit.key().c_str());
      return;
    }
      
    NoteBase::Ptr hit_note(hit.value());

    if (hit.key().lowercase() != hit_note->get_title().lowercase()) { // == 0 if same string
      DBG_OUT ("DoHighlight: '%s' links wrongly to note '%s'." ,
               hit.key().c_str(),
               hit_note->get_title().c_str());
      return;
    }
      
    if (hit_note == get_note())
      return;

    Gtk::TextIter title_start = start;
    title_start.forward_chars (hit.start());

    Gtk::TextIter title_end = start;
    title_end.forward_chars (hit.end());

    // Only link against whole words/phrases
    if ((!title_start.starts_word () && !title_start.starts_sentence ()) ||
        (!title_end.ends_word() && !title_end.ends_sentence())) {
      return;
    }

    // Don't create links inside URLs
    if(get_note()->get_tag_table()->has_link_tag(title_start)) {
      return;
    }

    DBG_OUT ("Matching Note title '%s' at %d-%d...",
             hit.key().c_str(), hit.start(), hit.end());

    get_note()->get_tag_table()->foreach(
      boost::bind(sigc::mem_fun(*this, &NoteLinkWatcher::remove_link_tag),
                  _1, title_start, title_end));
    get_buffer()->apply_tag (m_link_tag, title_start, title_end);
  }

  void NoteLinkWatcher::remove_link_tag(const Glib::RefPtr<Gtk::TextTag> & tag,
                                        const Gtk::TextIter & start, const Gtk::TextIter & end)
  {
    NoteTag::Ptr note_tag = NoteTag::Ptr::cast_dynamic(tag);
    if (note_tag && note_tag->can_activate()) {
      get_buffer()->remove_tag(note_tag, start, end);
    }
  }

  void NoteLinkWatcher::highlight_note_in_block (const NoteBase::Ptr & find_note,
                                                 const Gtk::TextIter & start,
                                                 const Gtk::TextIter & end)
  {
    Glib::ustring buffer_text = start.get_text(end).lowercase();
    Glib::ustring find_title_lower = find_note->get_title().lowercase();
    int idx = 0;

    while (true) {
      idx = sharp::string_index_of(buffer_text, find_title_lower, idx);
      if (idx < 0)
        break;

      TrieHit<NoteBase::WeakPtr> hit(idx, idx + find_title_lower.length(),
                             find_title_lower, find_note);
      do_highlight (hit, start, end);

      idx += find_title_lower.length();
    }

  }


  void NoteLinkWatcher::highlight_in_block(const Gtk::TextIter & start,
                                           const Gtk::TextIter & end)
  {
    TrieHit<NoteBase::WeakPtr>::ListPtr hits = manager().find_trie_matches (start.get_slice (end));
    for(TrieHit<NoteBase::WeakPtr>::List::const_iterator iter = hits->begin();
        iter != hits->end(); ++iter) {
      do_highlight (**iter, start, end);
    }
  }

  void NoteLinkWatcher::unhighlight_in_block(const Gtk::TextIter & start,
                                           const Gtk::TextIter & end)
  {
    get_buffer()->remove_tag(m_link_tag, start, end);
  }
  

  void NoteLinkWatcher::on_delete_range(const Gtk::TextIter & s,
                                        const Gtk::TextIter & e)
  {
    Gtk::TextIter start = s;
    Gtk::TextIter end = e;

    NoteBuffer::get_block_extents (start, end,
                                   manager().trie_max_length(),
                                   m_link_tag);

    unhighlight_in_block (start, end);
    highlight_in_block (start, end);
  }
  

  void NoteLinkWatcher::on_insert_text(const Gtk::TextIter & pos, 
                                       const Glib::ustring &, int length)
  {
    Gtk::TextIter start = pos;
    start.backward_chars (length);

    Gtk::TextIter end = pos;

    NoteBuffer::get_block_extents (start, end,
                                   manager().trie_max_length(),
                                   m_link_tag);

    unhighlight_in_block (start, end);
    highlight_in_block (start, end);
  }


  void NoteLinkWatcher::on_apply_tag(const Glib::RefPtr<Gtk::TextBuffer::Tag> & tag,
                                     const Gtk::TextIter & start, const Gtk::TextIter &end)
  {
    if (tag->property_name() != get_note()->get_tag_table()->get_link_tag()->property_name())
      return;
    std::string link_name = start.get_text (end);
    NoteBase::Ptr link = manager().find(link_name);
    if(!link)
        unhighlight_in_block(start, end);
  }


  bool NoteLinkWatcher::open_or_create_link(const NoteEditor &,
                                            const Gtk::TextIter & start,
                                            const Gtk::TextIter & end)
  {
    std::string link_name = start.get_text (end);
    NoteBase::Ptr link = manager().find(link_name);

    if (!link) {
      DBG_OUT("Creating note '%s'...", link_name.c_str());
      try {
        link = manager().create (link_name);
      } 
      catch(...)
      {
        // Fail silently.
      }
    }

    Glib::RefPtr<Gtk::TextTag> broken_link_tag = get_note()->get_tag_table()->get_broken_link_tag();
    if(start.begins_tag(broken_link_tag)) {
      get_note()->get_buffer()->remove_tag(broken_link_tag, start, end);
      get_note()->get_buffer()->apply_tag(get_note()->get_tag_table()->get_link_tag(), start, end);
    }

    // FIXME: We used to also check here for (link != this.Note), but
    // somehow this was causing problems receiving clicks for the
    // wrong instance of a note (see bug #413234).  Since a
    // link:internal tag is never applied around text that's the same
    // as the current note's title, it's safe to omit this check and
    // also works around the bug.
    if (link) {
      DBG_OUT ("Opening note '%s' on click...", link_name.c_str());
      MainWindow::present_default(static_pointer_cast<Note>(link));
      return true;
    }

    return false;
  }

  bool NoteLinkWatcher::on_link_tag_activated(const NoteEditor & editor,
                                              const Gtk::TextIter &start, 
                                              const Gtk::TextIter &end)
  {
    return open_or_create_link(editor, start, end);
  }


  ////////////////////////////////////////////////////////////////////////

  // This is a PCRE regex.
  const char * NoteWikiWatcher::WIKIWORD_REGEX = "\\b((\\p{Lu}+[\\p{Ll}0-9]+){2}([\\p{Lu}\\p{Ll}0-9])*)\\b";


  NoteAddin * NoteWikiWatcher::create()
  {
    return new NoteWikiWatcher();
  }

  void NoteWikiWatcher::initialize ()
  {
    m_broken_link_tag = get_note()->get_tag_table()->get_broken_link_tag();
  }


  void NoteWikiWatcher::shutdown ()
  {
    // Do nothing.
  }


  void NoteWikiWatcher::on_note_opened ()
  {
    get_buffer()->signal_insert().connect(
      sigc::mem_fun(*this, &NoteWikiWatcher::on_insert_text));
    get_buffer()->signal_erase().connect(
      sigc::mem_fun(*this, &NoteWikiWatcher::on_delete_range));
  }


  void NoteWikiWatcher::apply_wikiword_to_block (Gtk::TextIter start, Gtk::TextIter end)
  {
    NoteBuffer::get_block_extents (start,
                                   end,
                                   80 /* max wiki name */,
                                   m_broken_link_tag);

    get_buffer()->remove_tag (m_broken_link_tag, start, end);

    Glib::ustring s(start.get_slice(end));
    Glib::MatchInfo match_info;
    while(m_regex->match(s, match_info)) {
      Glib::ustring match = match_info.fetch(0);
      Glib::ustring::size_type start_pos = s.find(match);

      Gtk::TextIter start_cpy = start;
      start_cpy.forward_chars(start_pos);

      Gtk::TextIter end_cpy = start_cpy;
      end_cpy.forward_chars(match.size());

      if(get_note()->get_tag_table()->has_link_tag(start_cpy)) {
	break;
      }

      DBG_OUT("Highlighting wikiword: '%s' at offset %d",
              start_cpy.get_slice(end_cpy).c_str(), int(start_pos));

      if(!manager().find(match)) {
	get_buffer()->apply_tag (m_broken_link_tag, start_cpy, end_cpy);
      }

      start = end_cpy;
      s = start.get_slice(end);
    }
  }

  void NoteWikiWatcher::on_delete_range(const Gtk::TextIter & start, const Gtk::TextIter & end)
  {
    apply_wikiword_to_block (start, end);
  }


  void NoteWikiWatcher::on_insert_text(const Gtk::TextIter & pos, const Glib::ustring &, 
                                       int length)
  {
    Gtk::TextIter start = pos;
    start.backward_chars(length);
    
    apply_wikiword_to_block (start, pos);
  }

  ////////////////////////////////////////////////////////////////////////

  bool MouseHandWatcher::s_static_inited = false;
  Glib::RefPtr<Gdk::Cursor> MouseHandWatcher::s_normal_cursor;
  Glib::RefPtr<Gdk::Cursor> MouseHandWatcher::s_hand_cursor;

  void MouseHandWatcher::_init_static()
  {
    if(!s_static_inited) {
      s_normal_cursor = Gdk::Cursor::create(Gdk::XTERM);
      s_hand_cursor = Gdk::Cursor::create(Gdk::HAND2);
      s_static_inited = true;
    }
  }


  NoteAddin * MouseHandWatcher::create()
  {
    return new MouseHandWatcher();
  }


  void MouseHandWatcher::initialize ()
  {
    // Do nothing.
    
  }
 
  
  void MouseHandWatcher::shutdown ()
  {
    // Do nothing.
  }
  

  void MouseHandWatcher::on_note_opened ()
  {
    Gtk::TextView *editor = get_window()->editor();
    editor->signal_motion_notify_event()
      .connect(sigc::mem_fun(*this, &MouseHandWatcher::on_editor_motion), false);
    editor->signal_key_press_event()
      .connect(sigc::mem_fun(*this, &MouseHandWatcher::on_editor_key_press), false);
    editor->signal_key_release_event()
      .connect(sigc::mem_fun(*this, &MouseHandWatcher::on_editor_key_release), false);
  }

  bool MouseHandWatcher::on_editor_key_press(GdkEventKey* ev)
  {
    bool retval = false;

    switch (ev->keyval) {
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
    {
      // Control or Shift when hovering over a link
      // swiches to a bar cursor...

      if (!m_hovering_on_link)
        break;

      Glib::RefPtr<Gdk::Window> win = get_window()->editor()->get_window (Gtk::TEXT_WINDOW_TEXT);
      win->set_cursor(s_normal_cursor);
      break;
    }
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    {
      Gtk::TextIter iter = get_buffer()->get_iter_at_mark (get_buffer()->get_insert());

      Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> > tag_list = iter.get_tags();
      for(Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> >::const_iterator tag_iter = tag_list.begin();
          tag_iter != tag_list.end(); ++tag_iter) {
        const Glib::RefPtr<Gtk::TextTag>& tag(*tag_iter);

        if (NoteTagTable::tag_is_activatable (tag)) {
          Glib::RefPtr<Gtk::TextView> editor(get_window()->editor());
          editor->reference();
          retval = tag->event (editor, (GdkEvent*)ev, iter);
          if (retval) {
            break;
          }
        }
      }
      break;
    }
    default:
      break;
    }
    return retval;
  }


  bool MouseHandWatcher::on_editor_key_release(GdkEventKey* ev)
  {
    bool retval = false;
    switch (ev->keyval) {
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
    {
      if (!m_hovering_on_link)
        break;

      Glib::RefPtr<Gdk::Window> win = get_window()->editor()->get_window (Gtk::TEXT_WINDOW_TEXT);
      win->set_cursor(s_hand_cursor);
      break;
    }
    default:
      break;
    }
    return retval;
  }


  bool MouseHandWatcher::on_editor_motion(GdkEventMotion *)
  {
    bool retval = false;

    int pointer_x, pointer_y;
    Gdk::ModifierType pointer_mask;

    get_window()->editor()->Gtk::Widget::get_window()->get_pointer (pointer_x,
                                                                  pointer_y,
                                                                  pointer_mask);

    bool hovering = false;

    // Figure out if we're on a link by getting the text
    // iter at the mouse point, and checking for tags that
    // start with "link:"...

    int buffer_x, buffer_y;
    get_window()->editor()->window_to_buffer_coords (Gtk::TEXT_WINDOW_WIDGET,
                                        pointer_x, pointer_y,
                                                      buffer_x, buffer_y);

    Gtk::TextIter iter;
    get_window()->editor()->get_iter_at_location (iter, buffer_x, buffer_y);

    Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> > tag_list = iter.get_tags();
    for(Glib::SListHandle<Glib::RefPtr<Gtk::TextTag> >::const_iterator tag_iter = tag_list.begin();
        tag_iter != tag_list.end(); ++tag_iter) {
      const Glib::RefPtr<Gtk::TextTag>& tag(*tag_iter);

      if (NoteTagTable::tag_is_activatable (tag)) {
        hovering = true;
        break;
      }
    }

    // Don't show hand if Shift or Control is pressed
    bool avoid_hand = (pointer_mask & (Gdk::SHIFT_MASK |
                                       Gdk::CONTROL_MASK)) != 0;

    if (hovering != m_hovering_on_link) {
      m_hovering_on_link = hovering;

      Glib::RefPtr<Gdk::Window> win = get_window()->editor()->get_window(Gtk::TEXT_WINDOW_TEXT);
      if (hovering && !avoid_hand) {
        win->set_cursor(s_hand_cursor);
      }
      else {
        win->set_cursor(s_normal_cursor);
      }
    }
    return retval;
  }

  ////////////////////////////////////////////////////////////////////////



  NoteAddin * NoteTagsWatcher::create()
  {
    return new NoteTagsWatcher();
  }


  void NoteTagsWatcher::initialize ()
  {
#ifdef DEBUG
    m_on_tag_added_cid = get_note()->signal_tag_added.connect(
      sigc::mem_fun(*this, &NoteTagsWatcher::on_tag_added));
    m_on_tag_removing_cid = get_note()->signal_tag_removing.connect(
      sigc::mem_fun(*this, &NoteTagsWatcher::on_tag_removing));
#endif
    m_on_tag_removed_cid = get_note()->signal_tag_removed.connect(
      sigc::mem_fun(*this, &NoteTagsWatcher::on_tag_removed));      
  }


  void NoteTagsWatcher::shutdown ()
  {
    m_on_tag_added_cid.disconnect();
    m_on_tag_removing_cid.disconnect();
    m_on_tag_removed_cid.disconnect();
  }


  void NoteTagsWatcher::on_note_opened ()
  {
    // FIXME: Just for kicks, spit out the current tags
    DBG_OUT ("%s tags:", get_note()->get_title().c_str());
//    foreach (const Tag::Ptr & tag, get_note()->tags()) {
//      DBG_OUT ("\t%s", tag->name().c_str());
//    }
  }

#ifdef DEBUG
  void NoteTagsWatcher::on_tag_added(const NoteBase& DBG(note), const Tag::Ptr& DBG(tag))
  {
    DBG_OUT ("Tag added to %s: %s", note.get_title().c_str(), tag->name().c_str());
  }


  void NoteTagsWatcher::on_tag_removing(const NoteBase& note, const Tag & tag)
  {
    DBG_OUT ("Removing tag from %s: %s", note.get_title().c_str(), tag.name().c_str());
  }
#endif


  void NoteTagsWatcher::on_tag_removed(const NoteBase::Ptr&, const std::string& tag_name)
  {
    Tag::Ptr tag = ITagManager::obj().get_tag(tag_name);
    DBG_OUT ("Watchers.OnTagRemoved popularity count: %d", tag ? tag->popularity() : 0);
    if (tag && tag->popularity() == 0) {
      ITagManager::obj().remove_tag(tag);
    }
  }

}

