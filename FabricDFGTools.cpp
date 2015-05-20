#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>
#include <xsi_command.h>
#include <xsi_argument.h>
#include <xsi_uitoolkit.h>
#include <xsi_selection.h>
#include <xsi_customoperator.h>
#include <xsi_operatorcontext.h>
#include <xsi_comapihandler.h>
#include <xsi_project.h>
#include <xsi_port.h>
#include <xsi_inputport.h>
#include <xsi_outputport.h>
#include <xsi_portgroup.h>
#include <xsi_inputport.h>
#include <xsi_factory.h>
#include <xsi_model.h>
#include <xsi_status.h>
#include <xsi_x3dobject.h>
#include <xsi_kinematics.h>

#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"
#include "FabricDFGTools.h"

using namespace XSI;

// opens a file browser for "*.dfg.json" files.
// params:  isSave        true: save file browser, false: load file browser.
//          out_filepath  complete filepath or "" on abort or error.
// return: true on success, false on abort or error.
bool dfgTool_FileBrowserJSON(bool isSave, XSI::CString &out_filepath)
{
  // init output.
  out_filepath = L"";

  // open file browser.
  CComAPIHandler toolkit;
  toolkit.CreateInstance(L"XSI.UIToolkit");
  CString ext = ".dfg.json";

  CComAPIHandler filebrowser(toolkit.GetProperty(L"FileBrowser"));
  filebrowser.PutProperty(L"InitialDirectory", Application().GetActiveProject().GetPath());
  filebrowser.PutProperty(L"Filter", L"DFG Preset (*" + ext + L")|*" + ext + L"||");

  CValue returnVal;
  filebrowser.Call(isSave ? L"ShowSave" : L"ShowOpen", returnVal);
  CString uiFileName = filebrowser.GetProperty(L"FilePathName").GetAsText();
  if(uiFileName.IsEmpty())
    return false;

  // convert backslashes to slashes.
  for(ULONG i=0;i<uiFileName.Length();i++)
  {
    if(uiFileName.GetSubString(i, 1).IsEqualNoCase(L"\\"))
      out_filepath += L"/";
    else
      out_filepath += uiFileName[i];
  }

  // take care of possible double extension (e.g. "myProfile.gfd.json.gfd.json").
  ULONG dfg1 = out_filepath.ReverseFindString(ext, UINT_MAX);
  if (dfg1 != UINT_MAX)
  {
    ULONG dfg2 = out_filepath.ReverseFindString(ext, dfg1 - 1);
    if (dfg2 != UINT_MAX && dfg2 + ext.Length() == dfg1)
    {
      out_filepath = out_filepath.GetSubString(0, out_filepath.Length() - ext.Length());
    }
  }

  // done.
  return (out_filepath.Length() > 0);
}

// for a given X3DObject fill the CRef array out_refs with all refs
// at "dfgSoftimageOp" custom operators that belong to the X3DObject.
// return value: the amount of custom operators that were found.
int dfgTool_GetRefsAtOps(XSI::X3DObject &in_obj, XSI::CRefArray &out_refs)
{
  // init output.
  out_refs.Clear();

  // fill out_refs.
  if (in_obj.IsValid())
  {
    CRefArray refs = Application().FindObjects(siCustomOperatorID);
    for (int i=0;i<refs.GetCount();i++)
    {
      CustomOperator op(refs[i]);
      if (   op.GetType() == "dfgSoftimageOp"
          && op.GetParent3DObject().GetUniqueName() == in_obj.GetUniqueName())
      {
        out_refs.Add(refs[i]);
      }
    }
  }

  // done.
  return out_refs.GetCount();
}
