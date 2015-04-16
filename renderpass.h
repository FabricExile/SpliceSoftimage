#ifndef __CreationSpliceRenderPass_H_ 
#define __CreationSpliceRenderPass_H_

#include <FabricCore.h>
#include <xsi_camera.h>

bool isRTRPassEnabled();
void enableRTRPass(bool enable);

void destroyDrawContext();
FabricCore::RTVal & getDrawContext(int viewportWidth, int viewportHeight, XSI::Camera & camera);

void pushRefToDelete(const XSI::CRef & ref);

#endif