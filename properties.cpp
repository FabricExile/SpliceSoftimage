
#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>

#include <xsi_command.h>
#include <xsi_factory.h>
#include <xsi_math.h>
#include <xsi_customproperty.h>
#include <xsi_ppglayout.h>
#include <xsi_parameter.h>

using namespace XSI;

SICALLBACK SpliceInfo_Define( CRef& in_ctxt )
{
  Context ctxt( in_ctxt );
  CustomProperty prop = ctxt.GetSource();
  Parameter param;

  prop.AddParameter(L"target", CValue::siString, siReadOnly, "", "", 0, param);

  return CStatus::OK;
}

SICALLBACK SpliceInfo_DefineLayout( CRef& in_ctxt )
{
  Context ctxt( in_ctxt );
  PPGLayout layout = ctxt.GetSource();
  PPGItem item;
  layout.Clear();
  
  item = layout.AddItem("target", "Operator Target");

  return CStatus::OK;
}
