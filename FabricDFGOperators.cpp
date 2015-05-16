#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>

#include <xsi_command.h>
#include <xsi_argument.h>
#include <xsi_factory.h>
#include <xsi_longarray.h>
#include <xsi_doublearray.h>
#include <xsi_math.h>
#include <xsi_customproperty.h>
#include <xsi_ppglayout.h>
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

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Define(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  CustomOperator oCustomOperator;

  Parameter oParam;
  CRef oPDef;

  Factory oFactory = Application().GetFactory();
  oCustomOperator = ctxt.GetSource();

  oPDef = oFactory.CreateParamDef(L"evalID", CValue::siInt4, siReadOnly, L"evalID", L"evalID", 0, 0, 1000, 0, 1000);
  oCustomOperator.AddParameter(oPDef,oParam);
  oPDef = oFactory.CreateParamDef(L"persistenceData", CValue::siString, siReadOnly, L"persistenceData", L"persistenceData", "", "", "", "", "");
  oCustomOperator.AddParameter(oPDef,oParam);
  oPDef = oFactory.CreateParamDef(L"alwaysConvertMeshes", CValue::siBool, siPersistable, L"alwaysConvertMeshes", L"alwaysConvertMeshes", false, CValue(), CValue(), CValue(), CValue());
  oCustomOperator.AddParameter(oPDef,oParam);

  oCustomOperator.PutAlwaysEvaluate(false);
  oCustomOperator.PutDebug(0);

  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_DefineLayout(CRef &in_ctxt)
{
  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Init(CRef &in_ctxt)
{
  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Term(CRef &in_ctxt)
{
  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Update(CRef &in_ctxt)
{
  // init.
  OperatorContext ctxt(in_ctxt);
  CustomOperator  op(ctxt.GetSource());
  if (!op.IsValid())
  { Application().LogMessage(L"failed to get custom operator from context", siErrorMsg);
    return CStatus::OK; }

  // get the output port that is currently being evaluated.
  OutputPort outputPort(ctxt.GetOutputPort());
  Application().LogMessage(L"evaluating output port \"" + outputPort.GetName() + L"\"");

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

