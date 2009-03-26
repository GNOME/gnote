


#ifndef __ABSTRACT_ADDIN_HPP_
#define __ABSTRACT_ADDIN_HPP_

#include <sigc++/trackable.h>

namespace gnote {

class AbstractAddin
  : public sigc::trackable
{
public:
  AbstractAddin();
  virtual ~AbstractAddin();

  void dispose();
  bool is_disposing() const
    { return m_disposing; }
protected:
  virtual void dispose(bool disposing) = 0;

private:
  bool m_disposing;
};

}



#endif
