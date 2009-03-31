


#include "sharp/string.hpp"
#include "notemanager.hpp"
#include "search.hpp"
#include "tagmanager.hpp"
#include "utils.hpp"
#include "sharp/foreach.hpp"

namespace gnote {


  Search::Search(NoteManager & manager)
    : m_manager(manager)
  {
  }


  Search::ResultsPtr Search::search_notes(const std::string & query, bool case_sensitive, 
                                  const notebooks::Notebook::Ptr & selected_notebook)
  {
    std::vector<std::string> words;
    sharp::string_split(words, query, " \t\n");

    // Used for matching in the raw note XML
    std::vector<std::string> encoded_words; 
    sharp::string_split(encoded_words, utils::XmlEncoder::encode (query), " \t\n");
    ResultsPtr temp_matches(new Results);
			
			// Skip over notes that are template notes
    Tag::Ptr template_tag = TagManager::instance().get_or_create_system_tag (TagManager::TEMPLATE_NOTE_SYSTEM_TAG);

    foreach (const Note::Ptr & note, m_manager.get_notes()) {
      // Skip template notes
      if (note->contains_tag (template_tag)) {
        continue;
      }
				
      // Skip notes that are not in the
      // selected notebook
      if (selected_notebook && !selected_notebook->contains_note (note))
        continue;
				
      // Check the note's raw XML for at least one
      // match, to avoid deserializing Buffers
      // unnecessarily.
      if (check_note_has_match (note, encoded_words,
                                case_sensitive)) {
        int match_count =
          find_match_count_in_note (note->text_content(),
                                    words, case_sensitive);
        if (match_count > 0) {
          // TODO: Improve note.GetHashCode()
          temp_matches->insert(std::make_pair(note, match_count));
        }
      }
    }
    return temp_matches;
  }

  bool Search::check_note_has_match(const Note::Ptr & note, 
                                    const std::vector<std::string> & encoded_words,
                                    bool match_case)
  {
    std::string note_text = note->xml_content();
    if (!match_case) {
      note_text = sharp::string_to_lower(note_text);
    }

    foreach (const std::string & word, encoded_words) {
      if (sharp::string_contains(note_text, word) ) {
        continue;
      }
      else {
        return false;
      }
    }

    return true;
  }

  int Search::find_match_count_in_note(std::string note_text, 
                                       const std::vector<std::string> & words,
                                       bool match_case)
  {
    int matches = 0;

    if (!match_case) {
      note_text = sharp::string_to_lower(note_text);
    }
    
    foreach (const std::string & word, words) {
      int idx = 0;
      bool this_word_found = false;

      if (word.empty())
        continue;

      while (true) {
        idx = sharp::string_index_of(note_text, word, idx);

        if (idx == -1) {
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
