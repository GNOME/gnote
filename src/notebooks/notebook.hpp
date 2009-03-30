

#ifndef __NOTEBOOKS_NOTEBOOK_HPP_
#define __NOTEBOOKS_NOTEBOOK_HPP_

#include <string>
#include <tr1/memory>

#include "tag.hpp"
#include "note.hpp"

namespace gnote {
namespace notebooks {


/// <summary>
/// An object that represents a notebook in Tomboy
/// </summary>
class Notebook 
{
public:
  typedef std::tr1::shared_ptr<Notebook> Ptr;
  static const char * NOTEBOOK_TAG_PREFIX;
  Notebook(const std::string &);
  Notebook(const Tag::Ptr &);
  std::string get_name() const
    { return m_name; }
  void set_name(const std::string &);
  virtual std::string get_normalized_name() const;
  virtual Tag::Ptr    get_tag() const;
  virtual Note::Ptr   get_template_note() const;
  bool contains_note(const Note::Ptr &);
  static std::string normalize(const std::string & s);
////
  virtual ~Notebook()
    {}
private:
  std::string m_name;
  std::string m_normalized_name;
  std::string m_template_note_title;
  Tag::Ptr    m_tag;
};


/// <summary>
/// A notebook of this type is special in the sense that it
/// will not normally be displayed to the user as a notebook
/// but it's used in the Search All Notes Window for special
/// filtering of the notes.
/// </summary>
class SpecialNotebook
  : public Notebook
{
public:
  typedef std::tr1::shared_ptr<SpecialNotebook> Ptr;
protected:
  SpecialNotebook(const std::string &s)
    : Notebook(s)
    {
    }
  virtual Tag::Ptr    get_tag() const;
  virtual Note::Ptr   get_template_note() const;
};


/// <summary>
/// A special notebook that represents really "no notebook" as
/// being selected.  This notebook is used in the Search All
/// Notes Window to allow users to select it at the top of the
/// list so that all notes are shown.
/// </summary>
class AllNotesNotebook
  : public SpecialNotebook
{
public:
  typedef std::tr1::shared_ptr<AllNotesNotebook> Ptr;
  AllNotesNotebook();
  virtual std::string get_normalized_name() const;
};


/// <summary>
/// A special notebook that represents a notebook with notes
/// that are not filed.  This is used in the Search All Notes
/// Window to filter notes that are not placed in any notebook.
/// </summary>
class UnfiledNotesNotebook
  : public SpecialNotebook
{
public:
  typedef std::tr1::shared_ptr<UnfiledNotesNotebook> Ptr;
  UnfiledNotesNotebook();
  virtual std::string get_normalized_name() const;
};
  


}
}

#endif
