#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>

#include <xsi_command.h>
#include <xsi_desktop.h>
#include <xsi_comapihandler.h>
#include <xsi_argument.h>
#include <xsi_factory.h>
#include <xsi_uitoolkit.h>
#include <xsi_longarray.h>
#include <xsi_doublearray.h>
#include <xsi_math.h>
#include <xsi_customproperty.h>
#include <xsi_ppglayout.h>
#include <xsi_ppgeventcontext.h>
#include <xsi_menu.h>
#include <xsi_model.h>
#include <xsi_griddata.h>
#include <xsi_gridwidget.h>
#include <xsi_selection.h>
#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_port.h>
#include <xsi_portgroup.h>
#include <xsi_inputport.h>
#include <xsi_outputport.h>
#include <xsi_utils.h>
#include <xsi_matrix4.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_customoperator.h>
#include <xsi_operatorcontext.h>
#include <xsi_parameter.h>
#include <xsi_polygonmesh.h>
#include <xsi_value.h>
#include <xsi_matrix4f.h>
#include <xsi_primitive.h>
#include <xsi_expression.h>
#include <xsi_iceattribute.h>
#include <xsi_iceattributedataarray.h>
#include <xsi_iceattributedataarray2D.h>
#include <xsi_color4f.h>

#include "plugin.h"
#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"
#include "FabricDFGBaseInterface.h"
#include "FabricDFGTools.h"
#include "FabricDFGWidget.h"
#include <Persistence/RTValToJSONEncoder.hpp>

#include <FabricSplice.h>

std::map <unsigned int, _opUserData *>  _opUserData::s_instances;
std::vector<_portMapping>               _opUserData::s_newOp_portmap;
std::vector<std::string>                _opUserData::s_newOp_expressions;

using namespace XSI;

CString xsiGetWorkgroupPath();

XSI::CRef recreateOperator(XSI::CustomOperator op, XSI::CString &dfgJSON)
{
  // memorize current undo levels and then set them to zero.
  const LONG memUndoLevels = dfgTools::GetUndoLevels();
  dfgTools::SetUndoLevels(0);

  CValue newOpRef;

  CValueArray args;
  CValue retVal;
  args.Add(op.GetParent3DObject().GetFullName());
  args.Add(dfgJSON);
  args.Add(true);
  args.Add(op.GetFullName());
  Application().ExecuteCommand(L"FabricCanvasOpApply", args, newOpRef);
  if (CRef(newOpRef).IsValid())
  {
    // delete the old operator.
    args.Clear();
    args.Add(op.GetFullName());
    Application().ExecuteCommand(L"DeleteObj", args, retVal);

    // transfer expressions, if any, from old operator to new one.
    if ((_opUserData::s_newOp_expressions.size() & 0x0001) == 0)
    {
      bool useSDKinsteadOfCommands = true;
      for (LONG i=0;i<_opUserData::s_newOp_expressions.size();i+=2)
      {
        if (useSDKinsteadOfCommands)
        {
          CustomOperator op(newOpRef);
          if (op.IsValid())
          {
            CString pName(_opUserData::s_newOp_expressions[i + 0].c_str());
            Parameter p = op.GetParameter(pName);
            CString expr(_opUserData::s_newOp_expressions[i + 1].c_str());
            if (!p.IsValid() || !p.AddExpression(expr).IsValid())
              Application().LogMessage(L"failed to add expression for parameter \"" + pName + L"\"", siWarningMsg);
          }
        }
        else
        {
          args.Clear();
          args.Add(CustomOperator(newOpRef).GetUniqueName() + L"." + CString(_opUserData::s_newOp_expressions[i + 0].c_str()));
          args.Add(CString(_opUserData::s_newOp_expressions[i + 1].c_str()));
          args.Add(true);
          Application().ExecuteCommand(L"AddExpr", args, retVal);
        }
      }
    }
  }

  // done.
  dfgTools::SetUndoLevels(memUndoLevels);
  return newOpRef;
}

