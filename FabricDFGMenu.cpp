#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_menu.h>
#include <xsi_selection.h>
#include <xsi_string.h>

using namespace XSI;

XSIPLUGINCALLBACK CStatus FabricDFG_Init( CRef& in_ctxt )
{
  Menu menu = Context(in_ctxt).GetSource();
  MenuItem item;

  menu.AddCallbackItem("Create DFG Operator", "FabricDFG_Menu_dfgSoftimageOpApply", item);
  menu.AddSeparatorItem();
  menu.AddCallbackItem("Online Help",         "FabricDFG_Menu_OnlineHelp",          item);
  menu.AddCallbackItem("Log Status",          "FabricDFG_Menu_LogStatus",           item);

  return CStatus::OK;
}

SICALLBACK FabricDFG_Menu_dfgSoftimageOpApply(XSI::CRef&)
{
  // init and fill array of target objects.
  CStringArray targetObjects;
  {
    Selection sel = Application().GetSelection();
    if (sel.GetCount() <= 0)
    {
      // nothing is selected
      // => fill targetObjects from picking session.
      CValueArray args(7);
      args[0] = siGenericObjectFilter;
      args[1] = L"Pick Object";
      args[2] = L"Pick Object";
      args[5] = 0;
      while (true)
      {
        // pick session failed?
        CValue ret;
        if (Application().ExecuteCommand(L"PickElement", args, ret) == CStatus::Fail)
          break;

        // right button?
        if ((LONG)args[4] == 0)
          break;

        // add CRef of picked object to targetObjects.
        CRef ref = args[3];
        targetObjects.Add(ref.GetAsText());
      }
    }
    else
    {
      // there are selected objects
      // => fill targetObjects from selection.
      for (int i=0;i<sel.GetCount();i++)
        targetObjects.Add(sel[i].GetAsText());
    }
  }

  // call the command dfgSoftimageOpApply for all elements in targetObjects.
  for (int i=0;i<targetObjects.GetCount();i++)
  {
    CValue ret;
    CValueArray args;
    args.Add(targetObjects[i]);
    Application().ExecuteCommand(L"dfgSoftimageOpApply", args, ret);
  }

  // done.
  return CStatus::OK;
}

SICALLBACK FabricDFG_Menu_OnlineHelp(XSI::CRef&)
{
  CValueArray args;
  
  args.Add(L"http://docs.fabric-engine.com/FabricEngine/latest/HTML/");
  args.Add(true);
  args.Add((LONG)1);
  Application().ExecuteCommand(L"OpenNetView", args, CValue());
  return CStatus::OK;
}

SICALLBACK FabricDFG_Menu_LogStatus(XSI::CRef&)
{
  Application().ExecuteCommand(L"dfgLogStatus", CValueArray(), CValue());
  return CStatus::OK;
}

