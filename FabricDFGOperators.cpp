#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>

#include <xsi_command.h>
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

using namespace XSI;

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Init(CRef &in_ctxt)
{
  // beta log.
  CString functionName = L"dfgSoftimageOp_Init()";
  if (FabricDFGPlugin_BETA)
    Application().LogMessage(functionName + L" called", siInfoMsg);

  // init.
  Context ctxt(in_ctxt);

  // create user data.
	_userData *pud = new _userData;
	_userData  &ud = *pud;
	ctxt.PutUserData((ULLONG)pud);

  // create base interface.
  ud.baseInterface = new BaseInterface(feLog, feLogError);

  // done.
  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Term(CRef &in_ctxt)
{
  // beta log.
  CString functionName = L"dfgSoftimageOp_Term()";
  if (FabricDFGPlugin_BETA)
    Application().LogMessage(functionName + L" called", siInfoMsg);

  // init.
  Context ctxt(in_ctxt);
	_userData *pud = (_userData *)((ULLONG)ctxt.GetUserData());

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
  if (FabricDFGPlugin_BETA)
    Application().LogMessage(functionName + L" called", siInfoMsg);

  // init.
  Context ctxt(in_ctxt);
  Factory oFactory = Application().GetFactory();
  CustomOperator op = ctxt.GetSource();
  CRef oPDef;

  // create default parameters.
  oPDef = oFactory.CreateParamDef(L"FabricActive",        CValue::siBool,   siPersistable | siAnimatable | siKeyable, L"", L"", true, CValue(), CValue(), CValue(), CValue());
  op.AddParameter(oPDef, Parameter());
  oPDef = oFactory.CreateParamDef(L"evalID",              CValue::siInt4,   siReadOnly,                               L"", L"", 0, 0, 1000, 0, 1000);
  op.AddParameter(oPDef, Parameter());
  oPDef = oFactory.CreateParamDef(L"persistenceData",     CValue::siString, siReadOnly,                               L"", L"", "", "", "", "", "");
  op.AddParameter(oPDef, Parameter());
  oPDef = oFactory.CreateParamDef(L"alwaysConvertMeshes", CValue::siBool,   siPersistable,                            L"", L"", false, CValue(), CValue(), CValue(), CValue());
  op.AddParameter(oPDef, Parameter());

  // set default values.
  op.PutAlwaysEvaluate(false);
  op.PutDebug(0);

  // done.
  return CStatus::OK;
}

void dfgSoftimageOp_DefineLayout(PPGLayout &oLayout, CustomOperator &op)
{
  // clear layout.
  oLayout.Clear();
  oLayout.PutAttribute(siUIDictionary, L"None");

  // Tab "Main"
  {
    oLayout.AddTab(L"Main");
      oLayout.AddItem(L"FabricActive", L"Execute Fabric Data Flow Graph");
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

  // Tab "Tools and Ports"
  {
    oLayout.AddTab(L"Tools and Ports");
      oLayout.AddGroup(L"Import/Export");
        oLayout.AddButton(L"ImportJSON", L"Import JSON");
        oLayout.AddButton(L"ExportJSON", L"Export JSON");
      oLayout.EndGroup();
  }

  // Tab "Advanced"
  {
    oLayout.AddTab(L"Advanced");
      oLayout.AddItem(L"evalID",              L"evalID");
      oLayout.AddItem(L"persistenceData",     L"persistenceData");
      oLayout.AddItem(L"alwaysConvertMeshes", L"Always Convert Meshes");
      oLayout.AddItem(L"mute",                L"Mute");
      oLayout.AddItem(L"alwaysevaluate",      L"Always Evaluate");
      oLayout.AddItem(L"debug",               L"Debug");
      oLayout.AddItem(L"Name",                L"Name");
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
  PPGEventContext::PPGEvent eventID = ctxt.GetEventID();
	UIToolkit toolkit = Application().GetUIToolkit();

  // process event.
  if (eventID == PPGEventContext::siOnInit)
  {
    CustomOperator op = ctxt.GetSource();
    dfgSoftimageOp_DefineLayout(op.GetPPGLayout(), op);
    ctxt.PutAttribute(L"Refresh", true);
  }
  else if (eventID == PPGEventContext::siButtonClicked)
  {
    CString btnName = ctxt.GetAttribute(L"Button").GetAsText();
    if (btnName.IsEmpty())
    {
      // nop.
    }
    else if (btnName == L"ImportJSON")
    {
      LONG ret;
      toolkit.MsgBox(L"not yet implemented", siMsgOkOnly | siMsgExclamation, "dfgSoftimageOp", ret);
    }
    else if (btnName == L"ExportJSON")
    {
      LONG ret;
      toolkit.MsgBox(L"not yet implemented", siMsgOkOnly | siMsgExclamation, "dfgSoftimageOp", ret);
    }
  }
  else if (eventID == PPGEventContext::siTabChange)
  {
  }
  else if (eventID == PPGEventContext::siParameterChange)
  {
  }

  // done.
  return CStatus::OK ;
}

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Update(CRef &in_ctxt)
{
  // beta log.
  CString functionName = L"dfgSoftimageOp_Update()";
  if (FabricDFGPlugin_BETA)
    Application().LogMessage(functionName + L" called", siInfoMsg);

  // init.
  OperatorContext ctxt(in_ctxt);
  CustomOperator op(ctxt.GetSource());
  if (!op.IsValid())
  { Application().LogMessage(functionName + L": failed to get custom operator from context", siErrorMsg);
    return CStatus::OK; }
	_userData *pud = (_userData *)((ULLONG)ctxt.GetUserData());
	_userData  &ud = *pud;
  if (!pud) { Application().LogMessage(functionName + L": no user data found!", siErrorMsg);
              return CStatus::OK; }

  // get the output port that is currently being evaluated.
  OutputPort outputPort(ctxt.GetOutputPort());
  if (FabricDFGPlugin_BETA)
    Application().LogMessage(functionName + L": evaluating output port \"" + outputPort.GetName() + L"\"");

  // get default input.
  KinematicState kineIn((CRef)ctxt.GetInputValue(L"internalMatrixIn"));
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

