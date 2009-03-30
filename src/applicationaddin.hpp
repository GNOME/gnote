

#ifndef __APPLICATION_ADDIN_HPP_
#define __APPLICATION_ADDIN_HPP_


#include "abstractaddin.hpp"


namespace gnote {

class ApplicationAddin
  : public AbstractAddin
{
public:
  /// <summary>
  /// Called when Gnote has started up and is nearly 100% initialized.
  /// </summary>
  virtual void initialize () = 0;

  /// <summary>
  /// Called just before Gnote shuts down for good.
  /// </summary>
  virtual void shutdown () = 0;

  /// <summary>
  /// Return true if the addin is initialized
  /// </summary>
  virtual bool initialized () = 0;

};


}

#endif

