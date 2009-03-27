
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/format.hpp>
#include <boost/regex.hpp>

#include <glibmm/i18n.h>

#include "sharp/string.hpp"
#include "debug.hpp"
#include "noteeditor.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferences.hpp"
#include "watchers.hpp"
#include "sharp/foreach.hpp"

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

    // FIXME: Needed because we hide on delete event, and
    // just hide on accelerator key, so we can't use delete
    // event.  This means the window will flash if closed
    // with a name clash.
    get_window()->signal_unmap_event().connect(
      sigc::mem_fun(*this, &NoteRenameWatcher::on_window_closed));

    // Clean up title line
    buffer->remove_all_tags (get_title_start(), get_title_end());
    buffer->apply_tag (m_title_tag, get_title_start(), get_title_end());
  }


  bool NoteRenameWatcher::on_editor_focus_out(GdkEventFocus *)
  {
    // TODO: Duplicated from Update(); refactor instead
    if (m_editing_title) {
      changed ();
      update_note_title ();
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
    get_window()->editor()->scroll_mark_onscreen (get_buffer()->get_insert());
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
        update_note_title ();
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
    get_window()->set_title(title);
  }


  bool NoteRenameWatcher::on_window_closed(GdkEventAny *)
  {
    if (!m_editing_title)
      return false;
    
    if (!update_note_title ()) {
      return true;
    }
    return false;
  }


  std::string NoteRenameWatcher::get_unique_untitled()
  {
    int new_num = manager().get_notes().size();
    std::string temp_title;

    while (true) {
      temp_title = str(boost::format(_("(Untitled %1%)")) % ++new_num);
      if (!manager().find (temp_title)) {
        return temp_title;
      }
    }
    return "";
  }


	bool NoteRenameWatcher::update_note_title()
  {
    std::string title = get_window()->get_title();

    Note::Ptr existing = manager().find (title);
    if (existing && (existing != get_note())) {
      // Present the window in case it got unmapped...
      // FIXME: Causes flicker.
      get_note()->get_window()->present ();

      show_name_clash_error (title);
      return false;
    }

    DBG_OUT ("Renaming note from %1% to %2%", get_note()->title().c_str(), title.c_str());
    get_note()->set_title(title);
    return true;
  }

  void NoteRenameWatcher::show_name_clash_error(const std::string & title)
  {
    // Select text from TitleStart to TitleEnd
    get_buffer()->move_mark (get_buffer()->get_selection_bound(), get_title_start());
    get_buffer()->move_mark (get_buffer()->get_insert(), get_title_end());

    std::string message = str(boost::format(
                                _("A note with the title "
                                  "<b>%1%</b> already exists. "
                                  "Please choose another name "
                                  "for this note before "
                                  "continuing.")) % title);

    /// Only pop open a warning dialog when one isn't already present
    /// Had to add this check because this method is being called twice.
    if (m_title_taken_dialog == NULL) {
      m_title_taken_dialog =
        new utils::HIGMessageDialog (get_window(),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     Gtk::MESSAGE_WARNING,
                                     Gtk::BUTTONS_OK,
                                     _("Note title taken"),
                                     message);
      m_title_taken_dialog->set_modal(true);
      m_title_taken_dialog->signal_response().connect(
        sigc::mem_fun(*this, &NoteRenameWatcher::on_dialog_response));
    }

    m_title_taken_dialog->present ();
  }


  void NoteRenameWatcher::on_dialog_response(int)
  {
    delete m_title_taken_dialog;
    m_title_taken_dialog = NULL;
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


  void NoteSpellChecker::shutdown ()
  {
    // Do nothing.
  }


  void NoteSpellChecker::on_note_opened ()
  {
    Preferences::get_preferences()->signal_setting_changed()
      .connect(sigc::mem_fun(*this, &NoteSpellChecker::on_enable_spellcheck_changed));
    if(Preferences::get_preferences()->get<bool>(Preferences::ENABLE_SPELLCHECKING)) {
      attach ();
    }
  }

  void NoteSpellChecker::attach ()
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
      sigc::mem_fun(*this, &NoteSpellChecker::tag_applied));

    if (!m_obj_ptr) {
      m_obj_ptr = gtkspell_new_attach (get_window()->editor()->gobj(),
                                     NULL,
                                     NULL);
    }
  }


  void NoteSpellChecker::detach ()
  {
    m_tag_applied_cid.disconnect();
    
    if(!m_obj_ptr) {
      gtkspell_detach(m_obj_ptr);
      m_obj_ptr = NULL;
    }
  }
  

  void NoteSpellChecker::on_enable_spellcheck_changed(Preferences *, GConfEntry * entry)
  {
    const char * key = gconf_entry_get_key(entry);
    
    if (strcmp(key, Preferences::ENABLE_SPELLCHECKING) == 0) {
      return;
    }
    GConfValue *value = gconf_entry_get_value(entry);
    
    if (gconf_value_get_bool(value)) {
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
      foreach (const Glib::RefPtr<const Gtk::TextTag> & atag, start_char.get_tags()) {
        if ((tag != atag) &&
            !NoteTagTable::tag_is_spell_checkable (atag)) {
          remove = true;
          break;
        }
      }
    } 
    else if (!NoteTagTable::tag_is_spell_checkable (tag)) {
      remove = true;
    }

    if (remove) {
      get_buffer()->remove_tag_by_name("gtkspell-misspelled",
                               start_char, end_char);
    }
  }

  
  ////////////////////////////////////////////////////////////////////////


  const char * NoteUrlWatcher::URL_REGEX = "((\\b((news|http|https|ftp|file|irc)://|mailto:|(www|ftp)\\.|\\S*@\\S*\\.)|/\\S+/|~/\\S+)\\S*\\b/?)";

  bool NoteUrlWatcher::s_text_event_connected = false;
  

  NoteUrlWatcher::NoteUrlWatcher()
    : m_regex(URL_REGEX, boost::regex::extended|boost::regex_constants::icase)
  {
  }

  NoteAddin * NoteUrlWatcher::create()
  {
    return new NoteUrlWatcher();
  }


  void NoteUrlWatcher::initialize ()
  {
    m_url_tag = NoteTag::Ptr::cast_dynamic(get_note()->get_tag_table()->lookup("link:url"));
  }


  void NoteUrlWatcher::shutdown ()
  {
    // Do nothing.
  }


  void NoteUrlWatcher::on_note_opened ()
  {
#if FIXED_GTKSPELL
    // NOTE: This hack helps avoid multiple URL opens for
    // cases where the GtkSpell version is fixed to allow
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
#else
    m_url_tag->signal_activate().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_url_tag_activated));
