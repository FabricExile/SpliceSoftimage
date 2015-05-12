
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
#include "plugin.h"
#include "operators.h"
#include "FabricSpliceBaseInterface.h"

using namespace XSI;

opUserData::opUserData(unsigned int objectID)
{
  _objectID = objectID;
  _interf = NULL;
}

FabricSpliceBaseInterface * opUserData::getInterf()
{
  if(_interf == NULL)
    _interf = FabricSpliceBaseInterface::getInstanceByObjectID(_objectID);
  return _interf;
}

unsigned int opUserData::getObjectID()
{
  return _objectID;
}

XSIPLUGINCALLBACK CStatus SpliceOp_Define(CRef & in_ctxt)
{
  Context ctxt( in_ctxt );
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
  oPDef = oFactory.CreateParamDef(L"editLayout", CValue::siBool, siPersistable, L"editLayout", L"editLayout", false, CValue(), CValue(), CValue(), CValue());
  oCustomOperator.AddParameter(oPDef,oParam);
  oPDef = oFactory.CreateParamDef(L"layout", CValue::siString, siReadOnly, L"layout", L"layout", "", "", "", "", "");
  oCustomOperator.AddParameter(oPDef,oParam);
  FabricSpliceBaseInterface::constructXSIParameters(oCustomOperator, oFactory);

  oCustomOperator.PutAlwaysEvaluate(false);
  oCustomOperator.PutDebug(0);

  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus SpliceOp_DefineLayout(CRef & in_ctxt)
{
  Context ctxt( in_ctxt );
  PPGLayout oLayout;
  PPGItem oItem;
  oLayout = ctxt.GetSource();
  if()
  oLayout.Clear();
  oLayout.AddTab("main");
  oLayout.AddItem("editLayout");
  oLayout.AddTab("layout");
  oLayout.AddItem("editLayout");
  oLayout.AddItem("layout");
  // layout can't be driven for now.
  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus SpliceOp_Update(CRef & in_ctxt)
{
  FabricSplice::Logging::AutoTimer("SpliceOp_Update");

  OperatorContext ctxt( in_ctxt );
  CValue udVal = ctxt.GetUserData();
  opUserData * p = (opUserData*)(CValue::siPtrType)udVal;
  if(p == NULL)
  {
    xsiLogErrorFunc("Missing SpliceOp UserData. Unexpected failure.");
    return CStatus::OK;
  }

  XSISPLICE_CATCH_BEGIN()

  FabricSpliceBaseInterface * interf = p->getInterf();
  CRef opRef = Application().GetObjectFromID(p->getObjectID());
  if(interf != NULL)
  {
    // When transfering the input values, we check for changes and only evaluate if
    // one of the inputs has actually changed. The Softimage application will evaluate
    // and operator multiple times if connected to multiple outputs(once for each connected outport).
    // This requires that we manage the clean/dirty state of the operator else for complex operators
    // driving many values in Softimage, the whole system slows to a crawl.
    if(interf->transferInputPorts(opRef, ctxt))
    {
      interf->evaluate();
    }
    interf->transferOutputPort(ctxt);
  }
  else if(!xsiIsLoadingScene())
  {
    CRef opRef = Application().GetObjectFromID(p->getObjectID());
    xsiLogErrorFunc("Missing FabricSpliceBaseInterface. Probably duplicated object. Please remove operator '"+opRef.GetAsText()+"'.");
    return CStatus::OK;
  }

  XSISPLICE_CATCH_END_CSTATUS()

  return CStatus::OK;
}

std::vector<opUserData*> gOperatorInstances;
XSIPLUGINCALLBACK CStatus SpliceOp_Init(CRef & in_ctxt)
{
  Context ctxt( in_ctxt );
  CustomOperator op(ctxt.GetSource());

  XSISPLICE_CATCH_BEGIN()

  CValue udVal = ctxt.GetUserData();
  opUserData * p = (opUserData*)(CValue::siPtrType)udVal;
  if(p != NULL){
   return CStatus::OK;
  }

  p = new opUserData(op.GetObjectID());
  CValue val = (CValue::siPtrType)p;
  ctxt.PutUserData( val ) ;

  gOperatorInstances.push_back(p);

  XSISPLICE_CATCH_END_CSTATUS()

  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus SpliceOp_Term(CRef & in_ctxt)
{
  Context ctxt( in_ctxt );

  XSISPLICE_CATCH_BEGIN()

  CValue udVal = ctxt.GetUserData();
  opUserData * p = (opUserData*)(CValue::siPtrType)udVal;
  if(p != NULL)
  {
    FabricSpliceBaseInterface * interf = p->getInterf();
    if(interf)
    {
      if(interf->needsDeletion())
        delete(interf);
    }
    for(size_t i=0;i<gOperatorInstances.size();i++)
    {
      if(gOperatorInstances[i] == p)
      {
        std::vector<opUserData*>::iterator it = gOperatorInstances.begin();
        it += i;
        gOperatorInstances.erase(it);
        break;
      }
    }
    delete(p);
	 CValue val = (CValue::siPtrType)NULL;
	 ctxt.PutUserData( val ) ;
  }

  XSISPLICE_CATCH_END_CSTATUS()

  return CStatus::OK;
}

std::vector<opUserData*> getXSIOperatorInstances()
{
  return gOperatorInstances;
}
