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
#include <xsi_parameter.h>
#include <xsi_x3dobject.h>
#include <xsi_kinematics.h>

#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"
#include "FabricDFGTools.h"

using namespace XSI;

// execute an XSI command.
// params:  commandName   name of the command to execute.
//          arg1234..     command arguments.
XSI::CStatus dfgTool_ExecuteCommand(XSI::CString commandName)
{
  CValueArray args;
  return Application().ExecuteCommand(commandName, args, CValue());
}
XSI::CStatus dfgTool_ExecuteCommand(XSI::CString commandName, XSI::CValue arg1)
{
  CValueArray args;
  args.Add(arg1);
  return Application().ExecuteCommand(commandName, args, CValue());
}
XSI::CStatus dfgTool_ExecuteCommand(XSI::CString commandName, XSI::CValue arg1, XSI::CValue arg2)
{
  CValueArray args;
  args.Add(arg1);
  args.Add(arg2);
  return Application().ExecuteCommand(commandName, args, CValue());
}
XSI::CStatus dfgTool_ExecuteCommand(XSI::CString commandName, XSI::CValue arg1, XSI::CValue arg2, XSI::CValue arg3)
{
  CValueArray args;
  args.Add(arg1);
  args.Add(arg2);
  args.Add(arg3);
  return Application().ExecuteCommand(commandName, args, CValue());
}
XSI::CStatus dfgTool_ExecuteCommand(XSI::CString commandName, XSI::CValue arg1, XSI::CValue arg2, XSI::CValue arg3, XSI::CValue arg4)
{
  CValueArray args;
  args.Add(arg1);
  args.Add(arg2);
  args.Add(arg3);
  args.Add(arg4);
  return Application().ExecuteCommand(commandName, args, CValue());
}

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

// for a given X3DObject: fills the CRef array out_refs with all refs
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

// for a given custom operator: allocates and fills the out_* array
// based on the op's DFG, ports, etc.
// returns: true on success, otherwise false plus an error description in out_err.
// note: this function allocates the array out_pm and it is up to the caller to free it.
bool GetOperatorPortMapping(XSI::CRef &in_op, _portMapping *&out_pm, int &out_numPm, XSI::CString &out_err)
{
  // init output.
  out_pm    = NULL;
  out_numPm = 0;
  out_err   = L"";

  // get the operator and its user data.
  if (!in_op.IsValid())
  { out_err = L"in_op is invalid.";
    return false; }
  CustomOperator op(in_op);
  if (!op.IsValid())
  { out_err = L"op is invalid.";
    return false; }
  _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
  if (!pud)
  { out_err = L"no user data found.";
    return false; }

  // check if the Fabric stuff is not valid.
  if (   !pud->GetBaseInterface()
      || !pud->GetBaseInterface()->getGraph())
  { out_err = L"failed to get base interface or graph.";
    return false; }

  // get the DFG ports.
  FabricServices::DFGWrapper::PortList ports = pud->GetBaseInterface()->getGraph()->getPorts();
  if (ports.size() == 0)
    return true;

  // get the op's port groups.
  CRefArray opPortGroups = op.GetPortGroups();

  // allocate and fill out_*.
  out_pm = new _portMapping[ports.size()];
  for (int i=0;i<ports.size();i++)
  {
    // ref at current DFG port.
    FabricServices::DFGWrapper::Port &port = *ports[i];

    // ref at current port mapping.
    _portMapping &pm = out_pm[out_numPm];
    pm.clear();

    // dfg port name.
    pm.dfgPortName = port.getName();

    // dfg port type (in/out)
    if      (port.getPortType() == FabricCore::DFGPortType_In)  pm.dfgPortType = DFG_PORT_TYPE::IN;
    else if (port.getPortType() == FabricCore::DFGPortType_Out) pm.dfgPortType = DFG_PORT_TYPE::OUT;
    else                                                        continue;

    // dfg port data type (resolved type).
    pm.dfgPortDataType = port.getResolvedType();

    // mapping type.
    {
      // init the type with 'internal'.
      pm.mapType = DFG_PORT_MAPTYPE::INTERNAL;

      // process input port.
      if (pm.dfgPortType == DFG_PORT_TYPE::IN)
      {
        // check if the operator has a parameter with the same name as the port.
        Parameter p = op.GetParameter(pm.dfgPortName);
        if (p.IsValid())
        {
          pm.mapType = DFG_PORT_MAPTYPE::XSI_PARAMETER;
        }

        // check if the operator has a XSI input port group with the same name as the port.
        else
        {
          for (int j=0;j<opPortGroups.GetCount();j++)
          {
            PortGroup pg(opPortGroups[j]);
            if (   pg.IsValid()
                && pg.GetName() == L"gIn" + pm.dfgPortName)
            {
              pm.mapType = DFG_PORT_MAPTYPE::XSI_PORT;
              CRefArray pRefs = pg.GetPorts();
              if (pRefs.GetCount() == 1)
              {
                Port p(pRefs[0]);
                if (p.IsValid() && p.IsConnected())
                  pm.mapTarget = p.GetTargetPath();
              }
            }
          }
        }
      }

      // process output port.
      else
      {
      }

      // inc.
      out_numPm++;
    }
  }

  // done.
  return true;
}