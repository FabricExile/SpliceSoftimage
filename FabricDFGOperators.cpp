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
#include <xsi_utils.h>
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

XSIPLUGINCALLBACK CStatus dfgSoftimageOp_Update(CRef &in_ctxt)
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
