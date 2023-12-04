/*
 * gnote
 *
 * Copyright (C) 2011,2013-2014,2017,2019,2023 Aurimas Cernius
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



#ifndef __SEARCH_HPP_
#define __SEARCH_HPP_

#include <map>
#include <vector>

#include "note.hpp"
#include "notebooks/notebook.hpp"
#include "sharp/string.hpp"

namespace gnote {

  class NoteManager;

class Search 
{
public:
  typedef std::multimap<int, NoteBase::Ref> Results;

  template<typename T>
  static void split_watching_quotes(std::vector<T> & split,
                                    const T & source);

  Search(NoteManagerBase &);

    
  /// Search the notes! A match number of
  /// INT_MAX indicates that the note
  /// title contains the search term.
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
  /// A <see cref="IDictionary`2"/> with the relevant Notes
  /// and a match number. If the search term is in the title,
  /// number will be INT_MAX.
  /// </returns>  
  Results search_notes(const Glib::ustring &, bool, notebooks::Notebook::ORef);
  bool check_note_has_match(const NoteBase & note, const std::vector<Glib::ustring> &, bool match_case);
  int find_match_count_in_note(Glib::ustring note_text, const std::vector<Glib::ustring> &,
                               bool match_case);
private:

  NoteManagerBase & m_manager;
};

template<typename T>
void Search::split_watching_quotes(std::vector<T> & split,
                                   const T & source)
{
  sharp::string_split(split, source, "\"");

  std::vector<T> tmp;

  for (typename std::vector<T>::iterator i = split.begin();
       split.end() != i;
       i++) {
    const T & part = *i;
    std::vector<T> parts;
    sharp::string_split(parts, part, " \t\n");

    for (typename std::vector<T>::const_iterator j = parts.begin();
         parts.end() != j;
         j++) {
      const T & s = *j;
      if (!s.empty())
        tmp.push_back(s);
    }

    i = split.erase(i);
    if (split.end() == i)
      break;
  }

  split.insert(split.end(), tmp.begin(), tmp.end());
}

}

#endif