XSIPLUGINCALLBACK CStatus CanvasOp_Init(CRef &in_ctxt)
{
  // beta log.
  CString functionName = L"CanvasOp_Init()";
  if (FabricDFGPlugin_BETA) Application().LogMessage(functionName + L" called", siInfoMsg);

  // init.
  Context ctxt(in_ctxt);
  CustomOperator op(ctxt.GetSource());

  // create user data.
  _opUserData *pud = new _opUserData(op.GetObjectID());
  ctxt.PutUserData((ULLONG)pud);

  // done.
  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus CanvasOp_Term(CRef &in_ctxt)
{
  // beta log.
  CString functionName = L"CanvasOp_Term()";
  if (FabricDFGPlugin_BETA) Application().LogMessage(functionName + L" called", siInfoMsg);

  // init.
  Context ctxt(in_ctxt);
  _opUserData *pud = (_opUserData *)((ULLONG)ctxt.GetUserData());

  // clean up.
  if (pud)
  {
    delete pud;
  }

  // set the global flag, so that the Softimage undo history gets cleared when the event siOnEndCommand gets fired.
  g_clearSoftimageUndoHistory = true;

  // done.
  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus CanvasOp_Define(CRef &in_ctxt)
{
  // beta log.
  CString functionName = L"CanvasOp_Define()";
  if (FabricDFGPlugin_BETA) Application().LogMessage(functionName + L" called", siInfoMsg);

  // init.
  Context ctxt(in_ctxt);
  CustomOperator op = ctxt.GetSource();
  Factory oFactory = Application().GetFactory();
  CRef oPDef;
  Parameter emptyParam;

  // ref at global _opUserData::s_portmap_newOp.
  std::vector <_portMapping> &portmap = _opUserData::s_newOp_portmap;

  // create default parameter(s).
  oPDef = oFactory.CreateParamDef(L"FabricActive",        CValue::siBool,   siPersistable | siAnimatable | siKeyable, L"", L"", true, CValue(), CValue(), CValue(), CValue());
  op.AddParameter(oPDef, emptyParam);
  oPDef = oFactory.CreateParamDef(L"persistenceData",     CValue::siString, siPersistable | siReadOnly,               L"", L"", "", "", "", "", "");
  op.AddParameter(oPDef, emptyParam);
  oPDef = oFactory.CreateParamDef(L"verbose",             CValue::siBool,   siPersistable,                            L"", L"", false, CValue(), CValue(), CValue(), CValue());
  op.AddParameter(oPDef, emptyParam);
  oPDef = oFactory.CreateParamDef(L"graphExecMode",       CValue::siInt4,   siPersistable,                            L"", L"", 1, 0, 99999, 0, 10);
  op.AddParameter(oPDef, emptyParam);
  oPDef = oFactory.CreateParamDef(L"myBitmapWidget",      CValue::siInt4,   siClassifMetaData, siPersistable,         L"", L"", CValue(), CValue(), CValue(), 0, 1);
  op.AddParameter(oPDef, emptyParam);

  // create grid parameter for the list of graph ports.
  // NOTE: this port MUST come JUST BEFORE the exposed DFG parameters!
  op.AddParameter(oFactory.CreateGridParamDef("dfgPorts"), emptyParam);

  // create exposed DFG parameter.
  CString exposedDFGParams = L"";
  {
    for (int i=0;i<portmap.size();i++)
    {
      _portMapping &pmap = portmap[i];

      if (pmap.mapType != DFG_PORT_MAPTYPE_XSI_PARAMETER)
        continue;

      CValue::DataType dt = CValue::siIUnknown;
      bool isBool    = false;
      bool isInteger = false;
      bool isScalar  = false;
      bool isString  = false;
      if      (pmap.dfgPortDataType == L"Boolean")  { dt = CValue::siBool;    isBool    = true;  }

      else if (pmap.dfgPortDataType == L"Scalar")   { dt = CValue::siFloat;   isScalar  = true;  }
      else if (pmap.dfgPortDataType == L"Float32")  { dt = CValue::siFloat;   isScalar  = true;  }
      else if (pmap.dfgPortDataType == L"Float64")  { dt = CValue::siDouble;  isScalar  = true;  }

      else if (pmap.dfgPortDataType == L"Integer")  { dt = CValue::siInt4;    isInteger = true;  }
      else if (pmap.dfgPortDataType == L"SInt8")    { dt = CValue::siInt1;    isInteger = true;  }
      else if (pmap.dfgPortDataType == L"SInt16")   { dt = CValue::siInt2;    isInteger = true;  }
      else if (pmap.dfgPortDataType == L"SInt32")   { dt = CValue::siInt4;    isInteger = true;  }
      else if (pmap.dfgPortDataType == L"SInt64")   { dt = CValue::siInt8;    isInteger = true;  }

      else if (pmap.dfgPortDataType == L"Byte")     { dt = CValue::siUInt1;   isInteger = true;  }
      else if (pmap.dfgPortDataType == L"UInt8")    { dt = CValue::siUInt1;   isInteger = true;  }
      else if (pmap.dfgPortDataType == L"UInt16")   { dt = CValue::siUInt2;   isInteger = true;  }
      else if (pmap.dfgPortDataType == L"Count")    { dt = CValue::siUInt4;   isInteger = true;  }
      else if (pmap.dfgPortDataType == L"Index")    { dt = CValue::siUInt4;   isInteger = true;  }
      else if (pmap.dfgPortDataType == L"Size")     { dt = CValue::siUInt4;   isInteger = true;  }
      else if (pmap.dfgPortDataType == L"UInt32")   { dt = CValue::siUInt4;   isInteger = true;  }
      else if (pmap.dfgPortDataType == L"DataSize") { dt = CValue::siUInt8;   isInteger = true;  }
      else if (pmap.dfgPortDataType == L"UInt64")   { dt = CValue::siUInt8;   isInteger = true;  }

      else if (pmap.dfgPortDataType == L"String")   { dt = CValue::siString;  isString  = true;  }

      if (dt == CValue::siIUnknown)
      { Application().LogMessage(L"The DFG port \"" + pmap.dfgPortName + "\" cannot be exposed as a XSI Parameter (data type \"" + pmap.dfgPortDataType + "\" not yet supported)" , siWarningMsg);
        continue; }

      // build the parameter's default value.
      // (note: we do this semi-manually because CValue doesn't properly convert strings into floats)
      CValue dv;
      if      (isBool)    dv = (bool)      (pmap.xsiDefaultValue.GetAsText() == L"true");
      else if (isScalar)  dv = (double)atof(pmap.xsiDefaultValue.GetAsText().GetAsciiString());
      else if (isInteger) dv = (LLONG) atol(pmap.xsiDefaultValue.GetAsText().GetAsciiString());
      else if (isString)  dv =              pmap.xsiDefaultValue.GetAsText();
      dv.ChangeType(dt);

      // create the parameter definition and add the parameter to the operator.
      Application().LogMessage(L"adding parameter \"" + pmap.dfgPortName + "\"" , siInfoMsg);
      oPDef = oFactory.CreateParamDef(pmap.dfgPortName, dt, siPersistable | siAnimatable | siKeyable, L"", L"", dv, CValue(), CValue(), 0, 1);
      op.AddParameter(oPDef, emptyParam);
      exposedDFGParams += pmap.dfgPortName + L";";
    }
  }

  // add internal string parameter with all the exposed DFG port names.
  // (this is used in the layout function).
  // NOTE: this port MUST come RIGHT AFTER the exposed DFG parameters!
  oPDef = oFactory.CreateParamDef(L"exposedDFGParams", CValue::siString, siReadOnly + siPersistable, L"", L"", exposedDFGParams, "", "", "", "");
  op.AddParameter(oPDef, emptyParam);

  // set default values.
  op.PutAlwaysEvaluate(false);
  op.PutDebug(0);

  // done.
  return CStatus::OK;
}

int CanvasOp_UpdateGridData_dfgPorts(CustomOperator &op)
{
  /*
     Here we set the data of the grid parameter "dfgPorts".

     Note that we only set dat if it is different from the existing one in
     order to avoid unnecessary updates of the dfgSoftimage operator each
     time the PPG is displayed or updated.
  */

  // get grid object.
  GridData grid((CRef)op.GetParameter("dfgPorts").GetValue());

  // get operator's user data and check if there are any graph ports.
  std::vector<_portMapping> pmap;
  CString       err;
  if (!dfgTools::GetOperatorPortMapping(op, pmap, err))
  { 
    // clear grid data.
    grid = GridData();
    return 0;
  }

  // set amount, name and type of the grid columns.
  if (grid.GetRowCount()     != pmap.size())      grid.PutRowCount(pmap.size());
  if (grid.GetColumnCount()  != 4)                grid.PutColumnCount(4);
  if (grid.GetColumnLabel(0) != L"Name")        { grid.PutColumnLabel(0, L"Name");         grid.PutColumnType(0, siColumnStandard); }
  if (grid.GetColumnLabel(1) != L"Type")        { grid.PutColumnLabel(1, L"Type");         grid.PutColumnType(1, siColumnStandard); }
  if (grid.GetColumnLabel(2) != L"Mode")        { grid.PutColumnLabel(2, L"Mode");         grid.PutColumnType(2, siColumnStandard); }
  if (grid.GetColumnLabel(3) != L"Type/Target") { grid.PutColumnLabel(3, L"Type/Target");  grid.PutColumnType(3, siColumnStandard); }

  // set data.
  for (int i=0;i<pmap.size();i++)
  {
    // name.
    CString name = pmap[i].dfgPortName;

    // type (= resolved data type).
    CString type = pmap[i].dfgPortDataType;

    // mode (= DFGPortType).
    CString mode;
    switch (pmap[i].dfgPortType)
    {
      case DFG_PORT_TYPE_IN:    mode = L"In";         break;
      case DFG_PORT_TYPE_OUT:   mode = L"Out";        break;
      default:                  mode = L"undefined";  break;
    }

    // target.
    CString target;
    switch (pmap[i].mapType)
    {
      case DFG_PORT_MAPTYPE_INTERNAL:        target = L"Internal";       break;
      case DFG_PORT_MAPTYPE_XSI_PARAMETER:   target = L"XSI Parameter";  break;
      case DFG_PORT_MAPTYPE_XSI_PORT:      { target = L"XSI Port";
                                             if (pmap[i].mapTarget.IsEmpty())  target += "  ( - )";
                                             else                              target += L" ( -> " + pmap[i].mapTarget + L" )";
                                           }                             break;
      case DFG_PORT_MAPTYPE_XSI_ICE_PORT:    target = L"XSI ICE Port";   break;
      default:                               target = L"unknown";        break;
    }
    
    // set grid data.
    CString label = L"  " + CString(i) + L"  ";
    if (grid.GetRowLabel(i) != label)   grid.PutRowLabel(i, label);
    if (grid.GetCell(0,  i) != name)    grid.PutCell(0,  i, name);
    if (grid.GetCell(1,  i) != type)    grid.PutCell(1,  i, type);
    if (grid.GetCell(2,  i) != mode)    grid.PutCell(2,  i, mode);
    if (grid.GetCell(3,  i) != target)  grid.PutCell(3,  i, target);
  }

  // return amount of rows.
  return pmap.size();
}

void CanvasOp_DefineLayout(PPGLayout &oLayout, CustomOperator &op)
{
  // debug log.
  if (opLOG)  Application().LogMessage(L"CanvasOp_DefineLayout() called");

  // init.
  oLayout.Clear();
  oLayout.PutAttribute(siUIDictionary, L"None");
  PPGItem pi;
	CValueArray lv_enum;

  // button size (Open Canvas).
  const LONG btnCx = 100;
  const LONG btnCy = 26;

  // button size (Refresh).
  const LONG btnRx = 93;
  const LONG btnRy = 26;

  // button size (tools).
  const LONG btnTx = 112;
  const LONG btnTy = 22;

  // button size (tools connect/disconnect).
  const LONG btnTCx = 100;
  const LONG btnTCy = 22;

  // button size (tools - select connected).
  const LONG btnTSCx = 90;
  const LONG btnTSCy = 22;

  // set the modelIsRegular flag depending on whether
  // the operator is under a reference or a regular model.
  bool modelIsRegular = !dfgTools::belongsToRefModel(op);

  // update the grid data.
  const int dfgPortsNumRows = CanvasOp_UpdateGridData_dfgPorts(op);

  // get the names of all graph ports that are available as XSI parameters.
  CStringArray exposedDFGParams = CString(op.GetParameterValue(L"exposedDFGParams")).Split(L";");

  // set the "sync" flag.
  bool outOfSync = false;
  {
    // get the current port mapping.
    std::vector <_portMapping> portmap;
    CString str;
    if (dfgTools::GetOperatorPortMapping(op, portmap, str))
    {
      // check if all the DFG input ports have a corresponding XSI parameter.
      if (!outOfSync)
      {
        for (int i=0;i<portmap.size();i++)
        {
          // get ref and check if we can skip this port.
          _portMapping &pmap = portmap[i];
          if (pmap.dfgPortType != DFG_PORT_TYPE_IN)                continue;
          if (pmap.mapType     != DFG_PORT_MAPTYPE_XSI_PARAMETER)  continue;

          // does a XSI parameter for pmap exist?
          Parameter xsiParam = op.GetParameter(pmap.dfgPortName);
          if (!xsiParam.IsValid())
          {
            outOfSync = true;
            break;
          }

          // check if pmap' data type doesn't match the XSI parameter's data type.
          CValue::DataType  dt = xsiParam.GetValue().m_t;
          {
            if      (pmap.dfgPortDataType == L"Boolean")  outOfSync = (dt != CValue::siBool);

            else if (pmap.dfgPortDataType == L"Scalar")   outOfSync = (dt != CValue::siFloat);
            else if (pmap.dfgPortDataType == L"Float32")  outOfSync = (dt != CValue::siFloat);
            else if (pmap.dfgPortDataType == L"Float64")  outOfSync = (dt != CValue::siDouble);

            else if (pmap.dfgPortDataType == L"Integer")  outOfSync = (dt != CValue::siInt4);
            else if (pmap.dfgPortDataType == L"SInt8")    outOfSync = (dt != CValue::siInt1);
            else if (pmap.dfgPortDataType == L"SInt16")   outOfSync = (dt != CValue::siInt2);
            else if (pmap.dfgPortDataType == L"SInt32")   outOfSync = (dt != CValue::siInt4);
            else if (pmap.dfgPortDataType == L"SInt64")   outOfSync = (dt != CValue::siInt8);

            else if (pmap.dfgPortDataType == L"Byte")     outOfSync = (dt != CValue::siUInt1);
            else if (pmap.dfgPortDataType == L"UInt8")    outOfSync = (dt != CValue::siUInt1);
            else if (pmap.dfgPortDataType == L"UInt16")   outOfSync = (dt != CValue::siUInt2);
            else if (pmap.dfgPortDataType == L"Count")    outOfSync = (dt != CValue::siUInt4);
            else if (pmap.dfgPortDataType == L"Index")    outOfSync = (dt != CValue::siUInt4);
            else if (pmap.dfgPortDataType == L"Size")     outOfSync = (dt != CValue::siUInt4);
            else if (pmap.dfgPortDataType == L"UInt32")   outOfSync = (dt != CValue::siUInt4);
            else if (pmap.dfgPortDataType == L"DataSize") outOfSync = (dt != CValue::siUInt8);
            else if (pmap.dfgPortDataType == L"UInt64")   outOfSync = (dt != CValue::siUInt8);

            else if (pmap.dfgPortDataType == L"String")   outOfSync = (dt != CValue::siString);

            if (outOfSync)
              break;
          }
        }
      }

      // check if all the XSI parameters have a corresponding DFG input port.
      if (!outOfSync)
      {
        bool startChecking = false;
        CParameterRefArray pRef = op.GetParameters();
        for (LONG i=0;i<pRef.GetCount();i++)
        {
          // ref at parameter.
          Parameter p(pRef[i]);

          // get parameter name.
          CString pName = p.GetName();

          // take care of startChecking flag.
          if (!startChecking)
          {
            startChecking = (pName == L"dfgPorts");
            continue;
          }
          else
          {
            if (pName == L"exposedDFGParams")
              break;
          }

          // no matching DFG port?
          int idx = _portMapping::findByPortName(pName, portmap);
          if (   idx < 0
              || portmap[idx].dfgPortType != DFG_PORT_TYPE_IN
              || portmap[idx].mapType     != DFG_PORT_MAPTYPE_XSI_PARAMETER)
          {
            outOfSync = true;
            break;
          }
        }
      }
    }
  }

  // Tab "Main"
  {
    oLayout.AddTab(L"Main");
    {
      oLayout.AddGroup(L"", false);
        oLayout.AddRow();
          oLayout.AddItem(L"FabricActive", L"Execute Graph");
          oLayout.AddSpacer(0, 0);
          pi = oLayout.AddButton(L"BtnOpenCanvas", L"Open Canvas");
          pi.PutAttribute(siUIButtonDisable, !modelIsRegular);
          pi.PutAttribute(siUICX, btnCx);
          pi.PutAttribute(siUICY, btnCy);
        oLayout.EndRow();
      oLayout.EndGroup();
      oLayout.AddSpacer(0, 12);

      if (outOfSync && modelIsRegular)
      {
        oLayout.AddGroup(L"", false);
          oLayout.AddRow();
            oLayout.AddGroup(L"", false);
              oLayout.AddRow();
                pi = oLayout.AddButton(L"BtnRecreateOpInfo", L" ? ");
                pi.PutAttribute(siUICX, 20);
                pi.PutAttribute(siUICY, btnTy);
                pi = oLayout.AddButton(L"BtnRecreateOp", L"Sync Op");
                pi.PutAttribute(siUICX, btnTx - 32);
                pi.PutAttribute(siUICY, btnTy);
                pi = oLayout.AddItem(L"myBitmapWidget", L"myBitmapWidget", siControlBitmap);
                pi.PutAttribute(siUINoLabel, true);
                pi.PutAttribute(siUIFilePath, xsiGetWorkgroupPath() + CUtils::Slash() + L"Application" + CUtils::Slash() + L"UI" + CUtils::Slash() + "FE_exclamation.bmp");
              oLayout.EndRow();
            oLayout.EndGroup();
            oLayout.AddGroup(L"", false);
            oLayout.EndGroup();
          oLayout.EndRow();
        oLayout.EndGroup();
        oLayout.AddSpacer(0, 12);
      }

      oLayout.AddGroup(L"Parameters");
        bool hasParams = (exposedDFGParams.GetCount() > 0);
        for (int i=0;i<exposedDFGParams.GetCount();i++)
        {
          oLayout.AddItem(exposedDFGParams[i]);
        }
        if (exposedDFGParams.GetCount() <= 0)
        {
          oLayout.AddSpacer(0, 8);
          oLayout.AddStaticText(L"   ... no Parameters available.");
          oLayout.AddSpacer(0, 16);
        }
      oLayout.EndGroup();
    }
  }

  // Tab "Ports and Tools"
  {
    oLayout.AddTab(L"Ports and Tools");
    {
      oLayout.AddGroup(L"", false);
        oLayout.AddRow();
          oLayout.AddItem(L"FabricActive", L"Execute Graph");
          oLayout.AddSpacer(0, 0);
          pi = oLayout.AddButton(L"BtnOpenCanvas", L"Open Canvas");
          pi.PutAttribute(siUIButtonDisable, !modelIsRegular);
          pi.PutAttribute(siUICX, btnCx);
          pi.PutAttribute(siUICY, btnCy);
        oLayout.EndRow();
      oLayout.EndGroup();
      oLayout.AddSpacer(0, 12);

      oLayout.AddGroup(L"Graph Ports");
        oLayout.AddRow();
          oLayout.AddSpacer(0, 0);
          pi = oLayout.AddButton(L"BtnUpdatePPG", L"Update UI");
          pi.PutAttribute(siUICX, btnRx);
          pi.PutAttribute(siUICY, btnRy);
        oLayout.EndRow();
        oLayout.AddSpacer(0, 6);
        if (dfgPortsNumRows)
        {
          pi = oLayout.AddItem("dfgPorts", "dfgPorts", siControlGrid);
          pi.PutAttribute(siUINoLabel,              true);
          pi.PutAttribute(siUIWidthPercentage,      100);
          pi.PutAttribute(siUIGridLockRowHeader,    false);
          pi.PutAttribute(siUIGridSelectionMode,    modelIsRegular ? siSelectionCell : siSelectionNone);
          pi.PutAttribute(siUIGridReadOnlyColumns,  "1:1:1:1:1");
        }
        else
        {
          oLayout.AddStaticText(L"   ... no Ports available.");
          oLayout.AddSpacer(0, 8);
        }
      oLayout.EndGroup();
      oLayout.AddSpacer(0, 4);

      oLayout.AddGroup(L" ");

        oLayout.AddGroup(L"Port Definitions and Connections");
          oLayout.AddRow();
            oLayout.AddGroup(L"", false);
              pi = oLayout.AddButton(L"BtnPortsDefineTT", L"Define Type/Target");
              pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0);
              pi.PutAttribute(siUIButtonDisable, !modelIsRegular);
              pi.PutAttribute(siUICX, btnTx);
              pi.PutAttribute(siUICY, btnTy);
              oLayout.AddSpacer(0, btnTy);
              oLayout.AddRow();
                pi = oLayout.AddButton(L"BtnRecreateOpInfo", L" ? ");
                pi.PutAttribute(siUIButtonDisable, !modelIsRegular);
                pi.PutAttribute(siUICX, 20);
                pi.PutAttribute(siUICY, btnTy);
                pi = oLayout.AddButton(L"BtnRecreateOp", L"Sync Op");
                pi.PutAttribute(siUIButtonDisable, !modelIsRegular);
                pi.PutAttribute(siUICX, btnTx - 32);
                pi.PutAttribute(siUICY, btnTy);
                if (outOfSync && modelIsRegular)
                {
                  pi = oLayout.AddItem(L"myBitmapWidget", L"myBitmapWidget", siControlBitmap);
                  pi.PutAttribute(siUINoLabel, true);
                  pi.PutAttribute(siUIFilePath, xsiGetWorkgroupPath() + CUtils::Slash() + L"Application" + CUtils::Slash() + L"UI" + CUtils::Slash() + "FE_exclamation.bmp");
                }
              oLayout.EndRow();
            oLayout.EndGroup();
            oLayout.AddGroup(L"", false);
              pi = oLayout.AddButton(L"BtnPortConnectPick", L"Connect (Pick)");
              pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0 || !modelIsRegular);
              pi.PutAttribute(siUICX, btnTCx + 30);
              pi.PutAttribute(siUICY, btnTCy);
              pi = oLayout.AddButton(L"BtnPortConnectSelf", L"Connect with Self");
              pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0 || !modelIsRegular);
              pi.PutAttribute(siUICX, btnTCx + 20);
              pi.PutAttribute(siUICY, btnTCy);
              pi = oLayout.AddButton(L"BtnPortDisconnectPick", L"Disconnect (Pick)");
              pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0 || !modelIsRegular);
              pi.PutAttribute(siUICX, btnTCx + 10);
              pi.PutAttribute(siUICY, btnTCy);
              pi = oLayout.AddButton(L"BtnPortDisconnectAll", L"Disconnect All");
              pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0 || !modelIsRegular);
              pi.PutAttribute(siUICX, btnTCx + 0);
              pi.PutAttribute(siUICY, btnTCy);
            oLayout.EndGroup();
          oLayout.EndRow();
        oLayout.EndGroup();
        oLayout.AddSpacer(0, 8);

        oLayout.AddGroup(L"Select connected Objects");
          oLayout.AddRow();
            pi = oLayout.AddButton(L"BtnSelConnectAll", L"All");
            pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0);
            pi.PutAttribute(siUICX, btnTSCx);
            pi.PutAttribute(siUICY, btnTSCy);
            pi = oLayout.AddButton(L"BtnSelConnectIn",  L"Inputs");
            pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0);
            pi.PutAttribute(siUICX, btnTSCx);
            pi.PutAttribute(siUICY, btnTSCy);
            pi = oLayout.AddButton(L"BtnSelConnectOut", L"Outputs");
            pi.PutAttribute(siUIButtonDisable, dfgPortsNumRows == 0);
            pi.PutAttribute(siUICX, btnTSCx);
            pi.PutAttribute(siUICY, btnTSCy);
          oLayout.EndRow();
        oLayout.EndGroup();
        oLayout.AddSpacer(0, 8);

        oLayout.AddGroup(L"File");
          oLayout.AddRow();
            pi = oLayout.AddButton(L"BtnImportGraph", L"Import Graph");
            pi.PutAttribute(siUIButtonDisable, !modelIsRegular);
            pi.PutAttribute(siUICX, btnTx);
            pi.PutAttribute(siUICY, btnTy);
            pi = oLayout.AddButton(L"BtnExportGraph", L"Export Graph");
            pi.PutAttribute(siUICX, btnTx);
            pi.PutAttribute(siUICY, btnTy);
          oLayout.EndRow();
        oLayout.EndGroup();
        oLayout.AddSpacer(0, 8);

        oLayout.AddGroup(L"Miscellaneous");
          oLayout.AddRow();
            pi = oLayout.AddButton(L"BtnLogGraphInfo", L"Log Graph Info");
            pi.PutAttribute(siUICX, btnTx);
            pi.PutAttribute(siUICY, btnTy);
            pi = oLayout.AddButton(L"BtnLogGraphFile", L"Log Graph File");
            pi.PutAttribute(siUICX, btnTx);
            pi.PutAttribute(siUICY, btnTy);
          oLayout.EndRow();
        oLayout.EndGroup();

      oLayout.EndGroup();
      oLayout.AddSpacer(0, 4);
    }
  }

  // Tab "Advanced"
  {
    oLayout.AddTab(L"Advanced");
    {
      oLayout.AddItem(L"verbose", L"Verbose");
			lv_enum.Clear();
			lv_enum.Add( L"always execute graph" );
			lv_enum.Add( 0 );
			lv_enum.Add( L"execute graph only if necessary" );
			lv_enum.Add( 1 );
			oLayout.AddEnumControl(L"graphExecMode", lv_enum, L"Exec Mode", siControlCombo);
      oLayout.AddItem(L"persistenceData",     L"persistenceData");
      oLayout.AddItem(L"mute",                L"Mute");
      oLayout.AddItem(L"alwaysevaluate",      L"Always Evaluate");
      oLayout.AddItem(L"debug",               L"Debug");
      oLayout.AddItem(L"Name",                L"Name");
      oLayout.AddButton(L"BtnDebug", L" - ");
    }
  }
}