#endif

    m_click_mark = get_buffer()->create_mark(get_buffer()->begin(), true);

    get_buffer()->signal_insert().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_insert_text));
    get_buffer()->signal_erase().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_delete_range));

    Gtk::TextView * editor(get_window()->editor());
    editor->signal_button_press_event().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_button_press));
    editor->signal_populate_popup().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_populate_popup));
    editor->signal_popup_menu().connect(
      sigc::mem_fun(*this, &NoteUrlWatcher::on_popup_menu));
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
    if (sharp::string_starts_with(url, "www.")) {
      url = "http://" + url;
    }
    else if (sharp::string_starts_with(url, "/") &&
             sharp::string_last_index_of(url, "/") > 1) {
      url = "file://" + url;
    }
    else if (sharp::string_starts_with(url, "~/")) {
      const char * home = getenv("HOME");
      if(home) {
        url = std::string("file://") + home + sharp::string_substring(url, 2);
      }
    }
    else if (sharp::string_match_iregex(url, 
                                        "^(?!(news|mailto|http|https|ftp|file|irc):).+@.{2,}$")) {
      url = "mailto:" + url;
    }

    return url;
  }

  void NoteUrlWatcher::open_url(const std::string & url)
    throw (Glib::Error)
  {
    if(!url.empty()) {
      GError *err = NULL;
      DBG_OUT("Opening url '%s'...", url.c_str());
      gtk_show_uri (NULL, url.c_str(), GDK_CURRENT_TIME, &err);
      if(err) {
        throw Glib::Error(err, true);
      }
    }
  }


  bool NoteUrlWatcher::on_url_tag_activated(const NoteTag::Ptr &, const NoteEditor &,
                              const Gtk::TextIter & start, const Gtk::TextIter & end)

  {
    std::string url = get_url (start, end);
    try {
      open_url (url);
    } 
    catch (Glib::Error & e) {
      utils::show_opening_location_error (get_window(), url, e.what());
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

    boost::match_results<std::string::const_iterator> m;
    std::string s(start.get_slice(end));
    DBG_OUT("matching %s with %s", s.c_str(), URL_REGEX);
    DBG_OUT("mark count %d", m_regex.mark_count());
    boost::regex_match(s, m, m_regex);
    DBG_OUT("# of matches %d", m.size());
    int count = 0;
    foreach(const boost::sub_match<std::string::const_iterator> & match, m) {
      if(!match.matched) {
        DBG_OUT("No match");
        continue;
      }
//      Match match = regex.Match (start.GetSlice (end));
//         match.Success;
//         match = match.NextMatch ()) {
//      System.Text.RegularExpressions.Group group = match.Groups [1];
      DBG_OUT("match %d len=%d", count, match.length());
      DBG_OUT("matched ='%s'", match.str().c_str());
      /*
        Logger.Log ("Highlighting url: '{0}' at offset {1}",
        group,
        group.Index);
      */

      Gtk::TextIter start_cpy = start;
      start_cpy.forward_chars (match.first - s.begin());

      end = start_cpy;
      end.forward_chars (match.length());

      std::string debug = start_cpy.get_slice(end);
      DBG_OUT("url is %s", debug.c_str());
      get_buffer()->apply_tag (m_url_tag, start_cpy, end);
      count++;
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


  bool NoteUrlWatcher::on_button_press(GdkEventButton *ev)
  {
    int x, y;

    get_window()->editor()->window_to_buffer_coords (Gtk::TEXT_WINDOW_TOP,
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

      item = manage(new Gtk::MenuItem (_("_Copy Link Address")));
      item->signal_activate().connect(
        sigc::mem_fun(*this, &NoteUrlWatcher::copy_link_activate));
      item->show ();
      menu->prepend (*item);

      item = manage(new Gtk::MenuItem (_("_Open Link")));
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

    on_url_tag_activated (m_url_tag, *(NoteEditor*)get_window()->editor(), start, end);
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


}

