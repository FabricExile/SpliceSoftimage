#ifndef __FabricDFGOperators_H_
#define __FabricDFGOperators_H_

#include "FabricDFGBaseInterface.h"

struct _userData
{
  BaseInterface *baseInterface;

  // constructor.
  _userData(void)
  {
    memset(this, NULL, sizeof(*this));
  }

  // destructor.
  ~_userData()
  {
    if (baseInterface)
    {
      delete baseInterface;
    }
  }
};

#endif