XSIPLUGINCALLBACK CStatus CanvasOp_PPGEvent(const CRef &in_ctxt)
{
  /*  note:

      if the value of a parameter changes but the UI is not shown then this
      code will not execute. Also this code is not re-entrant, so any changes
      to parameters inside this code will not result in further calls to this function.
  */

  // init.
  PPGEventContext ctxt(in_ctxt);
  CustomOperator op(ctxt.GetSource());
  PPGEventContext::PPGEvent eventID = ctxt.GetEventID();
  UIToolkit toolkit = Application().GetUIToolkit();

  // ref at global _opUserData::s_portmap_newOp.
  std::vector <_portMapping> &portmap = _opUserData::s_newOp_portmap;

  // process event.
  if (eventID == PPGEventContext::siOnInit)
  {
    PPGLayout oLayout = op.GetPPGLayout();
    CanvasOp_DefineLayout(oLayout, op);
    ctxt.PutAttribute(L"Refresh", true);
  }
  else if (eventID == PPGEventContext::siButtonClicked)
  {
    CString btnName = ctxt.GetAttribute(L"Button").GetAsText();
    if (btnName.IsEmpty())
    {
    }
    else if (btnName == L"BtnOpenCanvas")
    {
      // get the current port mapping.
      std::vector <_portMapping> portmap_old;
      CString str;
      dfgTools::GetOperatorPortMapping(op, portmap_old, str);

      // open canvas.
      CString title = L"Canvas - " + op.GetParent3DObject().GetName();
      if (OpenCanvas(_opUserData::GetUserData(op.GetObjectID()), title.GetAsciiString()) == SUCCESS)
      {
        // get the new port mapping.
        std::vector <_portMapping> portmap_new;
        dfgTools::GetOperatorPortMapping(op, portmap_new, str);

        // refresh, if necessary, the PPG.
        bool refresh = (portmap_old.size() != portmap_new.size());
        for (int i=0;i<portmap_old.size() && !refresh;i++)
          refresh = !_portMapping::areMatching(portmap_old[i], portmap_new[i]);
        if (refresh)
        {
          if (opLOG)  Application().LogMessage(L"refreshing PPG after closing Canvas");
          PPGLayout oLayout = op.GetPPGLayout();
          CanvasOp_DefineLayout(oLayout, op);
          ctxt.PutAttribute(L"Refresh", true);
        }
      }
    }
    else if (btnName == L"BtnPortsDefineTT")
    {
      // get the current port mapping.
      CString err;
      if (!dfgTools::GetOperatorPortMapping(op, portmap, err))
      { Application().LogMessage(L"dfgTools::GetOperatorPortMapping() failed, err = \"" + err + L"\"", siErrorMsg);
        portmap.clear();
        return CStatus::OK; }

      // anything there?
      if (portmap.size())
      {
        // open the port mapping dialog.
        if (Dialog_DefinePortMapping(portmap) <= 0)
        { portmap.clear();
          return CStatus::OK; }

        // get the current graph as JSON.
        CString dfgJSON;
        _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
        if (   pud
            && pud->GetBaseInterface())
        {
          try
          {
            std::string json = pud->GetBaseInterface()->getJSON();
            dfgJSON = json.c_str();
          }
          catch (FabricCore::Exception e)
          {
            feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
          }
        }

        // re-create operator.
        recreateOperator(op, dfgJSON);
        dfgTools::ClearUndoHistory();
        ctxt.PutAttribute(L"Close", true);
      }
    }
    else if (btnName == L"BtnRecreateOp")
    {
      // get the current port mapping.
      CString err;
      if (!dfgTools::GetOperatorPortMapping(op, portmap, err))
      { Application().LogMessage(L"dfgTools::GetOperatorPortMapping() failed, err = \"" + err + L"\"", siErrorMsg);
        portmap.clear();
        return CStatus::OK; }

      // get the current graph as JSON.
      CString dfgJSON;
      _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
      if (   pud
          && pud->GetBaseInterface())
      {
        try
        {
          std::string json = pud->GetBaseInterface()->getJSON();
          dfgJSON = json.c_str();
        }
        catch (FabricCore::Exception e)
        {
          feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
        }
      }

      // re-create operator.
      recreateOperator(op, dfgJSON);
      dfgTools::ClearUndoHistory();
      ctxt.PutAttribute(L"Close", true);
    }
    else if (btnName == L"BtnRecreateOpInfo")
    {
      LONG    r;
      CString s = L"Synchronizes the Softimage operator with the Fabric graph\n"
                  L"by re-creating the operator based on the current graph.\n"
                  L"\n"
                  L"Softimage and Fabric can sometime become \"out of sync\"\n"
                  L"after making modifications in Canvas, for example when\n"
                  L"making changes to exposed input and output ports.\n"
                  L"Pressing the \"Sync Op\" button fixes that.\n"
                  ;
      toolkit.MsgBox(s, siMsgOkOnly | siMsgInformation, L"dfgSoftimage", r);
    }
    else if (   btnName == L"BtnPortConnectPick"
             || btnName == L"BtnPortConnectSelf"
             || btnName == L"BtnPortDisconnectPick"
             || btnName == L"BtnPortDisconnectAll")
    {
      // ~~~~~
      // step 1: do the common things.
      // ~~~~~

      LONG ret;

      // get grid and its widget.
      GridData   g((CRef)op.GetParameter("dfgPorts").GetValue());
      if (!g.IsValid())
      { Application().LogMessage(L"failed to get grid data.", siWarningMsg);
        return CStatus::OK; }
      GridWidget w = g.GetGridWidget();
      if (!w.IsValid())
      { Application().LogMessage(L"failed to get grid widget.", siWarningMsg);
        return CStatus::OK; }

      // get selected row.
      int selRow = -1;
      for (int j=0;j<g.GetRowCount();j++)
        for (int i=0;i<g.GetColumnCount();i++)
          if (w.IsCellSelected(i, j))
          {
            if (selRow < 0) { selRow = j;
                              break; }
            else            { toolkit.MsgBox(L"More than one port selected.\n\nPlease select a single port and try again.", siMsgOkOnly, "CanvasOp", ret);
                              return CStatus::OK; }
          }
      if (selRow < 0)
      { toolkit.MsgBox(L"No port selected.\n\nPlease select a single port and try again.", siMsgOkOnly, "CanvasOp", ret);
        return CStatus::OK; }

      // get the name of the selected port and its port mapping.
      CString selPortName = g.GetCell(0, selRow);
      _portMapping pmap;
      {
        std::vector<_portMapping> tmp;
        CString err;
        dfgTools::GetOperatorPortMapping(op, tmp, err);
        for (int i=0;i<tmp.size();i++)
          if (selPortName == tmp[i].dfgPortName)
          {
            pmap = tmp[i];
            break;
          }
      }
      if (pmap.dfgPortName != selPortName)
      { toolkit.MsgBox(L"Failed to find selected port.", siMsgOkOnly | siMsgExclamation, "CanvasOp", ret);
        return CStatus::OK; }
      if (   pmap.dfgPortType != DFG_PORT_TYPE_IN
          && pmap.dfgPortType != DFG_PORT_TYPE_OUT)
      { toolkit.MsgBox(L"Selected port has unsupported type (neither \"In\" nor \"Out\").", siMsgOkOnly | siMsgExclamation, "CanvasOp", ret);
        return CStatus::OK; }
      if (pmap.mapType != DFG_PORT_MAPTYPE_XSI_PORT)
      { toolkit.MsgBox(L"Selected port Type/Target is not \"XSI Port\".", siMsgOkOnly, "CanvasOp", ret);
        return CStatus::OK; }

      // get the port group.
      CString portgroupName = pmap.dfgPortName;
      PortGroup portgroup(dfgTools::GetPortGroupRef(op, portgroupName));
      if (!portgroup.IsValid())
      { toolkit.MsgBox(L"Unable to find matching port group.", siMsgOkOnly | siMsgExclamation, "CanvasOp", ret);
        return CStatus::OK; }

      // ~~~~~
      // step 2: do the button specific things.
      // ~~~~~
      
      // connect things.
      if (btnName.FindString(L"BtnPortConnect") == 0)
      {
        if (btnName == L"BtnPortConnectPick")
        {
          while (true)
          {
            // pick target.
            CRef targetRef;
            CValueArray args(7);
            args[0] = siGenericObjectFilter;
            args[1] = L"Pick";
            args[2] = L"Pick";
            args[5] = 0;
            CValue emptyValue;
            if (Application().ExecuteCommand(L"PickElement", args, emptyValue) == CStatus::Fail)
            { Application().LogMessage(L"PickElement failed.", siWarningMsg);
              break; }
            if ((LONG)args[4] == 0)
              break;
            targetRef = args[3];

            // check/correct target's siClassID and CRef.
            siClassID portClassID = dfgTools::GetSiClassIdFromResolvedDataType(pmap.dfgPortDataType);
            if (targetRef.GetClassID() != portClassID)
            {
              bool err = true;

              // kinematics?
              if (portClassID == siKinematicStateID)
              {
                CRef tmp;
                tmp.Set(targetRef.GetAsText() + L".kine.global");
                if (tmp.IsValid())
                {
                  targetRef = tmp;
                  err = false;
                }
              }

              // polygon mesh?
              if (portClassID == siPolygonMeshID)
              {
                CRef tmp;
                tmp.Set(targetRef.GetAsText() + L".polymsh");
                if (tmp.IsValid())
                {
                  targetRef = tmp;
                  err = false;
                }
              }

              //
              if (err)
              {
                CString emptyString;
                Application().LogMessage(L"the picked element has the type \"" + targetRef.GetClassIDName() + L"\", but the port needs the type \"" + dfgTools::GetSiClassIdDescription(portClassID, emptyString) + L"\".", siErrorMsg);
                continue;
              }
            }

            // check if the connection already exists.
            if (dfgTools::isConnectedToPortGroup(op, portgroupName, targetRef))
            { Application().LogMessage(L"\"" + targetRef.GetAsText() + "\" is already connected to \"" + portgroupName + L"\".", siWarningMsg);
              continue; }

            // if the port group only allows one connection then disconnect any current ones.
            if (portgroup.GetMax() == 1)
              if (!dfgTools::DisconnectedAllFromPortGroup(op, portgroupName))
              { Application().LogMessage(L"DisconnectedAllFromPortGroup() failed.", siWarningMsg);
                break; }

            // connect.
            Application().LogMessage(L"connecting \"" + targetRef.GetAsText() + L"\".", siInfoMsg);
            LONG instance;
            if (op.ConnectToGroup(portgroup.GetIndex(), targetRef, instance) != CStatus::OK)
            { Application().LogMessage(L"failed to connect \"" + targetRef.GetAsText() + "\"", siErrorMsg);
              continue; }

            // leave?
            if (portgroup.GetMax() <= 1)
              break;
          }
        }
        else if (btnName == L"BtnPortConnectSelf")
        {
          // set target ref and check/correct its siClassID and CRef.
          CRef targetRef = op.GetParent3DObject().GetRef();
          siClassID portClassID = dfgTools::GetSiClassIdFromResolvedDataType(pmap.dfgPortDataType);
          if (targetRef.GetClassID() != portClassID)
          {
            bool err = true;

            // kinematics?
            if (portClassID == siKinematicStateID)
            {
              CRef tmp;
              tmp.Set(targetRef.GetAsText() + L".kine.global");
              if (tmp.IsValid())
              {
                targetRef = tmp;
                err = false;
              }
            }

            // polygon mesh?
            if (portClassID == siPolygonMeshID)
            {
              CRef tmp;
              tmp.Set(targetRef.GetAsText() + L".polymsh");
              if (tmp.IsValid())
              {
                targetRef = tmp;
                err = false;
              }
            }

            //
            if (err)
            {
              CString emptyString;
              Application().LogMessage(L"the element has the type \"" + targetRef.GetClassIDName() + L"\", but the port needs the type \"" + dfgTools::GetSiClassIdDescription(portClassID, emptyString) + L"\".", siErrorMsg);
              return CStatus::OK;
            }
          }

          // check if the connection already exists.
          if (dfgTools::isConnectedToPortGroup(op, portgroupName, targetRef))
          { Application().LogMessage(L"\"" + targetRef.GetAsText() + "\" is already connected to \"" + portgroupName + L"\".", siWarningMsg);
            return CStatus::OK; }

          // if the port group only allows one connection then disconnect any current ones.
          if (portgroup.GetMax() == 1)
            if (!dfgTools::DisconnectedAllFromPortGroup(op, portgroupName))
            { Application().LogMessage(L"DisconnectedAllFromPortGroup() failed.", siWarningMsg);
              return CStatus::OK; }

          // connect.
          Application().LogMessage(L"connecting \"" + targetRef.GetAsText() + L"\".", siInfoMsg);
          LONG instance;
          if (op.ConnectToGroup(portgroup.GetIndex(), targetRef, instance) != CStatus::OK)
          { Application().LogMessage(L"failed to connect \"" + targetRef.GetAsText() + "\"", siErrorMsg);
            return CStatus::OK; }
        }
      }
      // disconnect things.
      else
      {
        if (btnName == L"BtnPortDisconnectPick")
        {
          while (true)
          {
            op.SetObject(ctxt.GetSource());

            // pick target.
            CRef targetRef;
            CValueArray args(7);
            args[0] = siGenericObjectFilter;
            args[1] = L"Pick";
            args[2] = L"Pick";
            args[5] = 0;
            CValue emptyValue;
            if (Application().ExecuteCommand(L"PickElement", args, emptyValue) == CStatus::Fail)
            { Application().LogMessage(L"PickElement failed.", siWarningMsg);
              break; }
            if ((LONG)args[4] == 0)
              break;
            targetRef = args[3];

            // check/correct target's siClassID and CRef.
            siClassID portClassID = dfgTools::GetSiClassIdFromResolvedDataType(pmap.dfgPortDataType);
            if (targetRef.GetClassID() != portClassID)
            {
              // kinematics?
              if (portClassID == siKinematicStateID)
              {
                CRef tmp;
                tmp.Set(targetRef.GetAsText() + L".kine.global");
                if (tmp.IsValid())
                {
                  targetRef = tmp;
                }
              }

              // polygon mesh?
              if (portClassID == siPolygonMeshID)
              {
                CRef tmp;
                tmp.Set(targetRef.GetAsText() + L".polymsh");
                if (tmp.IsValid())
                {
                  targetRef = tmp;
                }
              }
            }

            // if the connection exists then disconnect it.
            if (dfgTools::isConnectedToPortGroup(op, portgroupName, targetRef))
            {
              Application().LogMessage(L"disconnecting \"" + targetRef.GetAsText() + L"\".", siInfoMsg);
              LONG instance;
              if (!dfgTools::DisconnectFromPortGroup(op, portgroupName, targetRef))
              { Application().LogMessage(L"failed to disconnect \"" + targetRef.GetAsText() + "\"", siErrorMsg);
                continue; }
            }
            else
            { Application().LogMessage(L"\"" + targetRef.GetAsText() + "\" is not connected to \"" + portgroupName + L"\".", siWarningMsg);
              continue; }

            // done?
            if (portgroup.GetInstanceCount() == 0)
              break;

            // note: we leave here due to a bug in the SDK (XSI somehow doesn't
            // correctly update the operators port group instances and disconnects
            // the wrong objects if we continue the picking session).
            break;
          }
        }
        else if (btnName == L"BtnPortDisconnectAll")
        {
          if (!dfgTools::DisconnectedAllFromPortGroup(op, portgroupName))
            Application().LogMessage(L"DisconnectedAllFromPortGroup() failed.", siWarningMsg);
        }
      }

      // ~~~~~
      // step 3: refresh layout..
      // ~~~~~

      PPGLayout oLayout = op.GetPPGLayout();
      CanvasOp_DefineLayout(oLayout, op);
      ctxt.PutAttribute(L"Refresh", true);
    }
    else if (btnName == L"BtnSelConnectAll")
    {
      CValueArray args;
      CValue val;
      args.Add(op.GetUniqueName());
      args.Add((LONG)0);
      Application().ExecuteCommand(L"FabricCanvasSelectConnected", args, val);
    }
    else if (btnName == L"BtnSelConnectIn")
    {
      CValueArray args;
      CValue val;
      args.Add(op.GetUniqueName());
      args.Add((LONG)-1);
      Application().ExecuteCommand(L"FabricCanvasSelectConnected", args, val);
    }
    else if (btnName == L"BtnSelConnectOut")
    {
      CValueArray args;
      CValue val;
      args.Add(op.GetUniqueName());
      args.Add((LONG)+1);
      Application().ExecuteCommand(L"FabricCanvasSelectConnected", args, val);
    }
    else if (btnName == L"BtnImportGraph")
    {
      // open file browser.
      CString fileName;
      if (!dfgTools::FileBrowserJSON(false, fileName))
      { Application().LogMessage(L"aborted by user.", siWarningMsg);
        return CStatus::OK; }

      // execute command.
      CValue opWasRecreated = false;
      CValueArray args;
      args.Add(op.GetUniqueName());
      args.Add(fileName);
      if (Application().ExecuteCommand(L"FabricCanvasImportGraph", args, opWasRecreated) == CStatus::OK)
      {
        if (opWasRecreated)
        {
          ctxt.PutAttribute(L"Close", true);
        }
        else
        {
          PPGLayout oLayout = op.GetPPGLayout();
          CanvasOp_DefineLayout(oLayout, op);
          ctxt.PutAttribute(L"Refresh", true);
        }
        dfgTools::ClearUndoHistory();
      }
    }
    else if (btnName == L"BtnExportGraph")
    {
      // open file browser.
      CString fileName;
      if (!dfgTools::FileBrowserJSON(true, fileName))
      { Application().LogMessage(L"aborted by user.", siWarningMsg);
        return CStatus::OK; }

      // export.
      CValueArray args;
      CValue val;
      args.Add(op.GetUniqueName());
      args.Add(fileName);
      Application().ExecuteCommand(L"FabricCanvasExportGraph", args, val);
    }
    else if (btnName == L"BtnUpdatePPG")
    {
      PPGLayout oLayout = op.GetPPGLayout();
      CanvasOp_DefineLayout(oLayout, op);
      ctxt.PutAttribute(L"Refresh", true);
    }
    else if (btnName == L"BtnLogGraphInfo")
    {
      try
      {
        // init/check.
        _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
        if (!pud)                                 { Application().LogMessage(L"no user data found!",      siErrorMsg);  return CStatus::OK; }
        if (!pud->GetBaseInterface())             { Application().LogMessage(L"no base interface found!", siErrorMsg);  return CStatus::OK; }

        // log intro.
        Application().LogMessage(L"\"" + op.GetRef().GetAsText() + L"\" (ObjectID = " + CString(op.GetObjectID()) + L")", siInfoMsg);
        Application().LogMessage(L"{", siInfoMsg);

        // log (graph ports).
        FabricCore::DFGExec exec = pud->GetBaseInterface()->getBinding()->getExec();
        Application().LogMessage(L"    amount of graph ports: " + CString((LONG)exec.getExecPortCount()), siInfoMsg);
        for (int i=0;i<exec.getExecPortCount();i++)
        {
          char s[16];
          sprintf(s, "%03ld", i);
          CString t;
          t  = L"        " + CString(s) + L". ";
          if      (exec.getExecPortType(i) == FabricCore::DFGPortType_In)  t += L"type = \"In\",  ";
          else if (exec.getExecPortType(i) == FabricCore::DFGPortType_Out) t += L"type = \"Out\", ";
          else                                                             t += L"type = \"IO\",  ";
          t += L"name = \"" + CString(exec.getExecPortName(i)) + L"\", ";
          t += L"resolved data type = \"" + CString(exec.getExecPortResolvedType(i)) + L"\"";
          Application().LogMessage(t, siInfoMsg);
        }

        // log (XSI port groups).
        LONG sumNumPortsInGroups = 0;
        CRefArray pGroups = op.GetPortGroups();
        Application().LogMessage(L" ", siInfoMsg);
        Application().LogMessage(L"    amount of XSI port groups: " + CString(pGroups.GetCount()), siInfoMsg);
        for (int i=0;i<pGroups.GetCount();i++)
        {
          PortGroup pg(pGroups[i]);

          char s[16];
          sprintf(s, "%03ld", i);
          CString t;
          t  = L"        " + CString(s) + L". ";
          t += L"name = \"" + pg.GetName() + L"\",  ";
          t += L"index = " + CString(pg.GetIndex()) + L",  ";
          t += L"num instances = " + CString(pg.GetInstanceCount()) + L",  ";
          LONG numPorts = pg.GetPorts().GetCount();
          t += L"num ports = " + CString(numPorts) + L",  ";
          Application().LogMessage(t, siInfoMsg);

          sumNumPortsInGroups += numPorts;
        }

        // log (XSI ports).
        CRefArray inPorts  = op.GetInputPorts();
        CRefArray outPorts = op.GetOutputPorts();
        LONG numPorts = inPorts.GetCount() + outPorts.GetCount();
        Application().LogMessage(L" ", siInfoMsg);
        Application().LogMessage(L"    amount of XSI ports: " + CString(numPorts) + L"(" + CString(inPorts.GetCount()) + L" In and " + CString(outPorts.GetCount()) + L" Out)", siInfoMsg);
        for (int i=0;i<inPorts.GetCount();i++)
        {
          InputPort port(inPorts[i]);
          char s[16];
          sprintf(s, "% 3ld", i);
          CString t;
          t  = L"  " + CString(s) + L". ";
          t += L"type = \"In\",  ";
          t += L"name = \"" + port.GetName() + L"\", ";
          t += L"target = \"" + port.GetTarget().GetAsText() + L"\", ";
          t += L"target classID = \"" + port.GetTarget().GetClassIDName() + L"\"";
          Application().LogMessage(t, siInfoMsg);
        }
        for (int i=0;i<outPorts.GetCount();i++)
        {
          OutputPort port(outPorts[i]);
          char s[16];
          sprintf(s, "% 3ld", i + inPorts.GetCount());
          CString t;
          t  = L"  " + CString(s) + L". ";
          t += L"type = \"Out\", ";
          t += L"name = \"" + port.GetName() + L"\", ";
          t += L"target = \"" + port.GetTarget().GetAsText() + L"\", ";
          t += L"target classID = \"" + port.GetTarget().GetClassIDName() + L"\"";
          Application().LogMessage(t, siInfoMsg);
        }
      }
      catch (FabricCore::Exception e)
      {
        feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
      }
    }
    else if (btnName == L"BtnLogGraphFile")
    {
      CValueArray args;
      CValue val;
      args.Add(op.GetUniqueName());
      args.Add(L"console");
      Application().ExecuteCommand(L"FabricCanvasExportGraph", args, val);
    }
    else if (btnName == L"BtnDebug")
    {
      _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
      if (pud)
        if (pud->GetBaseInterface())
        {
          FabricCore::DFGExec graph = pud->GetBaseInterface()->getBinding()->getExec();
          if (graph.isValid())
          {
            static ULONG i = 0;
            feLog(CString(L"BtnDebug #" + CString(i++)).GetAsciiString());
            Application().LogMessage(L"------- debug output start -------");
            {

              // add code to be executed when pressing the " - " button in the tab "Advanced".
              {
                FabricCore::Client client = FabricSplice::ConstructClient();
                CString result = client.getContextID();
                Application().LogMessage(L"FabricSplice::ConstructClient().getContextID() = \"" + result + L"\"");
              }

            }
            Application().LogMessage(L"------- debug output end -------");
          }
        }
    }
  }
  else if (eventID == PPGEventContext::siTabChange)
  {
  }
  else if (eventID == PPGEventContext::siParameterChange)
  {
  }

  // done.
  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus CanvasOp_Update(CRef &in_ctxt)
{
  // init.
  OperatorContext ctxt(in_ctxt);
  CustomOperator op(ctxt.GetSource());
  _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
  if (!pud)                                     { Application().LogMessage(L"no user data found!", siErrorMsg);
                                                  return CStatus::OK; }
  if (!pud->GetBaseInterface())                 { Application().LogMessage(L"no base interface found!", siErrorMsg);
                                                  return CStatus::OK; }
  if (!pud->GetBaseInterface()->getBinding())   { Application().LogMessage(L"no binding found!", siErrorMsg);
                                                  return CStatus::OK; }

  // log.
  CString functionName = L"CanvasOp_Update(opObjID = " + CString(op.GetObjectID()) + L")";
  const bool verbose = (bool)ctxt.GetParameterValue(L"verbose");
  if (verbose)  Application().LogMessage(functionName + L" called #" + CString((LONG)pud->updateCounter), siInfoMsg);
  pud->updateCounter++;

  // check the FabricActive parameter.
  if (!(bool)ctxt.GetParameterValue(L"FabricActive"))
  {
    pud->execFabricStep12 = false;
    return CStatus::OK;
  }

  // get the currently evaluated output port and its target.
  OutputPort outputPort(ctxt.GetOutputPort());
  CRef       outputPortTarget = outputPort.GetTarget();
  if (verbose) Application().LogMessage(functionName + L": evaluating output port \"" + outputPort.GetName() + L"\" (target = \"" + outputPortTarget.GetAsText() + L"\")");

  // get the total amount of connected output ports.
  int numConnectedOutputPorts = 0;
  {
    CRefArray opPortsOutput = op.GetOutputPorts();
    for (int i=0;i<opPortsOutput.GetCount();i++)
      if (OutputPort(opPortsOutput[i]).IsConnected())
        numConnectedOutputPorts++;
    if (verbose) Application().LogMessage(functionName + L": amount of connected output ports = " + CString((LONG)numConnectedOutputPorts));
  }

  // set the execFabric flags (i.e. check if we need to execute the Fabric steps 1 and 2).
  bool activateExecFlagsForNextUpdate = false;
  if ((LONG)ctxt.GetParameterValue(L"graphExecMode") == 0)
  {
    /* "always execute graph" */

    // execute the Fabric steps 1 and 2 now, regardless of how things are connected.
    pud->execFabricStep12 = true;
  }
  else
  {
    /* graphExecMode is "execute graph only if necessary" */

    // CASE 1: this is the port reservedMatrixOut.
    if (outputPort.GetName() == L"reservedMatrixOut")
    {
      if (numConnectedOutputPorts == 1)
      {
        // only the port reservedMatrixOut is connected, so we need to execute the Fabric steps 1 and 2 now.
        pud->execFabricStep12 = true;
      }
      else
      {
        // aside from the port reservedMatrixOut we have at least one further connected port, so we will
        // execute the Fabric steps 1 and 2 the next time this operator gets evaluated.
        pud->execFabricStep12          = false;
        activateExecFlagsForNextUpdate = true;
      }
    }

    // case 2: this is not the port reservedMatrixOut.
    else
    {
      // the only interesting case here is to check whether the port being currently evaluated has the same target
      // as the reserved port (note: the reserved port's target is equal the operator's parent).
      if (outputPortTarget.GetAsText() == op.GetParent().GetAsText())
        pud->execFabricStep12 = true;
    }
  }

  // get pointers/refs at binding, graph & co.
  BaseInterface                                   *baseInterface  = pud->GetBaseInterface();
  FabricCore::Client                              *client         = pud->GetBaseInterface()->getClient();
  FabricCore::DFGBinding                          *binding        = pud->GetBaseInterface()->getBinding();
  FabricCore::DFGExec                              exec           = binding->getExec();

  // Fabric Engine (step 0): update the base interface's evalContext.
  {
    FabricCore::RTVal &evalContext = *baseInterface->getEvalContext();

    if (!evalContext.isValid())
    {
      try
      {
        evalContext = FabricCore::RTVal::Create(*client, "EvalContext", 0, 0);
        evalContext = evalContext.callMethod("EvalContext", "getInstance", 0, 0);
      }
      catch(FabricCore::Exception e)
      {
        feLogError(e.getDesc_cstr());
      }
    }

    if(evalContext.isValid())
    {
      try
      {
        evalContext.setMember("time",            FabricCore::RTVal::ConstructFloat32(*client, (float)ctxt.GetTime().GetTime(CTime::Seconds)));
      }
      catch(FabricCore::Exception e)
      {
        feLogError(e.getDesc_cstr());
      }
    }
  }

  // Fabric Engine (step 1): loop through all the DFG's input ports and set
  //                         their values from the matching XSI ports or parameters.
  if (pud->execFabricStep12)
  {
    if (verbose) Application().LogMessage(L"------- SET DFG EXEC PORTS FROM XSI PARAMS/PORTS.");
    try
    {
      for (int i=0;i<exec.getExecPortCount();i++)
      {
        bool done = false;

        // get/check DFG port.
        if (exec.getExecPortType(i) != FabricCore::DFGPortType_In)
          continue;

        CString portName                = exec.getExecPortName(i);
        CString portResolvedType        = exec.getExecPortResolvedType(i);
        bool    portResolvedTypeIsArray = (portResolvedType.ReverseFindString(L"[]") != UINT_MAX);
        bool storable = true;

        // find a matching XSI port (singleton).
        if (!done && !portResolvedTypeIsArray)
        {
          CValue xsiPortValue = op.GetInputValue(portName, portName);
          if (!xsiPortValue.IsEmpty())
          {
            done = true;

            //
            if (verbose) Application().LogMessage(functionName + L": transfer xsi port data to dfg port \"" + portName + L"\"");

            if      (portResolvedType == L"")
            {
              // do nothing.
            }
            else if (portResolvedType == L"Mat44")
            {
              if (xsiPortValue.m_t == CValue::siRef)
              {
                KinematicState ks(xsiPortValue);
                if (ks.IsValid())
                {
                  // put the XSI port's value into a std::vector.
                  MATH::CMatrix4 m = ks.GetTransform().GetMatrix4();
                  std::vector <double> val(16);
                  val[ 0] = m.GetValue(0, 0); // row 0.
                  val[ 1] = m.GetValue(1, 0);
                  val[ 2] = m.GetValue(2, 0);
                  val[ 3] = m.GetValue(3, 0);
                  val[ 4] = m.GetValue(0, 1); // row 1.
                  val[ 5] = m.GetValue(1, 1);
                  val[ 6] = m.GetValue(2, 1);
                  val[ 7] = m.GetValue(3, 1);
                  val[ 8] = m.GetValue(0, 2); // row 2.
                  val[ 9] = m.GetValue(1, 2);
                  val[10] = m.GetValue(2, 2);
                  val[11] = m.GetValue(3, 2);
                  val[12] = m.GetValue(0, 3); // row 3.
                  val[13] = m.GetValue(1, 3);
                  val[14] = m.GetValue(2, 3);
                  val[15] = m.GetValue(3, 3);

                  // set the DFG port from the std::vector.
                  BaseInterface::SetValueOfArgMat44(*client, *binding, portName.GetAsciiString(), val);
                }
              }
            }
            else if (portResolvedType == L"Xfo")
            {
              if (xsiPortValue.m_t == CValue::siRef)
              {
                KinematicState ks(xsiPortValue);
                if (ks.IsValid())
                {
                  // put the XSI port's value into a std::vector.
                  MATH::CTransformation t = ks.GetTransform();
                  MATH::CQuaternion     q = t.GetRotationQuaternion();
                  std::vector <double> val(10);
                  val[0] = t.GetSclX();   // scaling.
                  val[1] = t.GetSclY();
                  val[2] = t.GetSclZ();
                  val[3] = q.GetW();      // orientation.
                  val[4] = q.GetX();
                  val[5] = q.GetY();
                  val[6] = q.GetZ();
                  val[7] = t.GetPosX();   // position.
                  val[8] = t.GetPosY();
                  val[9] = t.GetPosZ();

                  // set the DFG port from the std::vector.
                  BaseInterface::SetValueOfArgXfo(*client, *binding, portName.GetAsciiString(), val);
                }
              }
            }

            //
            else if (portResolvedType == L"PolygonMesh")
            {
              if (xsiPortValue.m_t == CValue::siRef)
              {
                CRef ref;   // note: Primitive::GetGeometryFromX3DObject() does not work inside the _update() context, so we build the reference at the X3DObject ourself.
                CString s = CRef(xsiPortValue).GetAsText();
                ref.Set(s.GetSubString(0, s.ReverseFindString(L".")));
                if (ref.IsValid())
                {
                  CString   errmsg;
                  CString   wrnmsg;
                  _polymesh val;
                  if (dfgTools::GetGeometryFromX3DObject( X3DObject(ref),
                                                          ctxt.GetTime().GetTime(),
                                                          val,
                                                          errmsg,
                                                          wrnmsg  ) )
                  {
                    // any warning?
                    if (wrnmsg != L"")  Application().LogMessage(L"\"" + wrnmsg + L"\"", siWarningMsg);

                    // verbose.
                    if (verbose)  Application().LogMessage(functionName + L": polygon mesh \"" + ref.GetAsText() + L"\": #vertices = " + CString((ULONG)val.numVertices) + L"  #polygons = " + CString((ULONG)val.numPolygons) + L"  #samples = " + CString((ULONG)val.numSamples));

                    // set the DFG port from val.
                    BaseInterface::SetValueOfArgPolygonMesh(*client, *binding, portName.GetAsciiString(), val);
                  }
                  else
                  {
                    // failed to get geometry from X3DObject.
                    Application().LogMessage(L"ERROR: failed to get geometry from \"" + ref.GetAsText() + "\": \"" + errmsg + L"\"", siWarningMsg);
                  }
                }
              }
              storable = false;
            }
          }
        }

        // find a matching XSI port (array).
        if (!done && portResolvedTypeIsArray)
        {
          PortGroup portGroup(dfgTools::GetPortGroupRef(op, portName));
          if (portGroup.IsValid())
          {
            done = true;

            //
            if (verbose) Application().LogMessage(functionName + L": transfer xsi port data to dfg port \"" + portName + L"\"");

            if      (portResolvedType == L"")
            {
              // do nothing.
            }
            else if (portResolvedType == L"Mat44[]")
            {
              // put the XSI port groups's values into a std::vector.
              MATH::CMatrix4 m;
              std::vector <double> val(16 * portGroup.GetInstanceCount());
              for (LONG i=0;i<portGroup.GetInstanceCount();i++)
              {
                CValue xsiPortValue = op.GetInputValue(portName, portName, i);
                KinematicState ks(xsiPortValue);
                if (ks.IsValid())   m = ks.GetTransform().GetMatrix4();
                else                m . SetIdentity();
                LONG vi = 16 * i;
                val[vi + 0] = m.GetValue(0, 0); // row 0.
                val[vi + 1] = m.GetValue(1, 0);
                val[vi + 2] = m.GetValue(2, 0);
                val[vi + 3] = m.GetValue(3, 0);
                val[vi + 4] = m.GetValue(0, 1); // row 1.
                val[vi + 5] = m.GetValue(1, 1);
                val[vi + 6] = m.GetValue(2, 1);
                val[vi + 7] = m.GetValue(3, 1);
                val[vi + 8] = m.GetValue(0, 2); // row 2.
                val[vi + 9] = m.GetValue(1, 2);
                val[vi +10] = m.GetValue(2, 2);
                val[vi +11] = m.GetValue(3, 2);
                val[vi +12] = m.GetValue(0, 3); // row 3.
                val[vi +13] = m.GetValue(1, 3);
                val[vi +14] = m.GetValue(2, 3);
                val[vi +15] = m.GetValue(3, 3);
              }

              // set the DFG port from the std::vector.
              BaseInterface::SetValueOfArgMat44Array(*client, *binding, portName.GetAsciiString(), val);
            }
            else if (portResolvedType == L"Xfo[]")
            {
              // put the XSI port groups's values into a std::vector.
              MATH::CTransformation t;
              MATH::CQuaternion     q;
              std::vector <double> val(10 * portGroup.GetInstanceCount());
              for (LONG i=0;i<portGroup.GetInstanceCount();i++)
              {
                CValue xsiPortValue = op.GetInputValue(portName, portName, i);
                KinematicState ks(xsiPortValue);
                if (ks.IsValid())   t = ks.GetTransform();
                else                t . SetIdentity();
                q = t.GetRotationQuaternion();
                LONG vi = 10 * i;
                val[vi + 0] = t.GetSclX();   // scaling.
                val[vi + 1] = t.GetSclY();
                val[vi + 2] = t.GetSclZ();
                val[vi + 3] = q.GetW();      // orientation.
                val[vi + 4] = q.GetX();
                val[vi + 5] = q.GetY();
                val[vi + 6] = q.GetZ();
                val[vi + 7] = t.GetPosX();   // position.
                val[vi + 8] = t.GetPosY();
                val[vi + 9] = t.GetPosZ();
              }

              // set the DFG port from the std::vector.
              BaseInterface::SetValueOfArgXfoArray(*client, *binding, portName.GetAsciiString(), val);
            }
          }
        }

        // find a matching XSI parameter.
        if (!done)
        {
          Parameter xsiParam = op.GetParameter(portName);
          if (xsiParam.IsValid())
          {
            done = true;

            //
            if (verbose) Application().LogMessage(functionName + L": transfer xsi parameter data to dfg port \"" + portName + L"\"");

            //
            CValue xsiValue = xsiParam.GetValue();

            //
            if      (   portResolvedType == "Boolean")    {
                                                              bool val = (bool)xsiValue;
                                                              BaseInterface::SetValueOfArgBoolean(*client, *binding, portName.GetAsciiString(), val);
                                                            }
            else if (   portResolvedType == "Integer"
                     || portResolvedType == "SInt8"
                     || portResolvedType == "SInt16"
                     || portResolvedType == "SInt32"
                     || portResolvedType == "SInt64" )    {
                                                              int val = (int)(LONG)xsiValue;
                                                              BaseInterface::SetValueOfArgSInt(*client, *binding, portName.GetAsciiString(), val);
                                                            }
            else if (   portResolvedType == "Byte"
                     || portResolvedType == "UInt8"
                     || portResolvedType == "UInt16"
                     || portResolvedType == "Count"
                     || portResolvedType == "Index"
                     || portResolvedType == "Size"
                     || portResolvedType == "UInt32"
                     || portResolvedType == "DataSize"
                     || portResolvedType == "UInt64" )    {
                                                              unsigned int val = (unsigned int)(ULONG)xsiValue;
                                                              BaseInterface::SetValueOfArgUInt(*client, *binding, portName.GetAsciiString(), val);
                                                            }
            else if (   portResolvedType == "Scalar"
                     || portResolvedType == "Float32"
                     || portResolvedType == "Float64")    {
                                                              double val = (double)xsiValue;
                                                              BaseInterface::SetValueOfArgFloat(*client, *binding, portName.GetAsciiString(), val);
                                                            }
            else if (   portResolvedType == "String" )    {
                                                              std::string val = CString(xsiValue).GetAsciiString();
                                                              BaseInterface::SetValueOfArgString(*client, *binding, portName.GetAsciiString(), val);
                                                            }
            else
            {
              Application().LogMessage(L"the port \"" + portName + L"\" has the unsupported data type \"" + portResolvedType + L"\"", siWarningMsg)  ;
              storable = false;
            }
          }
        }

        if( storable ) {
          // Set ports added with a "storable type" as persistable so their values are 
          // exported if saving the graph
          // TODO: handle this in a "clean" way; here we are not in the context of an undo-able command.
          //       We would need that the DFG knows which binding types are "stored" as attributes on the
          //       DCC side and set these as persistable in the source "addPort" command.
          exec.setExecPortMetadata( portName.GetAsciiString(), DFG_METADATA_UIPERSISTVALUE, "true", false /* canUndo */ );
        }
      }
    }
    catch (FabricCore::Exception e)
    {
      std::string s = functionName.GetAsciiString() + std::string("(step 1): ") + (e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
      feLogError(s);
    }
  }

  // Fabric Engine (step 2): execute the DFG.
  if (pud->execFabricStep12)
  {
    if (verbose) Application().LogMessage(L"------- EXECUTE DFG.");
    pud->execFabricStep12 = false;
    try
    {
      binding->execute();
    }
    catch (FabricCore::Exception e)
    {
      std::string s = functionName.GetAsciiString() + std::string("(step 2): ") + (e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
      feLogError(s);
    }
  }

  // Fabric Engine (step 3): set the XSI output port from the matching DFG output port and/or set the ICE data.
  {
    CString portName = outputPort.GetName();
    FabricCore::DFGExec exec = binding->getExec();

    try
    {
      if (baseInterface->HasOutputPort(outputPort.GetName().GetAsciiString()))
      {
        if (verbose) Application().LogMessage(L"------- SET XSI PORT FROM DFG EXEC PORT.");
        if (verbose) Application().LogMessage(functionName + L": transfer dfg port data to xsi port \"" + outputPort.GetName() + L"\"");
        if (!exec.haveExecPort(portName.GetAsciiString()))
          Application().LogMessage(functionName + L": graph->getExecPort() == NULL", siWarningMsg);
        else
        {
          if      (outputPort.GetTarget().GetClassID() == siUnknownClassID)
          {
            // do nothing.
          }
          else if (outputPort.GetTarget().GetClassID() == siKinematicStateID)
          {
            std::vector <double> val;
            if (BaseInterface::GetArgValueMat44(*binding, portName.GetAsciiString(), val) != 0)
              Application().LogMessage(functionName + L": BaseInterface::GetArgValueMat44(port) failed.", siWarningMsg);
            else
            {
              KinematicState kineOut(ctxt.GetOutputTarget());
              MATH::CTransformation t;
              MATH::CMatrix4 m;
              m.SetValue(0, 0, val[ 0]); // row 0.
              m.SetValue(1, 0, val[ 1]);
              m.SetValue(2, 0, val[ 2]);
              m.SetValue(3, 0, val[ 3]);
              m.SetValue(0, 1, val[ 4]); // row 1.
              m.SetValue(1, 1, val[ 5]);
              m.SetValue(2, 1, val[ 6]);
              m.SetValue(3, 1, val[ 7]);
              m.SetValue(0, 2, val[ 8]); // row 2.
              m.SetValue(1, 2, val[ 9]);
              m.SetValue(2, 2, val[10]);
              m.SetValue(3, 2, val[11]);
              m.SetValue(0, 3, val[12]); // row 3.
              m.SetValue(1, 3, val[13]);
              m.SetValue(2, 3, val[14]);
              m.SetValue(3, 3, val[15]);
              t.SetMatrix4(m);
              kineOut.PutTransform(t);
            }
          }
          else if (outputPort.GetTarget().GetClassID() == siPrimitiveID)
          {
            PolygonMesh xsiPolymesh(Primitive(outputPort.GetTarget()).GetGeometry());
            if (!xsiPolymesh.IsValid())
              Application().LogMessage(L"PolygonMesh(Primitive(outputPort.GetTarget()).GetGeometry()) failed", siErrorMsg);
            else
            {
              _polymesh polymesh;

              int ret = polymesh.setFromDFGArg(*binding, portName.GetAsciiString());
              if (ret)
                Application().LogMessage(functionName + L": failed to get mesh from DFG port \"" + portName + L"\" (returned " + CString(ret) + L")", siWarningMsg);
              else
              {
                if (verbose)
                {
                  Application().LogMessage(L"DFG port PolygonMesh: " + CString((LONG)polymesh.numVertices) + L" vertices, " + CString((LONG)polymesh.numPolygons) + L" polygons, " + CString((LONG)polymesh.numSamples) + L" nodes.");
                  Application().LogMessage(L"                      has UVWs:   " + CString(polymesh.hasUVWs()   ? L"yes" : L"no") + L".");
                  Application().LogMessage(L"                      has colors: " + CString(polymesh.hasColors() ? L"yes" : L"no") + L".");
                }

                MATH::CVector3Array vertices(polymesh.numVertices);
                CLongArray          polygons(polymesh.numPolygons + polymesh.numSamples);
                float *pv = polymesh.vertPositions.data();
                for (LONG i=0;i<polymesh.numVertices;i++,pv+=3)
                  vertices[i].Set(pv[0], pv[1], pv[2]);
                LONG *dst = (LONG *)polygons.GetArray();
                uint32_t *src = polymesh.polyVertices.data();
                for (LONG i=0;i<polymesh.numPolygons;i++)
                {
                  LONG num = polymesh.polyNumVertices[i];
                  *dst = num;
                  dst++;
                  for (LONG j=0;j<num;j++,src++, dst++)
                    *dst = *src;
                }
                if (xsiPolymesh.Set(vertices, polygons) != CStatus::OK)
                {
                  Application().LogMessage(L"xsiPolymesh.Set(vertices, polygons) failed", siErrorMsg);
                }
                else
                {
                  // store normals, UVWs and vertex colors as ICE data.
                  //
                  // note: attempting to directly set "per point" or "per node" ICE data fails, because attributes added
                  //       via XSI::Geometry::AddICEAttribute() be constant (ICEAttribute::IsConstant() == true) and there
                  //       seems no way to get around this.
                  //       Therefore the data (normals, colors, UVs, ..) is stored as an "array per object" which must then be
                  //       converted to "per point" or "per node" in ICE by the user.

                  // array of normals per polygon node.
                  {
                    typedef                 MATH::CVector3f T;
                    CString                 name          = L"FabricCanvasDataArrayNormalsPerNode";
                    siICENodeDataType       typeData      = siICENodeDataVector3;
                    siICENodeStructureType  typeStructure = siICENodeStructureArray;
                    siICENodeContextType    typeContext   = siICENodeContextSingleton;
                    ICEAttribute            attr          = xsiPolymesh.GetICEAttributeFromName(name);
                    if (!attr.IsValid())    attr          = xsiPolymesh.AddICEAttribute(name, typeData, typeStructure, typeContext);
                    if (!attr.IsValid())
                    {
                      Application().LogMessage(L"failed to get or create ICE attribute \"" + name + L"\"", siErrorMsg);
                    }
                    else if (   attr.GetDataType()      != typeData
                             || attr.GetStructureType() != typeStructure
                             || attr.GetContextType()   != typeContext)
                    {
                      Application().LogMessage(L"the ICE attribute \"" + name + L"\" already exists, but has the wrong type or context", siErrorMsg);
                    }
                    else if (attr.IsReadonly())
                    {
                      Application().LogMessage(L"the ICE attribute \"" + name + L"\" is read-only", siErrorMsg);
                    }
                    else
                    {
                      CICEAttributeDataArray2DVector3f data2D;
                      if (attr.GetDataArray2D(data2D) != CStatus::OK)
                      {
                        Application().LogMessage(L"failed to get the ICE data \"" + name + L"\"", siErrorMsg);
                      }
                      else if (data2D.GetCount() != 1)
                      {
                        Application().LogMessage(L"something's wrong with the ICE data \"" + name + L"\", data2D.GetCount() is " + CString(data2D.GetCount()) + L" but we expected 1.", siErrorMsg);
                      }
                      else if (polymesh.numSamples > 0)
                      {
                        // init temporary array.
                        T *tmp = NULL;
                        tmp = new T[polymesh.numSamples];

                        // fill tmp.
                        float *pnd = polymesh.polyNodeNormals.data();
                        for (LONG i=0;i<polymesh.numSamples;i++,pnd+=3)
                          tmp[i].Set(pnd[0], pnd[1], pnd[2]);

                        // set ICE sub array from tmp.
                        if (data2D.SetSubArray(0, tmp, polymesh.numSamples) != CStatus::OK)
                          Application().LogMessage(L"failed to set data in ICE data \"" + name + L"\"", siErrorMsg);

                        // clean up.
                        delete[] tmp;
                      }
                    }
                  }

                  // array of UVWs per polygon node.
                  if (polymesh.hasUVWs())
                  {
                    typedef                 MATH::CVector3f T;
                    CString                 name          = L"FabricCanvasDataArrayUVWPerNode";
                    siICENodeDataType       typeData      = siICENodeDataVector3;
                    siICENodeStructureType  typeStructure = siICENodeStructureArray;
                    siICENodeContextType    typeContext   = siICENodeContextSingleton;
                    ICEAttribute            attr          = xsiPolymesh.GetICEAttributeFromName(name);
                    if (!attr.IsValid())    attr          = xsiPolymesh.AddICEAttribute(name, typeData, typeStructure, typeContext);
                    if (!attr.IsValid())
                    {
                      Application().LogMessage(L"failed to get or create ICE attribute \"" + name + L"\"", siErrorMsg);
                    }
                    else if (   attr.GetDataType()      != typeData
                             || attr.GetStructureType() != typeStructure
                             || attr.GetContextType()   != typeContext)
                    {
                      Application().LogMessage(L"the ICE attribute \"" + name + L"\" already exists, but has the wrong type or context", siErrorMsg);
                    }
                    else if (attr.IsReadonly())
                    {
                      Application().LogMessage(L"the ICE attribute \"" + name + L"\" is read-only", siErrorMsg);
                    }
                    else
                    {
                      CICEAttributeDataArray2DVector3f data2D;
                      if (attr.GetDataArray2D(data2D) != CStatus::OK)
                      {
                        Application().LogMessage(L"failed to get the ICE data \"" + name + L"\"", siErrorMsg);
                      }
                      else if (data2D.GetCount() != 1)
                      {
                        Application().LogMessage(L"something's wrong with the ICE data \"" + name + L"\", data2D.GetCount() is " + CString(data2D.GetCount()) + L" but we expected 1.", siErrorMsg);
                      }
                      else if (polymesh.numSamples > 0)
                      {
                        // init temporary array.
                        T *tmp = NULL;
                        tmp = new T[polymesh.numSamples];

                        // fill tmp.
                        float *pnd = polymesh.polyNodeUVWs.data();
                        for (LONG i=0;i<polymesh.numSamples;i++,pnd+=3)
                          tmp[i].Set(pnd[0], pnd[1], pnd[2]);

                        // set ICE sub array from tmp.
                        if (data2D.SetSubArray(0, tmp, polymesh.numSamples) != CStatus::OK)
                          Application().LogMessage(L"failed to set data in ICE data \"" + name + L"\"", siErrorMsg);

                        // clean up.
                        delete[] tmp;
                      }
                    }
                  }

                  // array of colors per polygon node.
                  if (polymesh.hasColors())
                  {
                    typedef                 MATH::CColor4f  T;
                    CString                 name          = L"FabricCanvasDataArrayColorPerNode";
                    siICENodeDataType       typeData      = siICENodeDataColor4;
                    siICENodeStructureType  typeStructure = siICENodeStructureArray;
                    siICENodeContextType    typeContext   = siICENodeContextSingleton;
                    ICEAttribute            attr          = xsiPolymesh.GetICEAttributeFromName(name);
                    if (!attr.IsValid())    attr          = xsiPolymesh.AddICEAttribute(name, typeData, typeStructure, typeContext);
                    if (!attr.IsValid())
                    {
                      Application().LogMessage(L"failed to get or create ICE attribute \"" + name + L"\"", siErrorMsg);
                    }
                    else if (   attr.GetDataType()      != typeData
                             || attr.GetStructureType() != typeStructure
                             || attr.GetContextType()   != typeContext)
                    {
                      Application().LogMessage(L"the ICE attribute \"" + name + L"\" already exists, but has the wrong type or context", siErrorMsg);
                    }
                    else if (attr.IsReadonly())
                    {
                      Application().LogMessage(L"the ICE attribute \"" + name + L"\" is read-only", siErrorMsg);
                    }
                    else
                    {
                      CICEAttributeDataArray2DColor4f data2D;
                      if (attr.GetDataArray2D(data2D) != CStatus::OK)
                      {
                        Application().LogMessage(L"failed to get the ICE data \"" + name + L"\"", siErrorMsg);
                      }
                      else if (data2D.GetCount() != 1)
                      {
                        Application().LogMessage(L"something's wrong with the ICE data \"" + name + L"\", data2D.GetCount() is " + CString(data2D.GetCount()) + L" but we expected 1.", siErrorMsg);
                      }
                      else if (polymesh.numSamples > 0)
                      {
                        // init temporary array.
                        T *tmp = NULL;
                        tmp = new T[polymesh.numSamples];

                        // fill tmp.
                        float *pnd = polymesh.polyNodeColors.data();
                        for (LONG i=0;i<polymesh.numSamples;i++,pnd+=4)
                          tmp[i].Set(pnd[0], pnd[1], pnd[2], pnd[3]);

                        // set ICE sub array from tmp.
                        if (data2D.SetSubArray(0, tmp, polymesh.numSamples) != CStatus::OK)
                          Application().LogMessage(L"failed to set data in ICE data \"" + name + L"\"", siErrorMsg);

                        // clean up.
                        delete[] tmp;
                      }
                    }
                  }
                }
              }
            }
          }
          else if (outputPort.GetTarget().GetClassID() == siCustomPropertyID)
          {
            double val;
            if (BaseInterface::GetArgValueFloat(*binding, portName.GetAsciiString(), val) != 0)
              Application().LogMessage(functionName + L": BaseInterface::GetArgValueFloat(port) failed.", siWarningMsg);
            else
            {
              CustomProperty propOut(ctxt.GetOutputTarget());
              propOut.PutParameterValue(L"value", val);
            }
          }
          else
          {
            CString emptyString;
            Application().LogMessage(L"XSI output port \"" + outputPort.GetName() + L"\" has the unsupported classID \"" + dfgTools::GetSiClassIdDescription(outputPort.GetTarget().GetClassID(), emptyString) + L"\"", siWarningMsg);
          }
        }
      }
    }
    catch (FabricCore::Exception e)
    {
      std::string s = functionName.GetAsciiString() + std::string("(step 3): ") + (e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
      feLogError(s);
    }
  }
  
  // activate exec flags?
  if (activateExecFlagsForNextUpdate)
  {
    pud->execFabricStep12 = true;
  }

  // done.
  return CStatus::OK;
}

// returns: -1: error.
//           0: canceled by user or no changes made.
//           1: success.
int Dialog_DefinePortMapping(std::vector<_portMapping> &io_pmap)
{
  // nothing to do?
  if (io_pmap.size() == 0)
    return 0;

  // create the temporary custom property.
  CustomProperty cp = static_cast<CustomProperty>(Application().GetActiveSceneRoot().AddProperty(L"CustomProperty", false, L"dfgDefinePortMapping"));
  if (!cp.IsValid())
  { Application().LogMessage(L"failed to create custom property!", siErrorMsg);
    return -1; }
  
  // declare/init return value.
  int ret = 1;

  // add parameters.
  if (ret)
  {
    for (int i=0;i<io_pmap.size();i++)
    {
      _portMapping &pmap = io_pmap[i];

      // read-only grid with port name, type, etc.
      {
        Parameter gridParam = cp.AddGridParameter(L"grid" + CString(i));
        GridData grid((CRef)gridParam.GetValue());

        grid.PutRowCount(1);
        grid.PutColumnCount(3);
        grid.PutColumnLabel(0, "Name");         grid.PutColumnType(0, siColumnStandard);
        grid.PutColumnLabel(1, "Type");         grid.PutColumnType(1, siColumnStandard);
        grid.PutColumnLabel(2, "Mode");         grid.PutColumnType(2, siColumnStandard);
        grid.PutReadOnly(true);

        CString name = pmap.dfgPortName;
        CString type = pmap.dfgPortDataType;
        CString mode;
        switch (pmap.dfgPortType)
        {
          case DFG_PORT_TYPE_IN:    mode = L"In";         break;
          case DFG_PORT_TYPE_OUT:   mode = L"Out";        break;
          default:                  mode = L"undefined";  break;
        }

        grid.PutRowLabel( 0, L"  " + CString(i) + L"  ");
        grid.PutCell(0,   0, L" "  + name);
        grid.PutCell(1,   0, L" "  + type);
        grid.PutCell(2,   0, L" "  + mode);
      }

      // other params.
      {
        // map type.
        Parameter emptyParam;
        cp.AddParameter(L"MapType" + CString(i), CValue::siInt4, 0, L"Type/Target", L"", pmap.mapType, emptyParam);
      }
    }
  }

  // define layout.
  if (ret)
  {
    PPGItem pi;
    PPGLayout oLayout = cp.GetPPGLayout();
    oLayout.Clear();
    for (int i=0;i<io_pmap.size();i++)
    {
      _portMapping &pmap = io_pmap[i];

      oLayout.AddGroup(pmap.dfgPortName);
        oLayout.AddRow();
          oLayout.AddGroup(L"", false, 50);
            pi = oLayout.AddItem(L"grid" + CString(i));
            pi.PutAttribute(siUINoLabel,              true);
            pi.PutAttribute(siUIGridSelectionMode,    siSelectionNone);
            pi.PutAttribute(siUIGridHideColumnHeader, false);
            pi.PutAttribute(siUIGridLockColumnWidth,  true);
            pi.PutAttribute(siUIGridColumnWidths,     "20:128:64:40");
          oLayout.EndGroup();
          oLayout.AddGroup(L"", false, 5);
            oLayout.AddStaticText(L" ");
          oLayout.EndGroup();
          oLayout.AddGroup(L"", false);
          {
            CValueArray cvaMapType;
            cvaMapType.Add( L"Internal" );
            cvaMapType.Add( DFG_PORT_MAPTYPE_INTERNAL );
            if (pmap.dfgPortType == DFG_PORT_TYPE_IN)
            {
              if (   pmap.dfgPortDataType == L"Boolean"

                  || pmap.dfgPortDataType == L"Scalar"
                  || pmap.dfgPortDataType == L"Float32"
                  || pmap.dfgPortDataType == L"Float64"

                  || pmap.dfgPortDataType == L"Integer"
                  || pmap.dfgPortDataType == L"SInt8"
                  || pmap.dfgPortDataType == L"SInt16"
                  || pmap.dfgPortDataType == L"SInt32"
                  || pmap.dfgPortDataType == L"SInt64"

                  || pmap.dfgPortDataType == L"Byte"
                  || pmap.dfgPortDataType == L"UInt8"
                  || pmap.dfgPortDataType == L"UInt16"
                  || pmap.dfgPortDataType == L"Count"
                  || pmap.dfgPortDataType == L"Index"
                  || pmap.dfgPortDataType == L"Size"
                  || pmap.dfgPortDataType == L"UInt32"
                  || pmap.dfgPortDataType == L"DataSize"
                  || pmap.dfgPortDataType == L"UInt64"

                  || pmap.dfgPortDataType == L"String")
              {
                cvaMapType.Add( L"XSI Parameter" );
                cvaMapType.Add( DFG_PORT_MAPTYPE_XSI_PARAMETER );
              }
              if (   pmap.dfgPortDataType == L"Mat44"
                  || pmap.dfgPortDataType == L"Mat44[]"
                  || pmap.dfgPortDataType == L"Xfo"
                  || pmap.dfgPortDataType == L"Xfo[]"

                  || pmap.dfgPortDataType == L"PolygonMesh")
              {
                cvaMapType.Add( L"XSI Port" );
                cvaMapType.Add( DFG_PORT_MAPTYPE_XSI_PORT );
              }
            }
            else if (pmap.dfgPortType == DFG_PORT_TYPE_OUT)
            {
              if (   pmap.dfgPortDataType == L"Mat44"
                  || pmap.dfgPortDataType == L"Xfo"
                  
                  /* we do not expose this in the UI, because this
                     is an undocumented "hack" that was implemented
                     for Kraken.

                  || pmap.dfgPortDataType == L"Boolean"

                  || pmap.dfgPortDataType == L"Scalar"
                  || pmap.dfgPortDataType == L"Float32"
                  || pmap.dfgPortDataType == L"Float64"

                  || pmap.dfgPortDataType == L"Integer"
                  || pmap.dfgPortDataType == L"SInt8"
                  || pmap.dfgPortDataType == L"SInt16"
                  || pmap.dfgPortDataType == L"SInt32"
                  || pmap.dfgPortDataType == L"SInt64"

                  || pmap.dfgPortDataType == L"Byte"
                  || pmap.dfgPortDataType == L"UInt8"
                  || pmap.dfgPortDataType == L"UInt16"
                  || pmap.dfgPortDataType == L"Count"
                  || pmap.dfgPortDataType == L"Index"
                  || pmap.dfgPortDataType == L"Size"
                  || pmap.dfgPortDataType == L"UInt32"
                  || pmap.dfgPortDataType == L"DataSize"
                  || pmap.dfgPortDataType == L"UInt64"
                  */

                  || pmap.dfgPortDataType == L"PolygonMesh")
              {
                cvaMapType.Add( L"XSI Port" );
                cvaMapType.Add( DFG_PORT_MAPTYPE_XSI_PORT );
              }
            }

            oLayout.AddEnumControl(L"MapType" + CString(i), cvaMapType, CString(), siControlRadio);
          }
          oLayout.EndGroup();
        oLayout.EndRow();
      oLayout.EndGroup();
    }
  }

  // inspect the custom property as a modal dialog.
  if (ret)
  {
    CValue r;
    CValueArray args;
    args.Add(cp.GetRef().GetAsText());
    args.Add(L"");
    args.Add(L"Define Port Mapping");
    args.Add(siModal);
    args.Add(false);
    if (Application().ExecuteCommand(L"InspectObj", args, r) != CStatus::OK)
    { Application().LogMessage(L"InspectObj failed", siErrorMsg);
      ret = -1; }
    if (r == true)  // canceled by user.
      ret = 0;
  }

  // update io_pm from the custom property.
  if (ret)
  {
    ret = 0;

    for (int i=0;i<io_pmap.size();i++)
    {
      _portMapping &pmap = io_pmap[i];

      DFG_PORT_MAPTYPE mem = pmap.mapType;
      pmap.mapType = (DFG_PORT_MAPTYPE)(int)cp.GetParameterValue(L"MapType" + CString(i));
      if (pmap.mapType != mem)
        ret = 1;

      if (!pmap.isValid())
      {
        ret = -1;
        break;
      }
    }
  }

  // clean up.
  {
    // delete the custom property.
    CValueArray args;
    args.Add(cp.GetRef().GetAsText());
    CValue emptyValue;
    if (Application().ExecuteCommand(L"DeleteObj", args, emptyValue) != CStatus::OK)
      Application().LogMessage(L"DeleteObj failed", siWarningMsg);
  }

  // done.
  return ret;
}
