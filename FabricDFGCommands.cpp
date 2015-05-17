#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>
#include <xsi_command.h>
#include <xsi_argument.h>
#include <xsi_uitoolkit.h>
#include <xsi_selection.h>
#include <xsi_customoperator.h>
#include <xsi_operatorcontext.h>
#include <xsi_portgroup.h>
#include <xsi_inputport.h>
#include <xsi_factory.h>
#include <xsi_model.h>
#include <xsi_x3dobject.h>
#include <xsi_kinematics.h>

#include "FabricDFGPlugin.h"
#include "FabricDFGBaseInterface.h"

using namespace XSI;

// ---
// command "dfgSoftimageOpApply".
// ---

SICALLBACK dfgSoftimageOpApply_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.PutDescription(L"applies a dfgSoftimageOp operator.");
  oCmd.SetFlag(siNoLogging, false);
  oCmd.EnableReturnValue(false) ;

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"ObjName", CString());
  oArgs.Add(L"OpenPPG", false);

  return CStatus::OK;
}

SICALLBACK dfgSoftimageOpApply_Execute(CRef &in_ctxt)
{
  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");
  if (args.GetCount() < 2 || CString(args[0]).IsEmpty())
  { Application().LogMessage(L"apply dfgSoftimageOp operator failed: empty or missing argument(s)", siErrorMsg);
    return CStatus::OK; }
  CString objName(args[0]);
  bool    openPPG = args[1];

  // log.
  Application().LogMessage(L"applying a  \"dfgSoftimageOp\" custom operator to \"" + objName + L"\"", siVerboseMsg);

  // get target X3DObject.
  CRefArray objRefArray = Application().GetActiveSceneRoot().FindChildren2(objName, L"", CStringArray());
  if (objRefArray.GetCount() <= 0)
  { Application().LogMessage(L"failed to find an object called \"" + objName + L"\" in the scene", siErrorMsg);
    return CStatus::OK; }
  X3DObject obj(objRefArray[0]);
  if (!obj.IsValid())
  { Application().LogMessage(L"failed to create X3DObject for \"" + objName + L"\"", siErrorMsg);
    return CStatus::OK; }

  // create the operator
  CString opName = L"dfgSoftimageOp";
  CustomOperator newOp = Application().GetFactory().CreateObject(opName);

  // create ports.
  {
    CStatus returnStatus;

    // output ports.
    {
      // create the default output port "internalMatrixOut" and connect the object's global kinematics to it.
      newOp.AddOutputPort(obj.GetKinematics().GetGlobal().GetRef(), L"internalMatrixOut", -1, -1,  siDefaultPort, &returnStatus);
      if (returnStatus != CStatus::OK)
      { Application().LogMessage(L"failed to create default output port for the global kinematics", siErrorMsg);
        return CStatus::OK; }
    }

    // input ports.
    {
      newOp.AddInputPort(obj.GetKinematics().GetGlobal().GetRef(), L"internalMatrixIn", -1, -1,  siDefaultPort, &returnStatus);
      if (returnStatus != CStatus::OK)
      { Application().LogMessage(L"failed to create default input port for the global kinematics", siErrorMsg);
        return CStatus::OK; }
    }
  }

  // connect the operator.
  if (newOp.Connect() != CStatus::OK)
  { Application().LogMessage(L"newOp.Connect() failed.",siErrorMsg);
    return CStatus::OK; }

  // display operator's property page?
  if (openPPG)
  {
    CValueArray a;
    a.Add(newOp.GetRef().GetAsText());
    Application().ExecuteCommand(L"InspectObj", a, CValue());
  }

  // done.
  return CStatus::OK;
}

// ---
// command "dfgLogStatus".
// ---

SICALLBACK dfgLogStatus_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.PutDescription(L"logs the plugin/Fabric version and some info in the history log.");
  oCmd.SetFlag(siNoLogging, false);
  oCmd.EnableReturnValue(false) ;

  ArgumentArray oArgs = oCmd.GetArguments();

  return CStatus::OK;
}

SICALLBACK dfgLogStatus_Execute(CRef &in_ctxt)
{
  CString s = L"   Fabric Engine Plugin, Fabric Core v. " + CString(FabricCore::GetVersionStr()) + L"   ";
  CString line;
  for (int i=0;i<s.Length();i++)
    line += L"-";
  Application().LogMessage(line, siInfoMsg);

  Application().LogMessage(s, siInfoMsg);

  int num = BaseInterface::GetNumBaseInterfaces();
  if (num <= 0) s = L"       #BaseInterface: 0";
  else          s = L"       #BaseInterface: " + CString(num - 1) + L" + 1 = " + CString(num);
  Application().LogMessage(s, siInfoMsg);

  Application().LogMessage(line, siInfoMsg);
  
  return CStatus::OK;
}
