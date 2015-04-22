#ifndef __CreationSpliceRenderPass_H_
#define __CreationSpliceRenderPass_H_

#include <FabricCore.h>
#include <xsi_camera.h>

bool isRTRPassEnabled();
void enableRTRPass(bool enable);

void destroyDrawContext();

void pushRefToDelete(const XSI::CRef & ref);

extern FabricCore::RTVal sDrawContext;

#endif