#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_menu.h>
#include <xsi_model.h>
#include <xsi_customoperator.h>
#include <xsi_x3dobject.h>
#include <xsi_uitoolkit.h>
#include <xsi_null.h>
#include <xsi_vector3.h>
#include <xsi_selection.h>
#include <xsi_string.h>

#include "FabricDFGTools.h"

using namespace XSI;

XSIPLUGINCALLBACK CStatus FabricDFG_Init( CRef &in_ctxt )
{
  Menu menu = Context(in_ctxt).GetSource();
  MenuItem item;

  menu.AddCallbackItem("Create DFG Operator",                 "FabricDFG_Menu_CreateDFGOp",           item);
  menu.AddSeparatorItem();
  menu.AddCallbackItem("Create Null with DFG Operator",       "FabricDFG_Menu_CreateNullWithOp",      item);
  menu.AddCallbackItem("Create Polymesh with DFG Operator",   "FabricDFG_Menu_CreatePolymeshWithOp",  item);
  menu.AddSeparatorItem();
  menu.AddCallbackItem("Import JSON",                         "FabricDFG_Menu_ImportJSON",            item);
  menu.AddCallbackItem("Export JSON",                         "FabricDFG_Menu_ExportJSON",            item);
  menu.AddSeparatorItem();
  menu.AddCallbackItem("Online Help",                         "FabricDFG_Menu_OnlineHelp",            item);
  menu.AddCallbackItem("Log Status",                          "FabricDFG_Menu_LogStatus",             item);

  return CStatus::OK;
}

SICALLBACK FabricDFG_Menu_CreateDFGOp(XSI::CRef&)
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
        if (Application().ExecuteCommand(L"PickElement", args, CValue()) == CStatus::Fail)
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

  // execute command for all elements in targetObjects.
  for (int i=0;i<targetObjects.GetCount();i++)
    dfgTool_ExecuteCommand(L"dfgSoftimageOpApply", targetObjects[i], L"", true);

  // done.
  return CStatus::OK;
}

SICALLBACK FabricDFG_Menu_CreateNullWithOp(XSI::CRef&)
{
  // create a null.
  Null obj;
  if (Application().GetActiveSceneRoot().AddNull(L"null", obj) != CStatus::OK)
  { Application().LogMessage(L"failed to create a null", siErrorMsg);
  return CStatus::OK; }

  // select it.
  Selection sel = Application().GetSelection();
  sel.Clear();
  sel.Add(obj.GetRef());

  // execute command.
  dfgTool_ExecuteCommand(L"dfgSoftimageOpApply", obj.GetFullName(), L"", true);

  // done.
  return CStatus::OK;
}

SICALLBACK FabricDFG_Menu_CreatePolymeshWithOp(XSI::CRef&)
{
  // create an empty polygon mesh.
  X3DObject obj;
  if (Application().GetActiveSceneRoot().AddPolygonMesh(MATH::CVector3Array(), CLongArray(), L"polymsh", obj) != CStatus::OK)
  { Application().LogMessage(L"failed to create an empty polygon mesh", siErrorMsg);
  return CStatus::OK; }

  // select it.
  Selection sel = Application().GetSelection();
  sel.Clear();
  sel.Add(obj.GetRef());

  // execute command.
  dfgTool_ExecuteCommand(L"dfgSoftimageOpApply", obj.GetFullName(), L"", true);

  // done.
  return CStatus::OK;
}

SICALLBACK FabricDFG_Menu_OnlineHelp(XSI::CRef&)
{
  dfgTool_ExecuteCommand(L"OpenNetView",L"http://docs.fabric-engine.com/FabricEngine/latest/HTML/" ,true ,(LONG)1);
  return CStatus::OK;
}

