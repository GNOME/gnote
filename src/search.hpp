

#ifndef __SEARCH_HPP_
#define __SEARCH_HPP_

#include <map>
#include "note.hpp"
#include "notebooks/notebook.hpp"

namespace gnote {

  class NoteManager;

class Search 
{
public:
  typedef std::map<Note::Ptr,int> Results;
  Search(NoteManager &)
    {}
  
  Results search_notes(const std::string &, bool, 
                       const notebooks::Notebook::Ptr & )
    {
      return Results();
    }
};


}

#endif

