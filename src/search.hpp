

#ifndef __SEARCH_HPP_
#define __SEARCH_HPP_

#include <map>
#include <memory>
#include <string>

#include "note.hpp"
#include "notebooks/notebook.hpp"

namespace gnote {

  class NoteManager;

class Search 
{
public:
  typedef std::map<Note::Ptr,int> Results;
  typedef std::tr1::shared_ptr<Results> ResultsPtr;

  Search(NoteManager &);

		
  /// <summary>
  /// Search the notes!
  /// </summary>
  /// <param name="query">
  /// A <see cref="System.String"/>
  /// </param>
  /// <param name="case_sensitive">
  /// A <see cref="System.Boolean"/>
  /// </param>
  /// <param name="selected_notebook">
  /// A <see cref="Notebooks.Notebook"/>.  If this is not
  /// null, only the notes of the specified notebook will
  /// be searched.
  /// </param>
  /// <returns>
  /// A <see cref="IDictionary`2"/>
  /// </returns>  
  ResultsPtr search_notes(const std::string &, bool, 
                          const notebooks::Notebook::Ptr & );
  bool check_note_has_match(const Note::Ptr & note, const std::vector<std::string> & ,
                            bool match_case);
  int find_match_count_in_note(std::string note_text, const std::vector<std::string> &,
                               bool match_case);
private:

  NoteManager &m_manager;
};


}

#endif

