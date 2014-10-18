#ifndef __FabricSplicePlugin_H_ 
#define __FabricSplicePlugin_H_

#include <xsi_string.h>
#include <xsi_status.h>
#include <string>

//todo: group ports for matrices (essential for rigging)

void xsiLogFunc(const char * message, unsigned int length = 0);
void xsiLogErrorFunc(const char * message, unsigned int length = 0);
void xsiClearError();
XSI::CStatus xsiErrorOccured();
void xsiErrorLogEnable(bool enable = true);
void xsiLogFunc(XSI::CString message);
void xsiLogErrorFunc(XSI::CString message);
XSI::CString xsiGetWorkgroupPath();
void xsiInitializeSplice();
XSI::CString xsiGetKLKeyWords();
bool xsiIsLoadingScene();
XSI::CString xsiGetLastLoadedScene();

#endif