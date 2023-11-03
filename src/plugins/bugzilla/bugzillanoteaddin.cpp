/*
 * gnote
 *
 * Copyright (C) 2010,2012-2013,2017,2019,2023 Aurimas Cernius
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



#include <glib.h>
#include <glibmm/miscutils.h>
#include <glibmm/regex.h>

#include "sharp/directory.hpp"

#include "debug.hpp"
#include "ignote.hpp"
#include "notebuffer.hpp"
#include "notewindow.hpp"

#include "bugzillanoteaddin.hpp"
#include "bugzillalink.hpp"
#include "bugzillapreferencesfactory.hpp"
#include "insertbugaction.hpp"

namespace bugzilla {

  DECLARE_MODULE(BugzillaModule);

  BugzillaModule::BugzillaModule()
  {
    ADD_INTERFACE_IMPL(BugzillaNoteAddin);
    ADD_INTERFACE_IMPL(BugzillaPreferencesFactory);
    enabled(false);
  }


  const char * BugzillaNoteAddin::TAG_NAME = "link:bugzilla";

  BugzillaNoteAddin::BugzillaNoteAddin()
    : gnote::NoteAddin()
  {
    const bool is_first_run
                 = !sharp::directory_exists(images_dir());
    const Glib::ustring old_images_dir
      = Glib::build_filename(gnote::IGnote::old_note_dir(),
                             "BugzillaIcons");
    const bool migration_needed
                 = is_first_run
                   && sharp::directory_exists(old_images_dir);

    if (is_first_run)
      g_mkdir_with_parents(images_dir().c_str(), S_IRWXU);

    if (migration_needed)
      migrate_images(old_images_dir);
  }

  Glib::ustring BugzillaNoteAddin::images_dir()
  {
    return Glib::build_filename(gnote::IGnote::conf_dir(),
                                "BugzillaIcons");
  }

  void BugzillaNoteAddin::initialize()
  {
    auto & tag_table = get_note().get_tag_table();
    if(!tag_table->is_dynamic_tag_registered(TAG_NAME)) {
      tag_table->register_dynamic_tag(TAG_NAME, [this]() { return BugzillaLink::create(ignote()); });
    }
  }

  void BugzillaNoteAddin::migrate_images(
                            const Glib::ustring & old_images_dir)
  {
    const Glib::RefPtr<Gio::File> src
      = Gio::File::create_for_path(old_images_dir);
    const Glib::RefPtr<Gio::File> dest
      = Gio::File::create_for_path(gnote::IGnote::conf_dir());

    try {
      sharp::directory_copy(src, dest);
    }
    catch (const std::exception & e) {
      DBG_OUT("BugzillaNoteAddin: migrating images: %s", e.what());
    }
  }


  void BugzillaNoteAddin::shutdown()
  {
  }


  void BugzillaNoteAddin::on_note_opened()
  {
    dynamic_cast<gnote::NoteEditor*>(get_window()->editor())->signal_drop_string
      .connect(sigc::mem_fun(*this, &BugzillaNoteAddin::drop_string));
  }


  bool BugzillaNoteAddin::drop_string(const Glib::ustring & uriString, int x, int y)
  {
    if(uriString.empty()) {
      return false;
    }

    const char * regexString = "\\bhttps?://.*/show_bug\\.cgi\\?(\\S+\\&){0,1}id=(\\d{1,})";

    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(regexString, Glib::Regex::CompileFlags::CASELESS);
    Glib::MatchInfo match_info;

    if(re->match(uriString, match_info) && match_info.get_match_count() >= 3) {
      try {
        int bugId = STRING_TO_INT(match_info.fetch(2));
        insert_bug (x, y, uriString, bugId);
        return true;
      }
      catch(std::bad_cast&)
      {
      }
    }

    return false;
  }


  bool BugzillaNoteAddin::insert_bug(int x, int y, const Glib::ustring & uri, int id)
  {
    try {
      BugzillaLink::Ptr link_tag = std::dynamic_pointer_cast<BugzillaLink>(
        get_note().get_tag_table()->create_dynamic_tag(TAG_NAME));
      link_tag->set_bug_url(uri);

      // Place the cursor in the position where the uri was
      // dropped, adjusting x,y by the TextView's VisibleRect.
      Gdk::Rectangle rect;
      get_window()->editor()->get_visible_rect(rect);
      x = x + rect.get_x();
      y = y + rect.get_y();
      Gtk::TextIter cursor;
      gnote::NoteBuffer::Ptr buffer = get_buffer();
      get_window()->editor()->get_iter_at_location(cursor, x, y);
      buffer->place_cursor (cursor);

      Glib::ustring string_id = TO_STRING(id);
      buffer->undoer().add_undo_action (new InsertBugAction (cursor, 
                                                             string_id, 
                                                             link_tag));

      std::vector<Glib::RefPtr<Gtk::TextTag> > tags;
      tags.push_back(link_tag);
      buffer->insert_with_tags (cursor, 
                                string_id, 
                                tags);
      return true;
    } 
    catch (...)
    {
    }
    return false;
  }

}

