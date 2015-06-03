#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>
#include <xsi_command.h>
#include <xsi_argument.h>
#include <xsi_uitoolkit.h>
#include <xsi_selection.h>
#include <xsi_customoperator.h>
#include <xsi_operatorcontext.h>
#include <xsi_port.h>
#include <xsi_inputport.h>
#include <xsi_outputport.h>
#include <xsi_portgroup.h>
#include <xsi_inputport.h>
#include <xsi_factory.h>
#include <xsi_model.h>
#include <xsi_x3dobject.h>
#include <xsi_kinematics.h>

#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"
#include "FabricDFGTools.h"

#include "FabricSpliceBaseInterface.h"

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
  oArgs.Add(L"ObjectName", CString());
  oArgs.Add(L"dfgJSON", CString());
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
    _opUserData::s_portmap_newOp.clear();
    return CStatus::OK; }
  CString objectName(args[0]);
  CString dfgJSON(args[1]);
  bool openPPG = args[2];

  // log.
  Application().LogMessage(L"applying a  \"dfgSoftimageOp\" custom operator to \"" + objectName + L"\"", siVerboseMsg);

  // get target X3DObject.
  CRefArray objRefArray = Application().GetActiveSceneRoot().FindChildren2(objectName, L"", CStringArray());
  if (objRefArray.GetCount() <= 0)
  { Application().LogMessage(L"failed to find an object called \"" + objectName + L"\" in the scene", siErrorMsg);
    _opUserData::s_portmap_newOp.clear();
    return CStatus::OK; }
  X3DObject obj(objRefArray[0]);
  if (!obj.IsValid())
  { Application().LogMessage(L"failed to create X3DObject for \"" + objectName + L"\"", siErrorMsg);
    _opUserData::s_portmap_newOp.clear();
    return CStatus::OK; }

  // we also add a SpliceOp to the object, so that things don't go wrong when loading a scene.
  {
    dfgTool_ExecuteCommand(L"fabricSplice", L"newSplice", L"{\"targets\":\"" + objectName + L".kine.global\", \"portName\":\"matrix\", \"portMode\":\"io\"}");
  }

  // create the dfgSoftimageOp operator
  CString opName = L"dfgSoftimageOp";
  CustomOperator newOp = Application().GetFactory().CreateObject(opName);

  // set from JSON.
  if (!dfgJSON.IsEmpty())
  {
    _opUserData *pud = _opUserData::GetUserData(newOp.GetObjectID());
    if (  pud
        && pud->GetBaseInterface())
    {
      try
      {
        pud->GetBaseInterface()->setFromJSON(dfgJSON.GetAsciiString());
      }
      catch (FabricCore::Exception e)
      {
        feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
      }
    }
  }

  // the reserved ports.
  {
    CStatus returnStatus;

    // create port group.
    CRef pgReservedRef = newOp.AddPortGroup(L"reservedGroup", 1, 1);
    if (!pgReservedRef.IsValid())
    { Application().LogMessage(L"failed to create reserved port group.", siErrorMsg);
      return CStatus::OK; }

    // create the default output port "reservedMatrixOut" and connect the object's global kinematics to it.
    newOp.AddOutputPort(obj.GetKinematics().GetGlobal().GetRef(), L"reservedMatrixOut", PortGroup(pgReservedRef).GetIndex(), -1,  siDefaultPort, &returnStatus);
    if (returnStatus != CStatus::OK)
    { Application().LogMessage(L"failed to create default output port for the global kinematics", siErrorMsg);
      _opUserData::s_portmap_newOp.clear();
      return CStatus::OK; }

    // create the default input port "reservedMatrixIn" and connect the object's global kinematics to it.
    newOp.AddInputPort(obj.GetKinematics().GetGlobal().GetRef(), L"reservedMatrixIn", PortGroup(pgReservedRef).GetIndex(), -1,  siDefaultPort, &returnStatus);
    if (returnStatus != CStatus::OK)
    { Application().LogMessage(L"failed to create default input port for the global kinematics", siErrorMsg);
      _opUserData::s_portmap_newOp.clear();
      return CStatus::OK; }
  }



  // create exposed DFG output ports.
  for (int i=0;i<_opUserData::s_portmap_newOp.size();i++)
  {
    _portMapping &pmap = _opUserData::s_portmap_newOp[i];
    if (   pmap.dfgPortType != DFG_PORT_TYPE::OUT
        || pmap.mapType     != DFG_PORT_MAPTYPE::XSI_PORT)
      continue;
    Application().LogMessage(L"create port group and port for output port \"" + pmap.dfgPortName + L"\"", siInfoMsg);

    // get classID for the port.
    siClassID classID = GetSiClassIdFromResolvedDataType(pmap.dfgPortDataType);
    if (classID == siUnknownClassID)
    { Application().LogMessage(L"The DFG port \"" + pmap.dfgPortName + "\" cannot be exposed as a XSI Port (data type \"" + pmap.dfgPortDataType + "\" not yet supported)" , siWarningMsg);
      continue; }

    // create port group.
    CRef pgRef = newOp.AddPortGroup(pmap.dfgPortName, 0, 1);
    if (!pgRef.IsValid())
    { Application().LogMessage(L"failed to create port group for \"" + pmap.dfgPortName + "\"", siErrorMsg);
      continue; }

    // create port.
    CRef pRef = newOp.AddOutputPortByClassID(classID, pmap.dfgPortName, PortGroup(pgRef).GetIndex(), 0, siOptionalInputPort);
    if (!pRef.IsValid())
    { Application().LogMessage(L"failed to create port \"" + pmap.dfgPortName + "\"", siErrorMsg);
      continue; }
  }

  // create exposed DFG input ports.
  for (int i=0;i<_opUserData::s_portmap_newOp.size();i++)
  {
    _portMapping &pmap = _opUserData::s_portmap_newOp[i];
    if (   pmap.dfgPortType != DFG_PORT_TYPE::IN
        || pmap.mapType     != DFG_PORT_MAPTYPE::XSI_PORT)
      continue;
    Application().LogMessage(L"create port group and port for input port \"" + pmap.dfgPortName + L"\"", siInfoMsg);

    // get classID for the port.
    siClassID classID = GetSiClassIdFromResolvedDataType(pmap.dfgPortDataType);
    if (classID == siUnknownClassID)
    { Application().LogMessage(L"The DFG port \"" + pmap.dfgPortName + "\" cannot be exposed as a XSI Port (data type \"" + pmap.dfgPortDataType + "\" not yet supported)" , siWarningMsg);
      continue; }

    // create port group.
    CRef pgRef = newOp.AddPortGroup(pmap.dfgPortName, 0, 1);
    if (!pgRef.IsValid())
    { Application().LogMessage(L"failed to create port group for \"" + pmap.dfgPortName + "\"", siErrorMsg);
      continue; }

    // create port.
    CRef pRef = newOp.AddInputPortByClassID(classID, pmap.dfgPortName, PortGroup(pgRef).GetIndex(), 0, siOptionalInputPort);
    if (!pRef.IsValid())
    { Application().LogMessage(L"failed to create port \"" + pmap.dfgPortName + "\"", siErrorMsg);
      continue; }
  }

  // connect the operator.
  if (newOp.Connect() != CStatus::OK)
  { Application().LogMessage(L"newOp.Connect() failed.",siErrorMsg);
    _opUserData::s_portmap_newOp.clear();
    return CStatus::OK; }

  // display operator's property page?
  if (openPPG)
    dfgTool_ExecuteCommand(L"InspectObj", newOp.GetUniqueName());

  // done.
  _opUserData::s_portmap_newOp.clear();
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
  oArgs.Add(L"OperatorName", CString());
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
  CString operatorName(args[0]);
  CString filePath = args[1];

  // log.
  Application().LogMessage(L"importing JSON file \"" + filePath + L"\" into \"" + operatorName + L"\"", siVerboseMsg);

  // set ref at operator.
  CRef ref;
  ref.Set(operatorName);
  if (!ref.IsValid())
  { Application().LogMessage(L"failed to find an object called \"" + operatorName + L"\"", siErrorMsg);
    return CStatus::OK; }
  if (ref.GetClassID() != siCustomOperatorID)
  { Application().LogMessage(L"not a custom operator: \"" + operatorName + L"\"", siErrorMsg);
    return CStatus::OK; }

  // get operator.
  CustomOperator op(ref);
  if (!op.IsValid())
  { Application().LogMessage(L"failed to set custom operator from \"" + operatorName + L"\"", siErrorMsg);
    return CStatus::OK; }

  // get op's _opUserData.
  _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
  if (!pud)
  { Application().LogMessage(L"found no valid user data in custom operator \"" + operatorName + L"\"", siErrorMsg);
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
  oArgs.Add(L"OperatorName", CString());
  oArgs.Add(L"JSONFilePath", CString());  // the filepath of the dfg.json file or L"console" to log the content in the history log (without writing anything to disk).

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
  CString operatorName(args[0]);
  CString filePath = args[1];
  const bool onlyLog = filePath.IsEqualNoCase(L"console");

  // log.
  if (onlyLog)  Application().LogMessage(L"logging JSON of \"" + operatorName + L"\".", siVerboseMsg);
  else          Application().LogMessage(L"exporting \"" + operatorName + L"\" as JSON file \"" + filePath + L"\"", siVerboseMsg);

  // set ref at operator.
  CRef ref;
  ref.Set(operatorName);
  if (!ref.IsValid())
  { Application().LogMessage(L"failed to find an object called \"" + operatorName + L"\"", siErrorMsg);
    return CStatus::OK; }
  if (ref.GetClassID() != siCustomOperatorID)
  { Application().LogMessage(L"not a custom operator: \"" + operatorName + L"\"", siErrorMsg);
    return CStatus::OK; }

  // get operator.
  CustomOperator op(ref);
  if (!op.IsValid())
  { Application().LogMessage(L"failed to set custom operator from \"" + operatorName + L"\"", siErrorMsg);
    return CStatus::OK; }

  // get op's _opUserData.
  _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
  if (!pud)
  { Application().LogMessage(L"found no valid user data in custom operator \"" + operatorName + L"\"", siErrorMsg);
    Application().LogMessage(L"... operator perhaps not dfgSoftimageOp?", siErrorMsg);
    return CStatus::OK; }

  // log JSON.
  if (onlyLog)
  {
    try
    {
      if (!pud->GetBaseInterface())
      { Application().LogMessage(L"no base interface found!", siErrorMsg);
        return CStatus::OK; }
      std::string json = pud->GetBaseInterface()->getJSON();
      Application().LogMessage(L"\n" + CString(json.c_str()), siInfoMsg);
    }
    catch (FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  // export JSON file.
  else
  {
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
  }

  // done.
  return CStatus::OK;
}

// ---
// command "dfgSelectConnected".
// ---

SICALLBACK dfgSelectConnected_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.PutDescription(L"select connected objects.");
  oCmd.SetFlag(siNoLogging, false);
  oCmd.EnableReturnValue(false) ;

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"OperatorName", CString());
  oArgs.Add(L"selWhat", (LONG)0);   // < 0: In only, == 0: All, > 0: Out only.
  oArgs.Add(L"preClearSel", true);  // clear selection prior to selecting connected objects.
  oArgs.Add(L"skipReservedPorts", true);  // skip reserved ports.

  return CStatus::OK;
}

