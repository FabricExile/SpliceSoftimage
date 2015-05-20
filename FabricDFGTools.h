#ifndef __FabricDFGTools_H_
#define __FabricDFGTools_H_

#include <xsi_x3dobject.h>
#include <xsi_ref.h>
#include <xsi_string.h>

bool dfgTool_FileBrowserJSON(bool isSave, XSI::CString &out_filepath);
int dfgTool_GetRefsAtOps(XSI::X3DObject &in_obj, XSI::CRefArray &out_refs);

#endif
