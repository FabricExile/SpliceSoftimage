#ifndef __FabricDFGTools_H_
#define __FabricDFGTools_H_

#include <xsi_x3dobject.h>
#include <xsi_ref.h>
#include <xsi_status.h>
#include <xsi_value.h>
#include <xsi_string.h>

#include <vector>

struct _portMapping;

class dfgTools
{
 public:

  static XSI::CStatus ExecuteCommand(XSI::CString commandName);
  static XSI::CStatus ExecuteCommand(XSI::CString commandName, XSI::CValue arg1);
  static XSI::CStatus ExecuteCommand(XSI::CString commandName, XSI::CValue arg1, XSI::CValue arg2);
  static XSI::CStatus ExecuteCommand(XSI::CString commandName, XSI::CValue arg1, XSI::CValue arg2, XSI::CValue arg3);
  static XSI::CStatus ExecuteCommand(XSI::CString commandName, XSI::CValue arg1, XSI::CValue arg2, XSI::CValue arg3, XSI::CValue arg4);

  static bool FileBrowserJSON(bool isSave, XSI::CString &out_filepath);
  
  static int GetRefsAtOps(XSI::X3DObject &in_obj, XSI::CString &in_opName, XSI::CRefArray &out_refs);
  
  static bool GetOperatorPortMapping(XSI::CRef &in_op, std::vector<_portMapping> &out_pmap, XSI::CString &out_err);
  
  static XSI::siClassID GetSiClassIdFromResolvedDataType(const XSI::CString &resDataType);
  
  static XSI::CString &GetSiClassIdDescription(const XSI::siClassID in_siClassID, XSI::CString &out_description);
};

#endif