SICALLBACK dfgSelectConnected_Execute(CRef &in_ctxt)
{
  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");
  if (args.GetCount() < 4 || CString(args[0]).IsEmpty())
  { Application().LogMessage(L"empty or missing argument(s)", siErrorMsg);
    return CStatus::OK; }
  CString operatorName(args[0]);
  LONG selWhat = args[1];
  bool preClearSel = args[2];
  bool skipReservedPorts = args[3];

  // log.
  Application().LogMessage(L"select objects connected to \"" + operatorName + L"\"", siVerboseMsg);

  // set ref at operator.
  CRef ref;
  ref.Set(operatorName);
  if (!ref.IsValid())
  { Application().LogMessage(L"failed to find an object called \"" + operatorName + L"\"", siErrorMsg);
    return CStatus::OK; }
  if (ref.GetClassID() != siCustomOperatorID)
  { Application().LogMessage(L"not a custom operator: \"" + operatorName + L"\"", siErrorMsg);
    return CStatus::OK; }

  // get operator.
  CustomOperator op(ref);
  if (!op.IsValid())
  { Application().LogMessage(L"failed to set custom operator from \"" + operatorName + L"\"", siErrorMsg);
    return CStatus::OK; }

  // get current selection and possibly clear it.
  Selection sel = Application().GetSelection();
  if (preClearSel)
    sel.Clear();

  // select connected.
  CString r = L"reserved";
  CRefArray inPorts  = op.GetInputPorts();
  CRefArray outPorts = op.GetOutputPorts();
  if (selWhat <= 0)
  {
    for (int i=0;i<inPorts.GetCount();i++)
    {
      InputPort port(inPorts[i]);
      if (!port.IsConnected())
        continue;
      if (skipReservedPorts && port.GetName().FindString(r) == 0)
        continue;
      sel.Add(port.GetTarget());
    }
  }
  if (selWhat >= 0)
  {
    for (int i=0;i<outPorts.GetCount();i++)
    {
      OutputPort port(outPorts[i]);
      if (!port.IsConnected())
        continue;
      if (skipReservedPorts && port.GetName().FindString(r) == 0)
        continue;
      sel.Add(port.GetTarget());
    }
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



