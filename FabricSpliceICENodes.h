#ifndef __FabricSpliceICENodes_H_
#define __FabricSpliceICENodes_H_

#include "FabricSpliceBaseInterface.h"

struct iceNodeUD {
  unsigned int objectID;
  FabricSpliceBaseInterface * interf;
};

extern XSI::CStatus Register_spliceGetData( XSI::PluginRegistrar &in_reg );

#endif
