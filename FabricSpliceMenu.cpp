#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_pluginregistrar.h>
#include <xsi_plugin.h>
#include <xsi_menu.h>
#include <xsi_menuitem.h>
#include <xsi_desktop.h>
#include <xsi_layout.h>
#include <xsi_view.h>
#include <xsi_comapihandler.h>
#include <xsi_project.h>
#include <xsi_uitoolkit.h>

#include "FabricSplicePlugin.h"
#include "FabricSpliceRenderPass.h"
#include "FabricSpliceDialogs.h"
#include "FabricSpliceBaseInterface.h"

using namespace XSI;

SICALLBACK FabricSplice_Menu_LoadSplice( XSI::CRef& )
{
  xsiInitializeSplice();

  CComAPIHandler toolkit;
  toolkit.CreateInstance(L"XSI.UIToolkit");
  CComAPIHandler filebrowser(toolkit.GetProperty(L"FileBrowser"));
  filebrowser.PutProperty(L"InitialDirectory", Application().GetActiveProject().GetPath());
  filebrowser.PutProperty(L"Filter",L"Splice Files(*.splice)|*.splice||");
  CValue returnVal;
  filebrowser.Call(L"ShowOpen",returnVal);
  CString uiFileName = filebrowser.GetProperty(L"FilePathName").GetAsText();
  if(uiFileName.IsEmpty())
  {
    Application().LogMessage(L"aborted by user.", siWarningMsg);
    return CStatus::InvalidArgument;
  }
  CString fileName;
  for(ULONG i=0;i<uiFileName.Length();i++)
  {
    if(uiFileName.GetSubString(i,1).IsEqualNoCase(L"\\"))
      fileName += "/";
    else
      fileName += uiFileName[i];
  }

  CValueArray args(2);
  args[0] = L"loadSplice";
  args[1] = L"{\"fileName\":\""+fileName+"\", \"skipPicking\":true}";
  Application().ExecuteCommand(L"fabricSplice", args, returnVal);
  return CStatus::OK;
}

SICALLBACK FabricSplice_Menu_Editor( XSI::CRef& )
{
  xsiInitializeSplice();
  CustomProperty editor = editorPropGetEnsureExists(true);
  editorSetObjectIDFromSelection();
  
  CValue returnVal;
  CValueArray args(5);
  args[0] = editor.GetFullName();
  args[1] = L"";
  args[2] = L"Splice Editor";
  args[3] = siFollow;
  args[4] = false;
  Application().ExecuteCommand(L"InspectObj", args, returnVal);

  // // find the view and set it's size
  // CRefArray views = Application().GetDesktop().GetActiveLayout().GetViews();
  // for(LONG i=0;i<views.GetCount();i++)
  // {
  //   View view(views[i]);
  //   if(!view.GetType().IsEqualNoCase(L"Property Editor"))
  //     continue;
  //   Application().LogMessage(L"caption: "+view.GetAttributeValue(L"caption").GetAsText());
  //   Application().LogMessage(L"Caption: "+view.GetAttributeValue(L"Caption").GetAsText());
  //   Application().LogMessage(L"WindowTitle: "+view.GetAttributeValue(L"WindowTitle").GetAsText());
  //   view.Resize(800, 600);
  // }
  return true;
}

SICALLBACK FabricSplice_Menu_Renderer( XSI::CRef& )
{
  enableRTRPass(!isRTRPassEnabled());
  CValue returnVal;
  CValueArray args(0);
  Application().ExecuteCommand(L"Refresh", args, returnVal);
  return true;
}

SICALLBACK FabricSplice_Menu_Manipulation( XSI::CRef& )
{
  CValue returnVal;
  CValueArray args(0);
  Application().ExecuteScriptCode(L"Application.fabricSpliceTool()", L"Python");
  return true;
}

SICALLBACK FabricSplice_Menu_ThirdPartyLicenses( XSI::CRef& )
{
  CValue returnVal;
  CValueArray args(3);
  args[0] = "http://docs.fabric-engine.com/FabricEngine/latest/HTML/Splice/thirdpartylicenses.html";
  args[1] = true;
  args[2] = (LONG)1l;
  Application().ExecuteCommand(L"OpenNetView", args, returnVal);
  return true;
}

