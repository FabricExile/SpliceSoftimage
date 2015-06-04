#ifndef __FabricSpliceOperators_H_
#define __FabricSpliceOperators_H_

#include "FabricSpliceBaseInterface.h"

class opUserData
{
public:
  opUserData(unsigned int objectID);
  FabricSpliceBaseInterface * getInterf();  
  unsigned int getObjectID();
  
private:
  unsigned int _objectID;
  FabricSpliceBaseInterface * _interf;
};

std::vector<opUserData*> getXSIOperatorInstances();

#endif
