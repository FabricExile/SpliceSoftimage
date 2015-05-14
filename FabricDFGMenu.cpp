#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_menu.h>

using namespace XSI;

XSIPLUGINCALLBACK CStatus FabricDFG_Init( CRef& in_ctxt )
{
  Menu menu = Context(in_ctxt).GetSource();
  MenuItem item;

  menu.AddCallbackItem("Online Help", "FabricDFG_Menu_OnlineHelp", item);
  menu.AddCallbackItem("Log Version", "FabricDFG_Menu_LogVersion", item);

  return CStatus::OK;
}

SICALLBACK FabricDFG_Menu_OnlineHelp( XSI::CRef& )
{
  CValueArray args;
  
  args.Add(L"http://docs.fabric-engine.com/FabricEngine/latest/HTML/");
  args.Add(true);
  args.Add((LONG)1);
  Application().ExecuteCommand(L"OpenNetView", args, CValue());
  return CStatus::OK;
}

SICALLBACK FabricDFG_Menu_LogVersion( XSI::CRef& )
{
  Application().ExecuteCommand(L"dfgLogVersion", CValueArray(), CValue());
  return CStatus::OK;
}

