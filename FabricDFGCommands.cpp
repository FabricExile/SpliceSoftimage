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
#include "FabricDFGOperators.h"

#include <fstream>
#include <streambuf>

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
  bool openPPG = args[1];

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
      // create the default output port "reservedMatrixOut" and connect the object's global kinematics to it.
      newOp.AddOutputPort(obj.GetKinematics().GetGlobal().GetRef(), L"reservedMatrixOut", -1, -1,  siDefaultPort, &returnStatus);
      if (returnStatus != CStatus::OK)
      { Application().LogMessage(L"failed to create default output port for the global kinematics", siErrorMsg);
        return CStatus::OK; }
    }

    // input ports.
    {
      newOp.AddInputPort(obj.GetKinematics().GetGlobal().GetRef(), L"reservedMatrixIn", -1, -1,  siDefaultPort, &returnStatus);
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
    a.Add(newOp.GetUniqueName());
    Application().ExecuteCommand(L"InspectObj", a, CValue());
  }

  // done.
  return CStatus::OK;
}

// ---
// command "dfgImportJSON".
// ---

SICALLBACK dfgImportJSON_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.PutDescription(L"imports a dfg.json file.");
  oCmd.SetFlag(siNoLogging, false);
  oCmd.EnableReturnValue(false) ;

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"ObjName", CString());
  oArgs.Add(L"JSONFilePath", CString());

  return CStatus::OK;
}

SICALLBACK dfgImportJSON_Execute(CRef &in_ctxt)
{
  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");
  if (args.GetCount() < 2 || CString(args[0]).IsEmpty())
  { Application().LogMessage(L"import json failed: empty or missing argument(s)", siErrorMsg);
    return CStatus::OK; }
  CString objName(args[0]);
  CString filePath = args[1];

  // log.
  Application().LogMessage(L"importing JSON file \"" + filePath + L"\" into \"" + objName + L"\"", siVerboseMsg);

  // set ref at operator.
  CRef ref;
  ref.Set(objName);
  if (!ref.IsValid())
  { Application().LogMessage(L"failed to find an object called \"" + objName + L"\"", siErrorMsg);
    return CStatus::OK; }
  if (ref.GetClassID() != siCustomOperatorID)
  { Application().LogMessage(L"not a custom operator: \"" + objName + L"\"", siErrorMsg);
    return CStatus::OK; }

  // get operator.
  CustomOperator op(ref);
  if (!op.IsValid())
  { Application().LogMessage(L"failed to set custom operator from \"" + objName + L"\"", siErrorMsg);
    return CStatus::OK; }

  // get op's _opUserData.
  _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
  if (!pud)
  { Application().LogMessage(L"found no valid user data in custom operator \"" + objName + L"\"", siErrorMsg);
    Application().LogMessage(L"... operator perhaps not dfgSoftimageOp?", siErrorMsg);
    return CStatus::OK; }

  // read JSON file.
  std::ifstream t(filePath.GetAsciiString(), std::ios::binary);
  if (!t.good())
  { Application().LogMessage(L"unable to open \"" + filePath + "\"", siErrorMsg);
    return CStatus::OK; }
  t.seekg(0, std::ios::end);
  std::string json;
  json.reserve(t.tellg());
  t.seekg(0, std::ios::beg);
  json.assign((std::istreambuf_iterator<char>(t)),
               std::istreambuf_iterator<char>());

  // do it.
  try
  {
    if (!pud->GetBaseInterface())
    { Application().LogMessage(L"no base interface found!", siErrorMsg);
      return CStatus::OK; }
    pud->GetBaseInterface()->setFromJSON(json.c_str());
  }
  catch (FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // done.
  return CStatus::OK;
}

// ---
// command "dfgExportJSON".
// ---

SICALLBACK dfgExportJSON_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.PutDescription(L"exports a dfg.json file.");
  oCmd.SetFlag(siNoLogging, false);
  oCmd.EnableReturnValue(false) ;

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"ObjName", CString());
  oArgs.Add(L"JSONFilePath", CString());

  return CStatus::OK;
}

SICALLBACK dfgExportJSON_Execute(CRef &in_ctxt)
{
  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");
  if (args.GetCount() < 2 || CString(args[0]).IsEmpty())
  { Application().LogMessage(L"export json failed: empty or missing argument(s)", siErrorMsg);
    return CStatus::OK; }
  CString objName(args[0]);
  CString filePath = args[1];

  // log.
  Application().LogMessage(L"exporting \"" + objName + L"\" as JSON file \"" + filePath + L"\"", siVerboseMsg);

  // set ref at operator.
  CRef ref;
  ref.Set(objName);
  if (!ref.IsValid())
  { Application().LogMessage(L"failed to find an object called \"" + objName + L"\"", siErrorMsg);
    return CStatus::OK; }
  if (ref.GetClassID() != siCustomOperatorID)
  { Application().LogMessage(L"not a custom operator: \"" + objName + L"\"", siErrorMsg);
    return CStatus::OK; }

  // get operator.
  CustomOperator op(ref);
  if (!op.IsValid())
  { Application().LogMessage(L"failed to set custom operator from \"" + objName + L"\"", siErrorMsg);
    return CStatus::OK; }

  // get op's _opUserData.
  _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
  if (!pud)
  { Application().LogMessage(L"found no valid user data in custom operator \"" + objName + L"\"", siErrorMsg);
    Application().LogMessage(L"... operator perhaps not dfgSoftimageOp?", siErrorMsg);
    return CStatus::OK; }

  // export JSON file.
  try
  {
    if (!pud->GetBaseInterface())
    { Application().LogMessage(L"no base interface found!", siErrorMsg);
      return CStatus::OK; }
    std::ofstream t(filePath.GetAsciiString(), std::ios::binary);
    if (!t.good())
    { Application().LogMessage(L"unable to open file", siErrorMsg);
      return CStatus::OK; }
    std::string json = pud->GetBaseInterface()->getJSON();
    try
    {
      if (json.c_str())   t.write(json.c_str(), json.length());
      else                t.write("", 0);
    }
    catch (std::exception &e)
    {
      CString err = "write error: ";
      if (e.what())   err += e.what();
      else            err += "";
      feLogError(err.GetAsciiString());
      return CStatus::OK;
    }
  }
  catch (FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
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
  {
    Application().LogMessage(s, siInfoMsg);

    int num = BaseInterface::GetNumBaseInterfaces();
    if (num <= 0) s = L"       #BaseInterface: 0";
    else          s = L"       #BaseInterface: " + CString(num - 1) + L" + 1 = " + CString(num);
    Application().LogMessage(s, siInfoMsg);

    num = _opUserData::GetNumOpUserData();
    s = L"       #_opUserData:   " + CString(num);
    Application().LogMessage(s, siInfoMsg);
  }
  Application().LogMessage(line, siInfoMsg);
  
  return CStatus::OK;
}
