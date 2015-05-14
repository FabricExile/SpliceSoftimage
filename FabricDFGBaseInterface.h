#ifndef _FabricDFGBaseInterface_H_
#define _FabricDFGBaseInterface_H_

// disable some annoying VS warnings.
#pragma warning(disable : 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc.
#pragma warning(disable : 4800)  // forcing value to bool 'true' or 'false'.
#pragma warning(disable : 4806)  // unsafe operation: no value of type 'bool' promoted to type ...etc.

// includes.
#include <DFGWrapper/DFGWrapper.h>
#include <ASTWrapper/KLASTManager.h>
#include <Commands/CommandStack.h>
#include <map>

// a management class for client and host
class BaseInterface : public FabricServices::DFGWrapper::View
{
 public:

  BaseInterface();
  ~BaseInterface();
};

#endif
