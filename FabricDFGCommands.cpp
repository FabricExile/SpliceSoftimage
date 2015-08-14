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
#include <xsi_vector2f.h>
#include <xsi_x3dobject.h>
#include <xsi_kinematics.h>
#include <xsi_fcurve.h>
#include <xsi_expression.h>

#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"
#include "FabricDFGCommands.h"
#include "FabricDFGTools.h"
#include "FabricDFGWidget.h"

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
  oCmd.EnableReturnValue(true);     // L"ReturnValue" will contain the CRef of the new operator.

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"ObjectName",     CString());    // 
  oArgs.Add(L"dfgJSON",        CString());    // JSON string for the new op's graph.
  oArgs.Add(L"OpenPPG",        false);        // true: open PPG after creation.
  oArgs.Add(L"otherOpName",    CString());    // optional name of a dfgSoftimage operator. If set then the parameter values, animations, etc. are copied to the new operator's parameters.
  oArgs.Add(L"CreateSpliceOp", 2L);           // 0: no, 1: yes, 2: yes, but only if the object has no SpliceOp yet.

  return CStatus::OK;
}

SICALLBACK dfgSoftimageOpApply_Execute(CRef &in_ctxt)
{
  // ref at global _opUserData::s_portmap_newOp.
  std::vector <_portMapping> &portmap = _opUserData::s_portmap_newOp;

  // init.
  Context ctxt(in_ctxt);
  ctxt.PutAttribute(L"ReturnValue", CRef()); // init return value.
  CValueArray args = ctxt.GetAttribute(L"Arguments");
  if (args.GetCount() < 5 || CString(args[0]).IsEmpty())
  { Application().LogMessage(L"apply dfgSoftimageOp operator failed: empty or missing argument(s)", siErrorMsg);
    portmap.clear();
    return CStatus::OK; }
  CString objectName      (args[0]);
  CString dfgJSON         (args[1]);
  bool    openPPG =        args[2];
  CString otherOpName     (args[3]);
  LONG    createSpliceOp = args[4];

  // log.
  Application().LogMessage(L"applying a  \"dfgSoftimageOp\" custom operator to \"" + objectName + L"\"", siVerboseMsg);

  // get target X3DObject.
  CRefArray objRefArray = Application().GetActiveSceneRoot().FindChildren2(objectName, L"", CStringArray());
  if (objRefArray.GetCount() <= 0)
  { Application().LogMessage(L"failed to find an object called \"" + objectName + L"\" in the scene", siErrorMsg);
    portmap.clear();
    return CStatus::OK; }
  X3DObject obj(objRefArray[0]);
  if (!obj.IsValid())
  { Application().LogMessage(L"failed to create X3DObject for \"" + objectName + L"\"", siErrorMsg);
    portmap.clear();
    return CStatus::OK; }

  // create a SpliceOp before creating the dfgSoftimageOp?
  // (note: adding a SpliceOp to the object prevents things from going wrong when loading a scene into XSI that has one or more dfgSoftimageOp.)
  if (    createSpliceOp == 1
      || (createSpliceOp == 2 && dfgTools::GetRefsAtOps(obj, CString(L"SpliceOp"), XSI::CRefArray()) == 0)  )
  {
    CValueArray args;
    args.Add(L"newSplice");
    args.Add(L"{\"targets\":\"" + objectName + L".kine.global\", \"portName\":\"matrix\", \"portMode\":\"io\"}");
    Application().ExecuteCommand(L"fabricSplice", args, CValue());
  }

  // create the dfgSoftimageOp operator
  CString opName = L"dfgSoftimageOp";
  CustomOperator newOp = Application().GetFactory().CreateObject(opName);

  // store operator's CRef in result.
  ctxt.PutAttribute(L"ReturnValue", newOp.GetRef());

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
      portmap.clear();
      return CStatus::OK; }

    // create the default input port "reservedMatrixIn" and connect the object's global kinematics to it.
    newOp.AddInputPort(obj.GetKinematics().GetGlobal().GetRef(), L"reservedMatrixIn", PortGroup(pgReservedRef).GetIndex(), -1,  siDefaultPort, &returnStatus);
    if (returnStatus != CStatus::OK)
    { Application().LogMessage(L"failed to create default input port for the global kinematics", siErrorMsg);
      portmap.clear();
      return CStatus::OK; }
  }

  // create exposed DFG output ports.
  for (int i=0;i<portmap.size();i++)
  {
    _portMapping &pmap = portmap[i];
    if (   pmap.dfgPortType != DFG_PORT_TYPE_OUT
        || pmap.mapType     != DFG_PORT_MAPTYPE_XSI_PORT)
      continue;
    Application().LogMessage(L"create port group and port for output port \"" + pmap.dfgPortName + L"\"", siInfoMsg);

    // get classID for the port.
    siClassID classID = dfgTools::GetSiClassIdFromResolvedDataType(pmap.dfgPortDataType);
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
  for (int i=0;i<portmap.size();i++)
  {
    _portMapping &pmap = portmap[i];
    if (   pmap.dfgPortType != DFG_PORT_TYPE_IN
        || pmap.mapType     != DFG_PORT_MAPTYPE_XSI_PORT)
      continue;
    Application().LogMessage(L"create port group and port for input port \"" + pmap.dfgPortName + L"\"", siInfoMsg);

    // get classID for the port.
    siClassID classID = dfgTools::GetSiClassIdFromResolvedDataType(pmap.dfgPortDataType);
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
    portmap.clear();
    return CStatus::OK; }

  // copy exposed values, animations etc. from otherOpName.
  if (!otherOpName.IsEmpty())
  {
    CRef ref;
    ref.Set(otherOpName);
    if (!ref.IsValid())
      Application().LogMessage(L"failed to find an object called \"" + otherOpName + L"\"", siWarningMsg);
    else if (ref.GetClassID() != siCustomOperatorID)
      Application().LogMessage(L"not a custom operator: \"" + otherOpName + L"\"", siWarningMsg);
    else
    {
      CustomOperator otherOp(ref);
      if (!otherOp.IsValid())
        Application().LogMessage(L"failed to set custom operator from \"" + otherOpName + L"\"", siWarningMsg);
      else
      {
        CString err;
        std::vector <_portMapping> otherPortmap;
        if (!dfgTools::GetOperatorPortMapping(otherOp, otherPortmap, err))
          Application().LogMessage(L"dfgTools::GetOperatorPortMapping() failed, err = \"" + err + L"\"", siWarningMsg);
        else
        {
          // go through the new op's port mapping and search for a match in the other port mapping.
          for (int ni=0;ni<portmap.size();ni++)
          {
            // find a match for portmap[ni] in otherPortmap.
            int oi = _portMapping::findMatching(portmap[ni], otherPortmap, false /* <- ignore the port data type for this search */);
            if (oi < 0)  continue;
            _portMapping &o = otherPortmap[oi];
            _portMapping &n = portmap[ni];

            // transfer whatever we can from o to n.
            {
              // do we have a connected XSI port?
              if (o.mapType == DFG_PORT_MAPTYPE_XSI_PORT && !o.mapTarget.IsEmpty() && o.dfgPortDataType == n.dfgPortDataType)
              {
                // find the port group.
                PortGroup portgroup;
                CRefArray pgRef = newOp.GetPortGroups();
                for (int i=0;i<pgRef.GetCount();i++)
                {
                  PortGroup pg(pgRef[i]);
                  if (   pg.IsValid()
                      && pg.GetName() == o.dfgPortName)
                    {
                      portgroup.SetObject(pgRef[i]);
                      break;
                    }
                }
                if (!portgroup.IsValid())
                  Application().LogMessage(L"unable to find matching port group for \"" + o.dfgPortName + L"\"", siWarningMsg);
                else
                {
                  // set target ref.
                  CRef targetRef;
                  if (targetRef.Set(o.mapTarget) != CStatus::OK)
                    Application().LogMessage(L"failed to set target ref for \"" + o.mapTarget + L"\"", siWarningMsg);
                  else
                  {
                    // connect.
                    LONG instance;
                    if (newOp.ConnectToGroup(portgroup.GetIndex(), targetRef, instance) != CStatus::OK)
                      Application().LogMessage(L"failed to connect \"" + targetRef.GetAsText() + "\"", siWarningMsg);
                  }
                }
              }

              // do we have an input port that is exposed as a XSI parameter?
              if (o.dfgPortType == DFG_PORT_TYPE_IN && o.mapType == DFG_PORT_MAPTYPE_XSI_PARAMETER)
              {
                Parameter otherParam = otherOp.GetParameter(o.dfgPortName);
                Parameter newParam   = newOp  .GetParameter(n.dfgPortName);
                if (otherParam.IsValid() && newParam.IsValid())
                {
                  // copy the value.
                  newParam.PutValue(otherParam.GetValue());

                  // is the port animated via a fcurve?
                  if (otherParam.GetSource().GetClassIDName() == L"FCurve")
                  {
                    // we only transfer the fcurve if the data types match.
                    if (o.dfgPortDataType == n.dfgPortDataType)
                    {
                      // get the fcurve from otherParam.
                      FCurve otherFCurve(otherParam.GetSource());
                      if (!otherFCurve.IsValid())
                        Application().LogMessage(L"failed get FCurve from \"" + o.dfgPortName + L"\"", siWarningMsg);
                      else
                      {
                        // add a fcurve to newParam.
                        FCurve newFCurve;
                        if (newParam.AddFCurve(otherFCurve.GetFCurveType(), newFCurve) != CStatus::OK)
                          Application().LogMessage(L"failed to add FCurve to \"" + o.dfgPortName + L"\"", siWarningMsg);
                        else
                        {
                          // copy otherFCurve into newFCurve.
                          if (newFCurve.Set(otherFCurve) != CStatus::OK)
                            Application().LogMessage(L"failed to set the FCurve for \"" + o.dfgPortName + L"\"", siWarningMsg);
                        }
                      }
                    }
                  }

                  // is the port animated via an expression?
                  else if (otherParam.GetSource().GetClassIDName() == L"Expression")
                  {
                    // get the expression from otherParam.
                    Expression otherExpression(otherParam.GetSource());
                    if (!otherExpression.IsValid())
                      Application().LogMessage(L"failed get expression from \"" + o.dfgPortName + L"\"", siWarningMsg);
                    else
                    {
                      // copy expression to newParam.

                      /*
                        For some reason this doesn't work, possibly a limitation or a bug in the XSI SDK.
                        Note: also tried to use the command "AddExpr", but without success.
                      */

                      //Expression newExpression = newParam.AddExpression(otherExpression.GetParameter(L"definition").GetValue().GetAsText());
                      //if (!newExpression.IsValid())
                      //  Application().LogMessage(L"failed to set the expression for \"" + o.dfgPortName + L"\"", siWarningMsg);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // display operator's property page?
  if (openPPG)
  {
    CValueArray args;
    args.Add(newOp.GetUniqueName());
    Application().ExecuteCommand(L"InspectObj", args, CValue());
  }

  // done.
  portmap.clear();
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
  oCmd.EnableReturnValue(true);     // if L"ReturnValue" is true then the operator was recreated.

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"OperatorName", CString());
  oArgs.Add(L"JSONFilePath", CString());

  return CStatus::OK;
}

SICALLBACK dfgImportJSON_Execute(CRef &in_ctxt)
{
  // init.
  Context ctxt(in_ctxt);
  ctxt.PutAttribute(L"ReturnValue", false); // init return value.
  CValueArray args = ctxt.GetAttribute(L"Arguments");
  if (args.GetCount() < 2 || CString(args[0]).IsEmpty())
  { Application().LogMessage(L"import json failed: empty or missing argument(s)", siErrorMsg);
    return CStatus::OK; }
  CString operatorName          ( args[0] );
  CString filePath              = args[1];

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

  // get the current port mapping.
  std::vector <_portMapping> portmap_old;
  {
    CString err;
    if (!dfgTools::GetOperatorPortMapping(op, portmap_old, err))
    { Application().LogMessage(L"dfgTools::GetOperatorPortMapping() failed, err = \"" + err + L"\"", siWarningMsg);
      portmap_old.clear(); }
  }

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

  // set from JSON.
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
    return CStatus::OK;
  }

  // get new port mapping.
  std::vector <_portMapping> portmap_new;
  try
  {
    FabricCore::DFGExec exec = pud->GetBaseInterface()->getBinding().getExec();
    portmap_new.resize(exec.getExecPortCount());
    for (int i=0;i<exec.getExecPortCount();i++)
    {
      // init port mapping.
      portmap_new[i].clear();

      // get port name, type and data type.
      portmap_new[i].dfgPortName = exec.getExecPortName(i);
      if      (exec.getExecPortType(i) == FabricCore::DFGPortType_In)   portmap_new[i].dfgPortType = DFG_PORT_TYPE_IN;
      else if (exec.getExecPortType(i) == FabricCore::DFGPortType_Out)  portmap_new[i].dfgPortType = DFG_PORT_TYPE_OUT;
      portmap_new[i].dfgPortDataType = exec.getExecPortResolvedType(i);

      // get mapType.
      const char *data = exec.getExecPortMetadata(exec.getExecPortName(i), "XSI_mapType");
      if (data) portmap_new[i].mapType = (DFG_PORT_MAPTYPE)atoi(data);
    }
  }
  catch (FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // compare portmap_old and portmap_new to determine if we need to recreate the operator or not.
  {
    // for things to be cool we need to find a perfectly matching port in portmap_new/old for each exposed
    // port in portmap_old/new. Furthermore the order of the exposed input/output ports must be the same.
    bool recreateOp = false;
    for (int pass=0;pass<4;pass++)
    {
      std::vector <_portMapping> &a = ((pass & 0x01) == 0 ? portmap_old : portmap_new);
      std::vector <_portMapping> &b = ((pass & 0x01) == 0 ? portmap_new : portmap_old);
      int ib = 0;
      for (int ia=0;ia<a.size();ia++)
      {
        if (a[ia].mapType == DFG_PORT_MAPTYPE_INTERNAL)
          continue;
        if (   pass <= 1 && a[ia].dfgPortType == DFG_PORT_TYPE_IN
            || pass >= 2 && a[ia].dfgPortType == DFG_PORT_TYPE_OUT)
        {
          bool foundMatch = false;
          while (!foundMatch && ib < b.size())
            foundMatch = _portMapping::areMatching(a[ia], b[ib++]);
          if (!foundMatch)
          {
            recreateOp = true;
            break;
          }
        }
      }
    }

    // recreate operator.
    if (recreateOp)
    {
      // create a new operator based on the imported JSON file and portmap_new.
      _opUserData::s_portmap_newOp = portmap_new;
      CValueArray args;
      args.Add(op.GetParent3DObject().GetFullName());
      args.Add(CString(json.c_str()));
      args.Add(true);
      args.Add(op.GetFullName());
      if (Application().ExecuteCommand(L"dfgSoftimageOpApply", args, CValue()) == CStatus::OK)
      {
        // store return value in context, "true" meaning that the operator was recreated.
        ctxt.PutAttribute(L"ReturnValue", true);

        // delete the old operator.
        CValueArray args;
        args.Add(op.GetFullName());
        Application().ExecuteCommand(L"DeleteObj", args, CValue());
      }
    }
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

  // check if base interface exists.
  if (!pud->GetBaseInterface())
  { Application().LogMessage(L"no base interface found!", siErrorMsg);
    return CStatus::OK; }

  // store the ports' exposure types and default values in the ports meta data.
  {
    // get op's port mapping.
    CString pmap_err;
    std::vector <_portMapping> pmap;
    const bool hasValidPortMap = dfgTools::GetOperatorPortMapping(ref, pmap, pmap_err);
    if (!hasValidPortMap)
      Application().LogMessage(L"GetOperatorPortMapping() failed: \"" + pmap_err + L"\"", siWarningMsg);

    // set meta data.
    try
    {
      FabricCore::DFGExec exec = pud->GetBaseInterface()->getBinding().getExec();
      for (int i=0;i<exec.getExecPortCount();i++)
      {
        // mapType.
        exec.setExecPortMetadata(exec.getExecPortName(i), "XSI_mapType", hasValidPortMap ? CString((LONG)pmap[i].mapType).GetAsciiString() : NULL, false);
      }
    }
    catch (FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }
    
  // log JSON.
  if (onlyLog)
  {
    try
    {
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
// command "dfgOpenCanvas".
// ---

SICALLBACK dfgOpenCanvas_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.PutDescription(L"open Canvas.");
  oCmd.SetFlag(siNoLogging, false);
  oCmd.EnableReturnValue(false);
  
  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"OperatorName", CString());

  return CStatus::OK;
}

SICALLBACK dfgOpenCanvas_Execute(CRef &in_ctxt)
{
  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");
  if (args.GetCount() < 1 || CString(args[0]).IsEmpty())
  { Application().LogMessage(L"open canvas failed: empty or missing argument(s)", siErrorMsg);
    return CStatus::OK; }
  CString operatorName(args[0]);

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

  // open canvas.
  Application().LogMessage(L"opening canvas for \"" + operatorName + L"\"", siVerboseMsg);
  CString title = L"Canvas - " + op.GetParent3DObject().GetName();
  OPENCANVAS_RETURN_VALS ret = OpenCanvas(pud, title.GetAsciiString());
  if (ret != OPENCANVAS_RETURN_VALS::SUCCESS)
    Application().LogMessage(CString(GetOpenCanvasErrorDescription(ret)), siWarningMsg);
  else
    Application().LogMessage(L"closing canvas", siVerboseMsg);

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

    Application().LogMessage(L"       #FabricSpliceBaseInterface: " + CString((LONG)FabricSpliceBaseInterface::getInstances().size()), siInfoMsg);
  }
  Application().LogMessage(line, siInfoMsg);
  
  return CStatus::OK;
}

