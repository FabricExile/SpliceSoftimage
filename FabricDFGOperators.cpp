#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>

#include <xsi_command.h>
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
#include <xsi_griddata.h>
#include <xsi_selection.h>
#include <xsi_project.h>
#include <xsi_port.h>
#include <xsi_inputport.h>
#include <xsi_outputport.h>
#include <xsi_utils.h>
#include <xsi_matrix4.h>
#include <xsi_kinematics.h>
#include <xsi_customoperator.h>
#include <xsi_operatorcontext.h>

// project includes
#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"
#include "FabricDFGBaseInterface.h"

std::map <unsigned int, _opUserData *> _opUserData::s_instances;

using namespace XSI;

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Init(CRef &in_ctxt)
{
  // beta log.
  CString functionName = L"dfgSoftimageOp_Init()";
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

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Term(CRef &in_ctxt)
{
  // beta log.
  CString functionName = L"dfgSoftimageOp_Term()";
  if (FabricDFGPlugin_BETA) Application().LogMessage(functionName + L" called", siInfoMsg);

  // init.
  Context ctxt(in_ctxt);
  _opUserData *pud = (_opUserData *)((ULLONG)ctxt.GetUserData());

  // clean up.
  if (pud)
  {
    delete pud;
  }

  // done.
  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Define(CRef &in_ctxt)
{
  // beta log.
  CString functionName = L"dfgSoftimageOp_Define()";
  if (FabricDFGPlugin_BETA) Application().LogMessage(functionName + L" called", siInfoMsg);

  // init.
  Context ctxt(in_ctxt);
  CustomOperator op = ctxt.GetSource();
  Factory oFactory = Application().GetFactory();
  CRef oPDef;

  // create default parameter(s).
  oPDef = oFactory.CreateParamDef(L"FabricActive",        CValue::siBool,   siPersistable | siAnimatable | siKeyable, L"", L"", true, CValue(), CValue(), CValue(), CValue());
  op.AddParameter(oPDef, Parameter());
  oPDef = oFactory.CreateParamDef(L"evalID",              CValue::siInt4,   siReadOnly,                               L"", L"", 0, 0, 1000, 0, 1000);
  op.AddParameter(oPDef, Parameter());
  oPDef = oFactory.CreateParamDef(L"persistenceData",     CValue::siString, siReadOnly,                               L"", L"", "", "", "", "", "");
  op.AddParameter(oPDef, Parameter());
  oPDef = oFactory.CreateParamDef(L"alwaysConvertMeshes", CValue::siBool,   siPersistable,                            L"", L"", false, CValue(), CValue(), CValue(), CValue());
  op.AddParameter(oPDef, Parameter());

  // create grid parameter(s).
  op.AddParameter(oFactory.CreateGridParamDef("dfgPorts"), Parameter());

  // set default values.
  op.PutAlwaysEvaluate(false);
  op.PutDebug(0);

  // done.
  return CStatus::OK;
}

int dfgSoftimageOp_UpdateGridData_dfgPorts(CustomOperator &op)
{
  // get grid object.
  GridData grid((CRef)op.GetParameter("dfgPorts").GetValue());

  // get operator's user data and check if there are any DFG ports.
  _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
  if (   !pud
      || !pud->GetBaseInterface()
      || !pud->GetBaseInterface()->getGraph()
      || !pud->GetBaseInterface()->getGraph()->getPorts().size())
  {
    // clear grid data.
    grid = GridData();
    return 0;
  }

  // set amount, name and type of the grid columns.
  grid.PutColumnCount(4);
  grid.PutColumnLabel(0, " Name ");   grid.PutColumnType(0, siColumnStandard);
  grid.PutColumnLabel(1, " Type ");   grid.PutColumnType(1, siColumnStandard);
  grid.PutColumnLabel(2, " Mode ");   grid.PutColumnType(2, siColumnStandard);
  grid.PutColumnLabel(3, " Target "); grid.PutColumnType(3, siColumnStandard);

  // get all DFG ports.
  FabricServices::DFGWrapper::PortList ports = pud->GetBaseInterface()->getGraph()->getPorts();

  // set data.
  grid.PutRowCount(ports.size());
  for (int i=0;i<ports.size();i++)
  {
    // ref at current port.
    FabricServices::DFGWrapper::Port &port = *ports[i];

    // name.
    CString name(port.getName());

    // type (= resolved data type).
    CString type(port.getResolvedType());

    // mode (= DFGPortType).
    CString mode;
    if      (port.getPortType() == FabricCore::DFGPortType_In)  mode = L"In";
    else if (port.getPortType() == FabricCore::DFGPortType_Out) mode = L"Out";
    else                                                        mode = L"IO";

    // target.
    CString target = L"<internal>";
    
    // set grid data.
    grid.PutRowLabel( i, L"  " + CString(i) + L"  ");
    grid.PutCell(0,   i, name);
    grid.PutCell(1,   i, type);
    grid.PutCell(2,   i, mode);
    grid.PutCell(3,   i, target);
  }

  // return amount of rows.
  return ports.size();
}

void dfgSoftimageOp_DefineLayout(PPGLayout &oLayout, CustomOperator &op)
{
  // init.
  oLayout.Clear();
  oLayout.PutAttribute(siUIDictionary, L"None");
  PPGItem pi;

  // button size (Open Canvas).
  const LONG btnCx = 100;
  const LONG btnCy = 28;

  // button size (Refresh).
  const LONG btnRx = 93;
  const LONG btnRy = 28;

  // button size (tools).
  const LONG btnTx = 112;
  const LONG btnTy = 24;

  // button size (tools - select connected).
  const LONG btnTSCx = 90;
  const LONG btnTSCy = 22;

  // update the grid data.
  const int dfgPortsNumRows = dfgSoftimageOp_UpdateGridData_dfgPorts(op);

  // Tab "Main"
  {
    oLayout.AddTab(L"Main");
    {
      oLayout.AddGroup(L"", false);
        oLayout.AddRow();
          oLayout.AddItem(L"FabricActive", L"Execute Fabric DFG");
          oLayout.AddSpacer(0, 0);
          pi = oLayout.AddButton(L"BtnOpenCanvas", L"Open Canvas");
          pi.PutAttribute(siUIButtonDisable, true);
          pi.PutAttribute(siUICX, btnCx);
          pi.PutAttribute(siUICY, btnCy);
        oLayout.EndRow();
      oLayout.EndGroup();
      oLayout.AddSpacer(0, 12);

      oLayout.AddGroup(L"Parameters");
        bool hasParams = false;
        {
          // todo: add input parameters from graph.
        }
        if (!hasParams)
        {
          oLayout.AddSpacer(0, 8);
          oLayout.AddStaticText(L"   ... no Parameters available.");
          oLayout.AddSpacer(0, 8);
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
          oLayout.AddItem(L"FabricActive", L"Execute Fabric DFG");
          oLayout.AddSpacer(0, 0);
          pi = oLayout.AddButton(L"BtnOpenCanvas", L"Open Canvas");
          pi.PutAttribute(siUIButtonDisable, true);
          pi.PutAttribute(siUICX, btnCx);
          pi.PutAttribute(siUICY, btnCy);
        oLayout.EndRow();
      oLayout.EndGroup();
      oLayout.AddSpacer(0, 12);

      oLayout.AddGroup(L"DFG Ports");
        oLayout.AddRow();
          oLayout.AddSpacer(0, 0);
          pi = oLayout.AddButton(L"BtnRefreshPPG", L"Refresh");
          pi.PutAttribute(siUICX, btnRx);
          pi.PutAttribute(siUICY, btnRy);
        oLayout.EndRow();
        oLayout.AddSpacer(0, 6);
        if (dfgPortsNumRows)
        {
          pi = oLayout.AddItem("dfgPorts", "dfgPorts", siControlGrid);
          pi.PutAttribute(siUINoLabel,             true);
          pi.PutAttribute(siUIWidthPercentage,     100);
          pi.PutAttribute(siUIGridReadOnlyColumns, "1:1:1:1:1");
        }
        else
        {
          oLayout.AddStaticText(L"   ... no Ports available.");
          oLayout.AddSpacer(0, 8);
        }
      oLayout.EndGroup();
      oLayout.AddSpacer(0, 4);

      oLayout.AddGroup(L"Tools");
        oLayout.AddRow();
          pi = oLayout.AddButton(L"BtnLogDFGInfo", L"Log DFG Info");
          pi.PutAttribute(siUICX, btnTx);
          pi.PutAttribute(siUICY, btnTy);
        oLayout.EndRow();
        oLayout.AddSpacer(0, 4);
        oLayout.AddRow();
          pi = oLayout.AddButton(L"BtnImportJSON", L"Import JSON");
          pi.PutAttribute(siUIButtonDisable, true);
          pi.PutAttribute(siUICX, btnTx);
          pi.PutAttribute(siUICY, btnTy);
          pi = oLayout.AddButton(L"BtnExportJSON", L"Export JSON");
          pi.PutAttribute(siUICX, btnTx);
          pi.PutAttribute(siUICY, btnTy);
        oLayout.EndRow();
        oLayout.AddSpacer(0, 8);
        oLayout.AddGroup(L"Select connected Objects");
          oLayout.AddRow();
            pi = oLayout.AddButton(L"BtnSelConnectAll", L"All");
            pi.PutAttribute(siUIButtonDisable, true);
            pi.PutAttribute(siUICX, btnTSCx);
            pi.PutAttribute(siUICY, btnTSCy);
            pi = oLayout.AddButton(L"BtnSelConnectIn",  L"Input only");
            pi.PutAttribute(siUIButtonDisable, true);
            pi.PutAttribute(siUICX, btnTSCx);
            pi.PutAttribute(siUICY, btnTSCy);
            pi = oLayout.AddButton(L"BtnSelConnectOut", L"Output only");
            pi.PutAttribute(siUIButtonDisable, true);
            pi.PutAttribute(siUICX, btnTSCx);
            pi.PutAttribute(siUICY, btnTSCy);
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
      oLayout.AddItem(L"evalID",              L"evalID");
      oLayout.AddItem(L"persistenceData",     L"persistenceData");
      oLayout.AddItem(L"alwaysConvertMeshes", L"Always Convert Meshes");
      oLayout.AddItem(L"mute",                L"Mute");
      oLayout.AddItem(L"alwaysevaluate",      L"Always Evaluate");
      oLayout.AddItem(L"debug",               L"Debug");
      oLayout.AddItem(L"Name",                L"Name");
    }
  }
}

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_PPGEvent(const CRef &in_ctxt)
{
  /*
      note:

      if the value of a parameter changes but the UI is not shown then this
      code will not execute. Also this code is not re-entrant, so any changes
      to parameters inside this code will not result in further calls to this function.

  */

  // init.
  PPGEventContext ctxt(in_ctxt);
  CustomOperator op(ctxt.GetSource());
  PPGEventContext::PPGEvent eventID = ctxt.GetEventID();
  UIToolkit toolkit = Application().GetUIToolkit();

  // process event.
  if (eventID == PPGEventContext::siOnInit)
  {
    dfgSoftimageOp_DefineLayout(op.GetPPGLayout(), op);
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
      LONG ret;
      toolkit.MsgBox(L"not yet implemented", siMsgOkOnly, "dfgSoftimageOp", ret);
    }
    else if (btnName == L"BtnImportJSON")
    {
      LONG ret;
      toolkit.MsgBox(L"not yet implemented", siMsgOkOnly, "dfgSoftimageOp", ret);
    }
    else if (btnName == L"BtnExportJSON")
    {
      CComAPIHandler toolkit;
      toolkit.CreateInstance(L"XSI.UIToolkit");
      CString ext = ".dfg.json";

      CComAPIHandler filebrowser(toolkit.GetProperty(L"FileBrowser"));
      filebrowser.PutProperty(L"InitialDirectory", Application().GetActiveProject().GetPath());
      filebrowser.PutProperty(L"Filter", L"DFG Preset (*" + ext + L")|*" + ext + L"||");

      CValue returnVal;
      filebrowser.Call(L"ShowSave", returnVal);
      CString uiFileName = filebrowser.GetProperty(L"FilePathName").GetAsText();
      if(uiFileName.IsEmpty())
      { Application().LogMessage(L"aborted by user.", siWarningMsg);
        return CStatus::OK; }

      // convert backslashes to slashes.
      CString fileName;
      for(ULONG i=0;i<uiFileName.Length();i++)
      {
        if(uiFileName.GetSubString(i, 1).IsEqualNoCase(L"\\"))
          fileName += L"/";
        else
          fileName += uiFileName[i];
      }

      // take care of possible double extension (e.g. "myProfile.gfd.json.gfd.json").
      ULONG dfg1 = fileName.ReverseFindString(ext, UINT_MAX);
      if (dfg1 != UINT_MAX)
      {
        ULONG dfg2 = fileName.ReverseFindString(ext, dfg1 - 1);
        if (dfg2 != UINT_MAX && dfg2 + ext.Length() == dfg1)
        {
          fileName = fileName.GetSubString(0, fileName.Length() - ext.Length());
        }
      }

      // call command.
      CValueArray args;
      args.Add(op.GetUniqueName());
      args.Add(fileName);
      Application().ExecuteCommand(L"dfgExportJSON", args, CValue());
    }
    else if (btnName == L"BtnSelConnectAll")
    {
      LONG ret;
      toolkit.MsgBox(L"not yet implemented", siMsgOkOnly, "dfgSoftimageOp", ret);
    }
    else if (btnName == L"BtnSelConnectIn")
    {
      LONG ret;
      toolkit.MsgBox(L"not yet implemented", siMsgOkOnly, "dfgSoftimageOp", ret);
    }
    else if (btnName == L"BtnSelConnectOut")
    {
      LONG ret;
      toolkit.MsgBox(L"not yet implemented", siMsgOkOnly, "dfgSoftimageOp", ret);
    }
    else if (btnName == L"BtnRefreshPPG")
    {
      dfgSoftimageOp_DefineLayout(op.GetPPGLayout(), op);
      ctxt.PutAttribute(L"Refresh", true);
    }
    else if (btnName == L"BtnLogDFGInfo")
    {
      try
      {
        // init/check.
        _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
        if (!pud)                                 { Application().LogMessage(L"no user data found!",      siErrorMsg);  return CStatus::OK; }
        if (!pud->GetBaseInterface())             { Application().LogMessage(L"no base interface found!", siErrorMsg);  return CStatus::OK; }
        if (!pud->GetBaseInterface()->getGraph()) { Application().LogMessage(L"no graph found!",          siErrorMsg);  return CStatus::OK; }

        // log (DFG ports).
        FabricServices::DFGWrapper::PortList ports = pud->GetBaseInterface()->getGraph()->getPorts();
        Application().LogMessage(L"\"" + op.GetRef().GetAsText() + L"\" (ObjectID = " + CString(op.GetObjectID()) + L") has a DFG with " + CString((LONG)ports.size()) + L" port(s)" + (ports.size() > 0 ? L":" : L"."), siInfoMsg);
        for (int i=0;i<ports.size();i++)
        {
          FabricServices::DFGWrapper::Port &port = *ports[i];
          char s[16];
          sprintf(s, "% 3ld", i);
          CString t;
          t  = L"  " + CString(s) + L". ";
          if      (port.getPortType() == FabricCore::DFGPortType_In)  t += L"type = \"In\",  ";
          else if (port.getPortType() == FabricCore::DFGPortType_Out) t += L"type = \"Out\", ";
          else                                                        t += L"type = \"IO\",  ";
          t += L"name = \"" + CString(port.getName()) + L"\", ";
          t += L"resolved data type = \"" + CString(port.getResolvedType()) + L"\"";
          Application().LogMessage(t, siInfoMsg);
        }

        // log (XSI ports).
        CRefArray inPorts  = op.GetInputPorts();
        CRefArray outPorts = op.GetOutputPorts();
        LONG numPorts = inPorts.GetCount() + outPorts.GetCount();
        Application().LogMessage(L"as well as " + CString((LONG)ports.size()) + L" XSI port(s)" + (numPorts > 0 ? L":" : L"."), siInfoMsg);
        for (int i=0;i<inPorts.GetCount();i++)
        {
          InputPort port(inPorts[i]);
          char s[16];
          sprintf(s, "% 3ld", i);
          CString t;
          t  = L"  " + CString(s) + L". ";
          t += L"type = \"In\",  ";
          t += L"name = \"" + port.GetName() + L"\", ";
          t += L"target = \"" + port.GetTargetPath() + L"\"";
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
          t += L"target = \"" + port.GetTargetPath() + L"\"";
          Application().LogMessage(t, siInfoMsg);
        }
      }
      catch (FabricCore::Exception e)
      {
        feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
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

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Update(CRef &in_ctxt)
{
  // init.
  OperatorContext ctxt(in_ctxt);
  CustomOperator op(ctxt.GetSource());

  // init log.
  CString functionName = L"dfgSoftimageOp_Update()";
  const bool debugLog = ((LONG)ctxt.GetParameterValue(L"debug") != 0);
  if (debugLog) Application().LogMessage(functionName + L" called", siInfoMsg);

  // check the FabricActive parameter.
  if (!(bool)ctxt.GetParameterValue(L"FabricActive"))
    return CStatus::OK;

  // get the output port that is currently being evaluated.
  OutputPort outputPort(ctxt.GetOutputPort());
  if (debugLog) Application().LogMessage(functionName + L": evaluating output port \"" + outputPort.GetName() + L"\"");

  // get default input.
  KinematicState kineIn((CRef)ctxt.GetInputValue(L"reservedMatrixIn"));
  MATH::CMatrix4 matrixIn = kineIn.GetTransform().GetMatrix4();
  MATH::CTransformation gt;
  gt.SetMatrix4(matrixIn);

  // test (do something with gt).
  {
    double x, y, z;
    gt.GetTranslationValues(x, y, z);
    x = __max(-1, __min(1, x));
    y = __max(-2, __min(2, y));
    z = __max(-3, __min(3, z));
    gt.SetTranslationFromValues(x, y, z);
  }

  // set output.
  KinematicState kineOut(ctxt.GetOutputTarget());
  kineOut.PutTransform(gt);

  // done.
  return CStatus::OK;
}

