

#ifndef __NOTEBOOKS_NOTEBOOK_HPP_
#define __NOTEBOOKS_NOTEBOOK_HPP_


#include <tr1/memory>

#include "tag.hpp"

namespace gnote {
namespace notebooks {

class Notebook 
{
public:
  typedef std::tr1::shared_ptr<Notebook> Ptr;
  virtual ~Notebook()
    {}

//
  std::string get_name()
    { return ""; }
  Tag::Ptr    get_tag()
    { return Tag::Ptr(); }
  Note::Ptr   get_template_note()
    { return Note::Ptr(); }
};


class SpecialNotebook
  : public Notebook
{
public:
  typedef std::tr1::shared_ptr<SpecialNotebook> Ptr;

};

class UnfiledNotesNotebook
  : public SpecialNotebook
{
public:
  typedef std::tr1::shared_ptr<UnfiledNotesNotebook> Ptr;

};
  
class AllNotesNotebook
  : public SpecialNotebook
{
public:
  typedef std::tr1::shared_ptr<AllNotesNotebook> Ptr;

};


}
}

#endif
