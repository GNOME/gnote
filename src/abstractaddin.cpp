


#include "abstractaddin.hpp"

namespace gnote {

  AbstractAddin::AbstractAddin()
    : m_disposing(false)
  {
  }

  AbstractAddin::~AbstractAddin()
  {
  }

  void AbstractAddin::dispose()
  {
    m_disposing = true;
    dispose(true);
  }


  void AbstractAddin::dispose(bool )
  {
  }


}