SICALLBACK FabricDFG_Menu_ImportJSON(XSI::CRef&)
{
  LONG ret;
  UIToolkit toolkit = Application().GetUIToolkit();
  CString msgBoxtitle = L"Import JSON";

  // get/check current selection.
  CRef selRef;
  {
    Selection sel = Application().GetSelection();
    if (sel.GetCount() < 1)
    { toolkit.MsgBox(L"Nothing selected.\nPlease select an object or operator and try again.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
    if (sel.GetCount() > 1)
    { toolkit.MsgBox(L"More than one object selected.\nPlease select a single object or operator and try again.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
    selRef = sel.GetArray()[0];
  }

  // get operator from selection.
  CustomOperator op;
  if (selRef.GetClassID() == siCustomOperatorID)
  {
    op.SetObject(selRef);
    if (!op.IsValid())
    { toolkit.MsgBox(L"The selection cannot be used for this operation.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
  }
  else
  {
    X3DObject obj(selRef);
    if (!obj.IsValid())
    { toolkit.MsgBox(L"The selection cannot be used for this operation.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
    CRefArray opRefs;
    int numOps = dfgTool_GetRefsAtOps(obj, opRefs);
    if (numOps < 1)
    { toolkit.MsgBox(L"No valid custom operator found to perform this operation.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
    if (numOps > 1)
    { toolkit.MsgBox(L"The selection contains more than one custom operator.\nPlease select a single operator and try again.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
    op.SetObject(opRefs[0]);
    if (!op.IsValid())
    { toolkit.MsgBox(L"Failed to get custom operator.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
  }

  // open file browser.
  CString fileName;
  if (!dfgTool_FileBrowserJSON(false, fileName))
  { Application().LogMessage(L"aborted by user.", siWarningMsg);
    return CStatus::OK; }

  // execute command.
  dfgTool_ExecuteCommand(L"dfgImportJSON", op.GetUniqueName(), fileName);

  // done.
  return CStatus::OK;
}

SICALLBACK FabricDFG_Menu_ExportJSON(XSI::CRef&)
{
  LONG ret;
  UIToolkit toolkit = Application().GetUIToolkit();
  CString msgBoxtitle = L"Export JSON";

  // get/check current selection.
  CRef selRef;
  {
    Selection sel = Application().GetSelection();
    if (sel.GetCount() < 1)
    { toolkit.MsgBox(L"Nothing selected.\nPlease select an object or operator and try again.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
    if (sel.GetCount() > 1)
    { toolkit.MsgBox(L"More than one object selected.\nPlease select a single object or operator and try again.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
    selRef = sel.GetArray()[0];
  }

  // get operator from selection.
  CustomOperator op;
  if (selRef.GetClassID() == siCustomOperatorID)
  {
    op.SetObject(selRef);
    if (!op.IsValid())
    { toolkit.MsgBox(L"The selection cannot be used for this operation.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
  }
  else
  {
    X3DObject obj(selRef);
    if (!obj.IsValid())
    { toolkit.MsgBox(L"The selection cannot be used for this operation.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
    CRefArray opRefs;
    int numOps = dfgTool_GetRefsAtOps(obj, opRefs);
    if (numOps < 1)
    { toolkit.MsgBox(L"No valid custom operator found to perform this operation.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
    if (numOps > 1)
    { toolkit.MsgBox(L"The selection contains more than one custom operator.\nPlease select a single operator and try again.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
    op.SetObject(opRefs[0]);
    if (!op.IsValid())
    { toolkit.MsgBox(L"Failed to get custom operator.", siMsgOkOnly | siMsgInformation, msgBoxtitle, ret);
      return CStatus::OK; }
  }

  // open file browser.
  CString fileName;
  if (!dfgTool_FileBrowserJSON(true, fileName))
  { Application().LogMessage(L"aborted by user.", siWarningMsg);
    return CStatus::OK; }

  // execute command.
  dfgTool_ExecuteCommand(L"dfgExportJSON", op.GetUniqueName(), fileName);

  // done.
  return CStatus::OK;
}

SICALLBACK FabricDFG_Menu_LogStatus(XSI::CRef&)
{
  dfgTool_ExecuteCommand(L"dfgLogStatus");
  return CStatus::OK;
}


