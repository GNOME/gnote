/*
 * gnote
 *
 * Copyright (C) 2010-2015,2017,2019-2024 Aurimas Cernius
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

#include <glibmm/i18n.h>
#include <glibmm/stringutils.h>
#include <gtkmm/eventcontrollerfocus.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>

#include "sharp/string.hpp"
#include "debug.hpp"
#include "iactionmanager.hpp"
#include "ignote.hpp"
#include "mainwindow.hpp"
#include "noteeditor.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferences.hpp"
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
    m_title_tag = get_note().get_tag_table()->lookup("note-title");
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

    auto focus_controller = Gtk::EventControllerFocus::create();
    focus_controller->signal_leave().connect(sigc::mem_fun(*this, &NoteRenameWatcher::on_editor_focus_out));
    get_window()->editor()->add_controller(focus_controller);
    get_window()->signal_backgrounded.connect(
      sigc::mem_fun(*this, &NoteRenameWatcher::on_window_backgrounded));

    // Clean up title line
    buffer->remove_all_tags (get_title_start(), get_title_end());
    buffer->apply_tag (m_title_tag, get_title_start(), get_title_end());
  }


  void NoteRenameWatcher::on_editor_focus_out()
  {
    if(m_editing_title) {
      changed();
      update_note_title(false);
      m_editing_title = false;
    }
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
    Glib::ustring title = sharp::string_trim(get_title_start().get_slice(get_title_end()));
    if (title.empty()) {
      title = get_unique_untitled ();
    }
    // Only set window title here, to give feedback that we
    // are indeed changing the title.
    get_window()->set_name(std::move(title));
  }


  Glib::ustring NoteRenameWatcher::get_unique_untitled()
  {
    for(int i = 1;; ++i) {
      // TRANSLATORS: %1 is the placeholder for the number.
      auto temp_title = Glib::ustring::compose(_("(Untitled %1)"), i);
      if(!manager().find(temp_title)) {
        return temp_title;
      }
    }
    return "";
  }


  bool NoteRenameWatcher::update_note_title(bool only_warn)
  {
    Note & note = get_note();
    Glib::ustring title = get_window()->get_name();
    if(note.get_title() == title) {
      return false;
    }

    if(auto n = manager().find(title)) {
      NoteBase &existing = n.value();
      if(&existing != &note) {
        show_name_clash_error(title, only_warn);
      }

      return false;  // same note, same title
    }

    DBG_OUT("Renaming note from '%s' to '%s'", note.get_title().c_str(), title.c_str());
    note.set_title(std::move(title), true);
    return true;
  }

  void NoteRenameWatcher::show_name_clash_error(const Glib::ustring & title, bool only_warn)
  {
    // Select text from TitleStart to TitleEnd
    get_buffer()->move_mark (get_buffer()->get_selection_bound(), get_title_start());
    get_buffer()->move_mark (get_buffer()->get_insert(), get_title_end());

    Glib::ustring message = Glib::ustring::compose(
                                // TRANSLATORS: %1 is the placeholder for the title.
                                _("A note with the title "
                                  "<b>%1</b> already exists. "
                                  "Please choose another name "
                                  "for this note before "
                                  "continuing."), title);

    /// Only pop open a warning dialog when one isn't already present
    /// Had to add this check because this method is being called twice.
    if (m_title_taken_dialog == NULL) {
      Gtk::Window *parent = only_warn ? NULL : get_host_window();
      m_title_taken_dialog =
        new utils::HIGMessageDialog (parent,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     Gtk::MessageType::WARNING,
                                     Gtk::ButtonsType::OK,
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


#if ENABLE_GSPELL
  const char *NoteSpellChecker::LANG_PREFIX = "spellchecklang:";
  const char *NoteSpellChecker::LANG_DISABLED = "disabled";

  void NoteSpellChecker::shutdown ()
  {
    detach();
  }

  void NoteSpellChecker::on_note_opened ()
  {
    ignote().preferences().signal_enable_spellchecking_changed
      .connect(sigc::mem_fun(*this, &NoteSpellChecker::on_enable_spellcheck_changed));
    if(ignote().preferences().enable_spellchecking()) {
      attach ();
    }
    else {
      m_enabled = false;
    }

    NoteWindow *window = get_note()->get_window();
    window->signal_foregrounded.connect(sigc::mem_fun(*this, &NoteSpellChecker::on_note_window_foregrounded));
    window->signal_backgrounded.connect(sigc::mem_fun(*this, &NoteSpellChecker::on_note_window_backgrounded));
  }

  std::vector<gnote::PopoverWidget> NoteSpellChecker::get_actions_popover_widgets() const
  {
    auto widgets = NoteAddin::get_actions_popover_widgets();
    if(m_enabled) {
      auto button = utils::create_popover_button("win.enable-spell-check", _("Check spelling"));
      widgets.push_back(gnote::PopoverWidget(NOTE_SECTION_FLAGS, SPELL_CHECK_ORDER, button));
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
      tag->property_underline() = Pango::Underline::ERROR;
      get_note()->get_tag_table()->add (tag);
    }

    m_tag_applied_cid = get_buffer()->signal_apply_tag().connect(
      sigc::mem_fun(*this, &NoteSpellChecker::tag_applied), false);  // connect before

    Glib::ustring lang = get_language();

    if (!m_obj_ptr && lang != LANG_DISABLED) {
      const GspellLanguage *language = gspell_language_lookup(lang.c_str());
      m_obj_ptr = gspell_checker_new(language);
      g_signal_connect(G_OBJECT(m_obj_ptr), "notify::language", G_CALLBACK(language_changed), this);
      Glib::RefPtr<Gtk::TextBuffer> buffer = get_window()->editor()->get_buffer();
      GspellTextBuffer *gspell_buffer = gspell_text_buffer_get_from_gtk_text_buffer (buffer->gobj());
      gspell_text_buffer_set_spell_checker (gspell_buffer, m_obj_ptr);
      GspellTextView *gspell_view = gspell_text_view_get_from_gtk_text_view (get_window()->editor()->gobj());
      gspell_text_view_set_inline_spell_checking (gspell_view, TRUE);
      gspell_text_view_set_enable_language_menu (gspell_view, TRUE);
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
      Glib::RefPtr<Gtk::TextBuffer> buffer = get_window()->editor()->get_buffer();
      GspellTextBuffer *gspell_buffer = gspell_text_buffer_get_from_gtk_text_buffer (buffer->gobj());
      gspell_text_buffer_set_spell_checker (gspell_buffer, NULL);
      m_obj_ptr = NULL;
    }
  }
  

  void NoteSpellChecker::on_enable_spellcheck_changed()
  {
    bool value = ignote().preferences().enable_spellchecking();
    
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
      for(const auto & atag : start_char.get_tags()) {
        if(tag != atag && !NoteTagTable::tag_is_spell_checkable(atag)) {
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

  void NoteSpellChecker::language_changed(GspellChecker* self, GParamSpec*, NoteSpellChecker *checker)
  {
    try {
      const gchar *lang = gspell_language_get_code (gspell_checker_get_language (self));
      checker->on_language_changed(lang);
    }
    catch(...) {
    }
  }

  void NoteSpellChecker::on_language_changed(const gchar *lang)
  {
    Glib::ustring tag_name = LANG_PREFIX;
    tag_name += lang;
    Tag::Ptr tag = get_language_tag();
    if(tag && tag->name() != tag_name) {
      get_note()->remove_tag(tag);
    }
    tag = manager().tag_manager().get_or_create_tag(tag_name);
    get_note()->add_tag(tag);
    DBG_OUT("Added language tag %s", tag_name.c_str());
  }

  Tag::Ptr NoteSpellChecker::get_language_tag()
  {
    Tag::Ptr lang_tag;
    std::vector<Tag::Ptr> tags = get_note()->get_tags();
    for(Tag::Ptr tag : tags) {
      if(tag->name().find(LANG_PREFIX) == 0) {
        lang_tag = tag;
        break;
      }
    }
    return lang_tag;
  }

  Glib::ustring NoteSpellChecker::get_language()
  {
    Tag::Ptr tag = get_language_tag();
    Glib::ustring lang;
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
      Glib::ustring tag_name = LANG_PREFIX;
      tag_name += LANG_DISABLED;
      tag = manager().tag_manager().get_or_create_tag(tag_name);
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
    : m_regex(Glib::Regex::create(URL_REGEX, Glib::Regex::CompileFlags::CASELESS))
  {
  }

  NoteAddin * NoteUrlWatcher::create()
  {
    return new NoteUrlWatcher();
  }


  void NoteUrlWatcher::initialize ()
  {
    m_url_tag = get_note().get_tag_table()->get_url_tag();
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

    get_buffer()->signal_insert().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_insert_text));
    get_buffer()->signal_apply_tag().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_apply_tag));
    get_buffer()->signal_erase().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_delete_range));
  }

  Glib::ustring NoteUrlWatcher::get_url(const Gtk::TextIter & start, const Gtk::TextIter & end)
  {
    Glib::ustring url = start.get_slice(end);

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
        url = Glib::ustring("file://") + home + "/" +
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
    Glib::ustring url = get_url(start, end);
    try {
      utils::open_url(*get_host_window(), url);
    } 
    catch (Glib::Error & e) {
      utils::show_opening_location_error (get_host_window(), url, e.what());
    }
    catch(const std::exception & e) {
      ERR_OUT("Failed to open URL: %s", e.what());
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

  ////////////////////////////////////////////////////////////////////////

  ApplicationAddin *AppLinkWatcher::create()
  {
    return new AppLinkWatcher;
  }

  AppLinkWatcher::AppLinkWatcher()
    : m_initialized(false)
  {
  }

  void AppLinkWatcher::initialize()
  {
    if(m_initialized) {
      return;
    }
    m_initialized = true;
    m_on_note_deleted_cid = note_manager().signal_note_deleted.connect(
      sigc::mem_fun(*this, &AppLinkWatcher::on_note_deleted));
    m_on_note_added_cid = note_manager().signal_note_added.connect(
      sigc::mem_fun(*this, &AppLinkWatcher::on_note_added));
    m_on_note_renamed_cid = note_manager().signal_note_renamed.connect(
      sigc::mem_fun(*this, &AppLinkWatcher::on_note_renamed));
  }

  void AppLinkWatcher::shutdown()
  {
    m_initialized = false;
    m_on_note_deleted_cid.disconnect();
    m_on_note_added_cid.disconnect();
    m_on_note_renamed_cid.disconnect();
  }

  bool AppLinkWatcher::initialized()
  {
    return m_initialized;
  }

  void AppLinkWatcher::on_note_added(NoteBase & added)
  {
    note_manager().for_each([this, &added](NoteBase & note) {
      if(&added == &note) {
        return;
      }

      if(!contains_text(note, added.get_title())) {
        return;
      }

      // Highlight previously unlinked text
      auto & n = static_cast<Note&>(note);
      auto buffer = n.get_buffer();
      highlight_in_block(note_manager(), n, buffer->begin(), buffer->end());
    });
  }

  void AppLinkWatcher::on_note_deleted(NoteBase & deleted)
  {
    auto tag_table = static_cast<Note&>(deleted).get_tag_table();
    auto link_tag = tag_table->get_link_tag();
    auto broken_link_tag = tag_table->get_broken_link_tag();

    note_manager().for_each([this, &deleted, &link_tag, &broken_link_tag](NoteBase & note) {
      if(&deleted == &note) {
        return;
      }

      if(!contains_text(note, deleted.get_title())) {
        return;
      }

      Glib::ustring old_title_lower = deleted.get_title().lowercase();
      auto buffer = static_cast<Note&>(note).get_buffer();

      // Turn all link:internal to link:broken for the deleted note.
      utils::TextTagEnumerator enumerator(buffer, link_tag);
      while(enumerator.move_next()) {
        const utils::TextRange & range(enumerator.current());
        if(enumerator.current().text().lowercase() != old_title_lower)
          continue;

        buffer->remove_tag(link_tag, range.start(), range.end());
        buffer->apply_tag(broken_link_tag, range.start(), range.end());
      }
    });
  }

  void AppLinkWatcher::on_note_renamed(const NoteBase & renamed, const Glib::ustring & /*old_title*/)
  {
    note_manager().for_each([this, &renamed](NoteBase & note) {
      if(&renamed == &note) {
        return;
      }

      // Highlight previously unlinked text
      if(contains_text(note, renamed.get_title())) {
        auto & n = static_cast<Note&>(note);
        auto buffer = n.get_buffer();
        highlight_note_in_block(note_manager(), n, renamed, buffer->begin(), buffer->end());
      }
    });
  }

  bool AppLinkWatcher::contains_text(const NoteBase & note, const Glib::ustring & text)
  {
    Glib::ustring body = const_cast<NoteBase&>(note).text_content().lowercase();
    Glib::ustring match = text.lowercase();

    return body.find(match) != Glib::ustring::npos;
  }

  void AppLinkWatcher::highlight_in_block(NoteManagerBase & note_manager, Note & note, const Gtk::TextIter & start, const Gtk::TextIter & end)
  {
    for(const auto & hit : note_manager.find_trie_matches(start.get_slice(end))) {
      do_highlight(note_manager, note, hit, start, end);
    }
  }

  void AppLinkWatcher::highlight_note_in_block(NoteManagerBase & note_manager, Note & note, const NoteBase & find_note, const Gtk::TextIter & start, const Gtk::TextIter & end)
  {
    Glib::ustring buffer_text = start.get_text(end).lowercase();
    Glib::ustring find_title_lower = find_note.get_title().lowercase();
    int idx = 0;

    while (true) {
      idx = buffer_text.find(find_title_lower, idx);
      if (idx < 0)
        break;

      auto title_len = find_title_lower.length();
      TrieHit<Glib::ustring> hit(idx, idx + title_len, Glib::ustring(find_title_lower), Glib::ustring(find_note.uri()));
      do_highlight(note_manager, note, hit, start, end);

      idx += title_len;
    }
  }

  void AppLinkWatcher::do_highlight(NoteManagerBase & note_manager, Note & note, const TrieHit<Glib::ustring> & hit, const Gtk::TextIter & start, const Gtk::TextIter &)
  {
    // Some of these checks should be replaced with fixes to
    // TitleTrie.FindMatches, probably.
    auto hit_ref = note_manager.find_by_uri(hit.value());
    if(!hit_ref) {
      DBG_OUT("do_highlight: missing note '%s'." , hit.key().c_str());
      return;
    }
      
    if(!note_manager.find(hit.key())) {
      DBG_OUT("do_highlight: '%s' links to non-existing note." , hit.key().c_str());
      return;
    }
      
    const NoteBase & hit_note = hit_ref.value();

    if(hit.key().lowercase() != hit_note.get_title().lowercase()) { // == 0 if same string
      DBG_OUT("do_highlight: '%s' links wrongly to note '%s'." , hit.key().c_str(), hit_note.get_title().c_str());
      return;
    }
      
    if(&hit_note == &note) {
      return;
    }

    Gtk::TextIter title_start = start;
    title_start.forward_chars(hit.start());

    Gtk::TextIter title_end = start;
    title_end.forward_chars(hit.end());

    // Only link against whole words/phrases
    if((!title_start.starts_word() && !title_start.starts_sentence()) ||
        (!title_end.ends_word() && !title_end.ends_sentence())) {
      return;
    }

    // Don't create links inside URLs
    if(note.get_tag_table()->has_link_tag(title_start)) {
      return;
    }

    DBG_OUT("Matching Note title '%s' at %d-%d...", hit.key().c_str(), hit.start(), hit.end());

    auto tag_table = note.get_tag_table();
    auto link_tag = tag_table->get_link_tag();
    tag_table->foreach([&note, title_start, title_end](const Glib::RefPtr<Gtk::TextTag> & tag) {
      remove_link_tag(note, tag, title_start, title_end);
    });
    note.get_buffer()->apply_tag(link_tag, title_start, title_end);
  }

  void AppLinkWatcher::remove_link_tag(Note & note, const Glib::RefPtr<Gtk::TextTag> & tag, const Gtk::TextIter & start, const Gtk::TextIter & end)
  {
    auto note_tag = std::dynamic_pointer_cast<NoteTag>(tag);
    if(note_tag && note_tag->can_activate()) {
      note.get_buffer()->remove_tag(note_tag, start, end);
    }
  }


  ////////////////////////////////////////////////////////////////////////

  bool NoteLinkWatcher::s_text_event_connected = false;

  NoteAddin * NoteLinkWatcher::create()
  {
    return new NoteLinkWatcher;
  }


  void NoteLinkWatcher::initialize ()
  {
    auto & tag_table = get_note().get_tag_table();
    m_link_tag = tag_table->get_link_tag();
    m_broken_link_tag = tag_table->get_broken_link_tag();
  }


  void NoteLinkWatcher::shutdown ()
  {
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

  void NoteLinkWatcher::highlight_in_block(const Gtk::TextIter & start,
                                           const Gtk::TextIter & end)
  {
    AppLinkWatcher::highlight_in_block(manager(), get_note(), start, end);
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
    Glib::ustring tag_name = tag->property_name();
    Glib::ustring link_tag_name = get_note().get_tag_table()->get_link_tag()->property_name();
    if(tag_name != link_tag_name)
      return;
    Glib::ustring link_name = start.get_text(end);
    auto link = manager().find(link_name);
    if(!link) {
      unhighlight_in_block(start, end);
    }
  }


  bool NoteLinkWatcher::open_or_create_link(const NoteEditor &,
                                            const Gtk::TextIter & start,
                                            const Gtk::TextIter & end)
  {
    Glib::ustring link_name = start.get_text (end);
    auto link = manager().find(link_name);

    if (!link) {
      DBG_OUT("Creating note '%s'...", link_name.c_str());
      try {
        link = std::ref(manager().create(Glib::ustring(link_name)));
      } 
      catch(...)
      {
        // Fail silently.
      }
    }

    Note & note = get_note();
    Glib::RefPtr<Gtk::TextTag> broken_link_tag = note.get_tag_table()->get_broken_link_tag();
    if(start.starts_tag(broken_link_tag)) {
      note.get_buffer()->remove_tag(broken_link_tag, start, end);
      note.get_buffer()->apply_tag(note.get_tag_table()->get_link_tag(), start, end);
    }

    // FIXME: We used to also check here for (link != this.Note), but
    // somehow this was causing problems receiving clicks for the
    // wrong instance of a note (see bug #413234).  Since a
    // link:internal tag is never applied around text that's the same
    // as the current note's title, it's safe to omit this check and
    // also works around the bug.
    if (link) {
      DBG_OUT ("Opening note '%s' on click...", link_name.c_str());
      MainWindow::present_default(ignote(), static_cast<Note&>(link.value().get()));
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
    m_broken_link_tag = get_note().get_tag_table()->get_broken_link_tag();
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

      if(get_note().get_tag_table()->has_link_tag(start_cpy)) {
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
      s_normal_cursor = Gdk::Cursor::create("text");
      s_hand_cursor = Gdk::Cursor::create("pointer");
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
  

  void MouseHandWatcher::on_note_opened()
  {
    Gtk::TextView *editor = get_window()->editor();
    auto motion_ctrl = Gtk::EventControllerMotion::create();
    motion_ctrl->signal_motion()
      .connect(sigc::mem_fun(*this, &MouseHandWatcher::on_editor_motion), false);
    editor->add_controller(motion_ctrl);
    auto & key_ctrl = dynamic_cast<NoteEditor*>(editor)->key_controller();
    key_ctrl.signal_key_pressed()
      .connect(sigc::mem_fun(*this, &MouseHandWatcher::on_editor_key_press), false);
    auto click_ctrl = Gtk::GestureClick::create();
    click_ctrl->set_button(1);
    click_ctrl->signal_released()
      .connect([this, click_ctrl](int, double x, double y) {
        on_button_release(x, y, click_ctrl->get_current_event_state());
      }, false);
    editor->add_controller(click_ctrl);
  }

  bool MouseHandWatcher::on_editor_key_press(guint keyval, guint, Gdk::ModifierType modifier)
  {
    bool retval = false;

    switch(keyval) {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    {
      if(Gdk::ModifierType::CONTROL_MASK == (modifier & Gdk::ModifierType::CONTROL_MASK)) {
        break;
      }

      Gtk::TextIter iter = get_buffer()->get_iter_at_mark(get_buffer()->get_insert());

      for(const auto & tag : iter.get_tags()) {
        if(NoteTagTable::tag_is_activatable(tag)) {
          if(auto note_tag = std::dynamic_pointer_cast<NoteTag>(tag)) {
            retval = note_tag->activate(*dynamic_cast<NoteEditor*>(get_window()->editor()), iter);
            if(retval) {
              break;
            }
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


  void MouseHandWatcher::on_editor_motion(double x, double y)
  {
    bool hovering = false;
    auto editor = get_window()->editor();

    // Figure out if we're on a link by getting the text
    // iter at the mouse point, and checking for tags that
    // start with "link:"...

    int buffer_x, buffer_y;
    editor->window_to_buffer_coords(Gtk::TextWindowType::WIDGET, x, y, buffer_x, buffer_y);

    Gtk::TextIter iter;
    editor->get_iter_at_location(iter, buffer_x, buffer_y);

    for(const auto & tag : iter.get_tags()) {
      if(NoteTagTable::tag_is_activatable(tag)) {
        hovering = true;
        break;
      }
    }

    if(hovering != m_hovering_on_link) {
      m_hovering_on_link = hovering;

      if(hovering) {
        editor->set_cursor(s_hand_cursor);
      }
      else {
        editor->set_cursor(s_normal_cursor);
      }
    }
  }


  void MouseHandWatcher::on_button_release(double x, double y, Gdk::ModifierType state)
  {
    if(Gdk::ModifierType::CONTROL_MASK == (state & Gdk::ModifierType::CONTROL_MASK)) {
      return;
    }
    if(Gdk::ModifierType::SHIFT_MASK == (state & Gdk::ModifierType::SHIFT_MASK)) {
      return;
    }

    auto editor = get_window()->editor();
    int buffer_x, buffer_y;
    editor->window_to_buffer_coords(Gtk::TextWindowType::WIDGET, x, y, buffer_x, buffer_y);
    Gtk::TextIter iter;
    editor->get_iter_at_location(iter, buffer_x, buffer_y);

    for(const auto & tag : iter.get_tags()) {
      if(NoteTagTable::tag_is_activatable(tag)) {
        if(auto note_tag = std::dynamic_pointer_cast<NoteTag>(tag)) {
          if(note_tag->activate(*dynamic_cast<NoteEditor*>(get_window()->editor()), iter)) {
            break;
          }
        }
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////



  NoteAddin * NoteTagsWatcher::create()
  {
    return new NoteTagsWatcher();
  }


  void NoteTagsWatcher::initialize ()
  {
    Note & note = get_note();
#ifdef DEBUG
    m_on_tag_added_cid = note.signal_tag_added.connect(sigc::mem_fun(*this, &NoteTagsWatcher::on_tag_added));
    m_on_tag_removing_cid = note.signal_tag_removing.connect(sigc::mem_fun(*this, &NoteTagsWatcher::on_tag_removing));
#endif
    m_on_tag_removed_cid = note.signal_tag_removed.connect(sigc::mem_fun(*this, &NoteTagsWatcher::on_tag_removed));
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
    DBG_OUT ("%s tags:", get_note().get_title().c_str());
    for(const Tag &tag : get_note().get_tags()) {
      DBG_OUT ("\t%s", tag.name().c_str());
    }
  }

#ifdef DEBUG
  void NoteTagsWatcher::on_tag_added(const NoteBase& DBG(note), const Tag& DBG(tag))
  {
    DBG_OUT ("Tag added to %s: %s", note.get_title().c_str(), tag.name().c_str());
  }


  void NoteTagsWatcher::on_tag_removing(const NoteBase& note, const Tag & tag)
  {
    DBG_OUT ("Removing tag from %s: %s", note.get_title().c_str(), tag.name().c_str());
  }
#endif


  void NoteTagsWatcher::on_tag_removed(const NoteBase&, const Glib::ustring& tag_name)
  {
    auto tag = manager().tag_manager().get_tag(tag_name);
    DBG_OUT ("Watchers.OnTagRemoved popularity count: %d", tag ? tag.value().get().popularity() : 0);
    if(tag && tag.value().get().popularity() == 0) {
      manager().tag_manager().remove_tag(*tag);
    }
  }

}

