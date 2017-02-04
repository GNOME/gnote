/*
 * gnote
 *
 * Copyright (C) 2011,2013-2014,2017 Aurimas Cernius
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




#include "sharp/string.hpp"
#include "notemanager.hpp"
#include "search.hpp"
#include "itagmanager.hpp"
#include "utils.hpp"

namespace gnote {


  Search::Search(NoteManager & manager)
    : m_manager(manager)
  {
  }


  Search::ResultsPtr Search::search_notes(const Glib::ustring & query, bool case_sensitive,
                                  const notebooks::Notebook::Ptr & selected_notebook)
  {
    Glib::ustring search_text = query;
    if(!case_sensitive) {
      search_text = search_text.lowercase();
    }

    std::vector<Glib::ustring> words;
    Search::split_watching_quotes(words, search_text);

    // Used for matching in the raw note XML
    std::vector<Glib::ustring> encoded_words;
    Search::split_watching_quotes(encoded_words, utils::XmlEncoder::encode(search_text));
    ResultsPtr temp_matches(new Results);
      
      // Skip over notes that are template notes
    Tag::Ptr template_tag = ITagManager::obj().get_or_create_system_tag(ITagManager::TEMPLATE_NOTE_SYSTEM_TAG);

    FOREACH(const NoteBase::Ptr & iter, m_manager.get_notes()) {
      Note::Ptr note(static_pointer_cast<Note>(iter));

      // Skip template notes
      if (note->contains_tag (template_tag)) {
        continue;
      }
        
      // Skip notes that are not in the
      // selected notebook
      if (selected_notebook && !selected_notebook->contains_note(note))
        continue;
        
      // First check the note's title for a match,
      // if there is no match check the note's raw
      // XML for at least one match, to avoid
      // deserializing Buffers unnecessarily.
      if (0 < find_match_count_in_note(note->get_title(), words, case_sensitive)) {
        temp_matches->insert(std::make_pair(INT_MAX, note));
      }
      else if (check_note_has_match(note, encoded_words, case_sensitive)) {
        int match_count = find_match_count_in_note(note->text_content(), words, case_sensitive);
        if (match_count > 0) {
          // TODO: Improve note.GetHashCode()
          temp_matches->insert(std::make_pair(match_count, note));
        }
      }
    }
    return temp_matches;
  }

  bool Search::check_note_has_match(const Note::Ptr & note, 
                                    const std::vector<Glib::ustring> & encoded_words,
                                    bool match_case)
  {
    Glib::ustring note_text = note->xml_content();
    if (!match_case) {
      note_text = note_text.lowercase();
    }

    for(auto iter : encoded_words) {
      if(note_text.find(iter) != Glib::ustring::npos) {
        continue;
      }
      else {
        return false;
      }
    }

    return true;
  }

  int Search::find_match_count_in_note(Glib::ustring note_text,
                                       const std::vector<Glib::ustring> & words,
                                       bool match_case)
  {
    int matches = 0;

    if (!match_case) {
      note_text = note_text.lowercase();
    }

    for(auto word : words) {
      Glib::ustring::size_type idx = 0;
      bool this_word_found = false;

      if (word.empty())
        continue;

      while (true) {
        idx = note_text.find(word, idx);

        if (idx == Glib::ustring::npos) {
          if (this_word_found) {
            break;
          }
          else {
            return 0;
          }
        }

        this_word_found = true;

        matches++;

        idx += word.length();
      }
    }

    return matches;
  }


}
