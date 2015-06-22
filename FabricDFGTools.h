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

  // execute an XSI command.
  // params:  commandName   name of the command to execute.
  //          arg1234..     command arguments.
  static XSI::CStatus ExecuteCommand0(XSI::CString commandName);
  static XSI::CStatus ExecuteCommand1(XSI::CString commandName, XSI::CValue arg1);
  static XSI::CStatus ExecuteCommand2(XSI::CString commandName, XSI::CValue arg1, XSI::CValue arg2);
  static XSI::CStatus ExecuteCommand3(XSI::CString commandName, XSI::CValue arg1, XSI::CValue arg2, XSI::CValue arg3);
  static XSI::CStatus ExecuteCommand4(XSI::CString commandName, XSI::CValue arg1, XSI::CValue arg2, XSI::CValue arg3, XSI::CValue arg4);

  // opens a file browser for "*.dfg.json" files.
  // params:  isSave        true: save file browser, false: load file browser.
  //          out_filepath  complete filepath or "" on abort or error.
  // return: true on success, false on abort or error.
  static bool FileBrowserJSON(bool isSave, XSI::CString &out_filepath);
  
  // for a given X3DObject: fills the CRef array out_refs with all refs
  // at in_opName custom operators that belong to the X3DObject.
  // return value: the amount of custom operators that were found.
  static int GetRefsAtOps(XSI::X3DObject &in_obj, XSI::CString &in_opName, XSI::CRefArray &out_refs);
  
  // for a given custom operator: allocates and fills out_pmap based on the op's DFG, ports, etc.
  // returns: true on success, otherwise false plus an error description in out_err.
  static bool GetOperatorPortMapping(XSI::CRef &in_op, std::vector<_portMapping> &out_pmap, XSI::CString &out_err);
  
  // returns the matching siClassID for a given DFG resolved data type.
  static XSI::siClassID GetSiClassIdFromResolvedDataType(const XSI::CString &resDataType);
  
  // gets the description of a siCLassID.
  static XSI::CString &GetSiClassIdDescription(const XSI::siClassID in_siClassID, XSI::CString &out_description);

  // checks the existence of a file.
  // params:  filePath    path and filename
  // returns: true if file exists.
  static bool FileExists(const char *filePath);
};

#endif
