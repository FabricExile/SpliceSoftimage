
#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>

#include <xsi_command.h>
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
#include <xsi_ppgeventcontext.h>
#include <xsi_parameter.h>
#include <xsi_griddata.h>
#include <xsi_gridwidget.h>
#include <xsi_x3dobject.h>
#include <xsi_customoperator.h>
#include <xsi_primitive.h>
#include <xsi_uitoolkit.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_comapihandler.h>
#include <xsi_project.h>
#include <xsi_null.h>
#include <xsi_model.h>
#include <xsi_meshbuilder.h>

// project includes
#include "plugin.h"
#include "FabricSpliceDialogs.h"
#include "FabricSpliceRenderPass.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceConversion.h"
#include <FabricSplice.h>

using namespace XSI;

const LONG gButtonHeight = 25;
const LONG gButtonWidth = 120;
#define DIALOGOPSPLITTER " - "
CString gLastSourceCode;

SICALLBACK SpliceEditor_Define( CRef& in_ctxt )
{
  Context ctxt( in_ctxt );
  CustomProperty prop = ctxt.GetSource();
  Parameter param;

  prop.AddParameter(L"logoBitmap", CValue::siInt4, siReadOnly, "", "", 0, param);
  prop.AddParameter(L"objectID", CValue::siInt4, siReadOnly, "", "", 0, param);
  prop.AddParameter(L"operatorName", CValue::siString, siReadOnly, "", "", 0, param);
  prop.AddGridParameter(L"ports");
  prop.AddGridParameter(L"operators");
  prop.AddParameter(L"availablePorts", CValue::siString, 0, "", "", "", param);
  prop.AddParameter(L"klCode", CValue::siString, 0, "", "", "", param);
  prop.AddParameter(L"klErrors", CValue::siString, siReadOnly, "", "", "", param);
  return CStatus::OK;
}

SICALLBACK SpliceEditor_DefineLayout( CRef& in_ctxt )
{
  Context ctxt( in_ctxt );
  PPGLayout layout = ctxt.GetSource();
  PPGItem item;
  layout.Clear();

  CString logoPath = xsiGetWorkgroupPath()+CUtils::Slash()+"Application"+CUtils::Slash()+"UI"+CUtils::Slash()+"FE_logo.bmp";
  
  layout.AddTab("Connections");
  item = layout.AddItem("logoBitmap", "SpliceLogo", siControlBitmap);
  item.PutAttribute(siUIFilePath, logoPath);
  item.PutAttribute(siUINoLabel, true);

  layout.AddRow();
  item = layout.AddButton("createIOSpliceOp", "New Splice (IO)");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  item = layout.AddButton("createOutSpliceOp", "New Splice (OUT)");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  item = layout.AddButton("saveSpliceOp", "Save Splice Preset");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  // item = layout.AddButton("loadSpliceOp", "Load Splice");
  // item.PutAttribute(siUICX, gButtonWidth);
  // item.PutAttribute(siUICY, gButtonHeight);
  layout.EndRow();
  layout.AddRow();
  item = layout.AddButton("inspectSpliceOp", "Inspect");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  item = layout.AddButton("selectSpliceOp", "Select");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  item = layout.AddButton("updateSpliceOpFromSelection", "Update UI");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  layout.EndRow();

  layout.AddGroup("Ports");
  item = layout.AddItem( "ports", "Ports", siControlGrid);
  item.PutAttribute(siUIGridHideRowHeader, true);
  item.PutAttribute(siUIGridLockColumnHeader, true);
  item.PutAttribute(siUINoLabel, true);
  item.PutAttribute(siUIWidthPercentage, 100);
  item.PutAttribute(siUIGridColumnWidths, "0:150:80:40:200");
  item.PutAttribute(siUIGridReadOnlyColumns, "1:1:1:1:1");
  layout.AddRow();
  item = layout.AddButton("addParam", "Add Parameter");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  item = layout.AddButton("addPort", "Add XSI Port");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  item = layout.AddButton("addInternalPort", "Add Internal Port");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  layout.EndRow();
  layout.AddRow();
  item = layout.AddButton("addICEPort", "Add ICE Port");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  item = layout.AddButton("removePort", "Remove Item");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  item = layout.AddButton("reroutePort", "Reroute Port");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  layout.EndRow();
  layout.EndGroup();

  layout.AddGroup("Operators");
  item = layout.AddItem( "operators", "Operators", siControlGrid);
  item.PutAttribute(siUIGridHideRowHeader, true);
  item.PutAttribute(siUIGridLockColumnHeader, true);
  item.PutAttribute(siUINoLabel, true);
  item.PutAttribute(siUIWidthPercentage, 100);
  item.PutAttribute(siUIGridColumnWidths, "0:150");
  item.PutAttribute(siUIGridReadOnlyColumns, "1");
  layout.AddRow();
  item = layout.AddButton("addOperator", "Add new Operator");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  item = layout.AddButton("removeOperator", "Remove Operator");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  item = layout.AddButton("editOperator", "Edit Operator");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  layout.EndRow();
  layout.EndGroup();

  layout.AddTab("KL");
  item = layout.AddItem("logoBitmap", "SpliceLogo", siControlBitmap);
  item.PutAttribute(siUIFilePath, logoPath);
  item.PutAttribute(siUINoLabel, true);
  layout.AddRow();
  item = layout.AddButton("compileOperator", "Compile KL");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  layout.EndRow();

  item = layout.AddItem( "availablePorts", "Available Ports", siControlTextEditor);
  item.PutAttribute(siUIFont, "Courier New");
  item.PutAttribute(siUIToolbar, false);
  item.PutAttribute(siUIFontSize, 10);
  item.PutAttribute(siUIHeight, 80);
  item.PutAttribute(siUIBackgroundColor, 0xf8f8f2);
  item.PutAttribute(siUIForegroundColor, 0x272822);
  item.PutAttribute(siUIHorizontalScroll, true);
  item.PutAttribute(siUIVerticalScroll, true);
  item.PutAttribute(siUILineWrap, false);

  layout.AddRow();
  item = layout.AddItem( "operatorName", "Operator Name");
  item.PutAttribute(siUINoLabel, true);
  layout.EndRow();

  item = layout.AddItem( "klCode", "Source Code", siControlTextEditor);
  item.PutAttribute(siUINoLabel, true);
  item.PutAttribute(siUIFont, "Courier New");
  item.PutAttribute(siUIKeywords, xsiGetKLKeyWords());
  item.PutAttribute(siUILanguage, "JScript");
  item.PutAttribute(siUIAutoComplete, siKeywords);
  item.PutAttribute(siUICommentFont, "Courier New");
  item.PutAttribute(siUICommentColor, 0x75715e);
  item.PutAttribute(siUIPreprocessorColor, 0x808080);

  // item.PutAttribute(siUICapability, siCanLoad | siCanSave);
  item.PutAttribute(siUIToolbar, false);
  item.PutAttribute(siUIFontSize, 10);
  item.PutAttribute(siUIHeight, 500);
  item.PutAttribute(siUIBackgroundColor, 0xf8f8f2);
  item.PutAttribute(siUIForegroundColor, 0x272822);
  item.PutAttribute(siUIHorizontalScroll, true);
  item.PutAttribute(siUIVerticalScroll, true);
  item.PutAttribute(siUILineNumbering, true);
  item.PutAttribute(siUILineWrap, false);
  item.PutAttribute("UseSpacesForTab", true);
  item.PutAttribute("TabSize", 2);

  item = layout.AddItem( "klErrors", "Errors", siControlTextEditor);
  item.PutAttribute(siUINoLabel, true);
  item.PutAttribute(siUIFont, "Courier New");
  item.PutAttribute(siUIBackgroundColor, 0x272822);
  item.PutAttribute(siUIForegroundColor, 0x0000ff);
  item.PutAttribute(siUIToolbar, false);
  item.PutAttribute(siUIFontSize, 10);
  item.PutAttribute(siUIHeight, 100);
  item.PutAttribute(siUIHeight, 100);
  item.PutAttribute(siUILineNumbering, false);
  item.PutAttribute(siUILineWrap, false);

  return CStatus::OK;
}

CustomProperty editorPropGet()
{
  CRef ref;
  ref.Set(L"Scene_Root.SpliceEditor");
  return CustomProperty(ref);
}

CustomProperty editorPropGetEnsureExists(bool clearErrors)
{
  CustomProperty editorProp = editorPropGet();
  if(!editorProp.IsValid())
  {
    CValueArray addpropArgs(5) ;
    addpropArgs[0] = L"SpliceEditor";
    addpropArgs[1] = L"Scene_Root";
    addpropArgs[3] = L"SpliceEditor";

    CValue retVal;
    Application().ExecuteCommand( L"SIAddProp", addpropArgs, retVal );

    editorProp = editorPropGet();
  }
  if(clearErrors)
    editorProp.PutParameterValue("klErrors", "");
  return editorProp;
}

void editorSetObjectIDFromSelection()
{
  CustomProperty editorProp = editorPropGetEnsureExists();
  Selection sel = Application().GetSelection();
  for(LONG i=0;i<sel.GetCount();i++)
  {
    CRef item = sel.GetItem(i);
    CStringArray itemStr;
    itemStr.Add(item.GetAsText());
    itemStr.Add(itemStr[0]+L".kine.local.SpliceOp");
    itemStr.Add(itemStr[0]+L".kine.global.SpliceOp");
    if(X3DObject(item).IsValid())
      itemStr.Add(X3DObject(item).GetActivePrimitive().GetFullName()+L".SpliceOp");

    for(LONG j=0;j<itemStr.GetCount();j++)
    {
      CRef target;
      target.Set(itemStr[j]);
      if(!target.IsValid())
        continue;
      CustomOperator op(target);
      if(op.IsValid() && op.GetType().IsEqualNoCase(L"SpliceOp"))
      {
        editorProp.PutParameterValue("objectID", (LONG)op.GetObjectID());
        return;
      }
    }
  }
}

void showSpliceEcitor(unsigned int objectID)
{
  CustomProperty editorProp = editorPropGetEnsureExists();
  editorProp.PutParameterValue("objectID", (LONG)objectID);
  CValue returnVal;
  CValueArray args(5);
  args[0] = editorProp.GetFullName();
  args[1] = L"";
  args[2] = L"Splice Editor";
  args[3] = siFollow;
  args[4] = false;
  Application().ExecuteCommand(L"InspectObj", args, returnVal);
}

const char * getSourceCodeForOperator(const char * graphName, const char * opName)
{
  CustomProperty editorProp = editorPropGet();
  if(editorProp.IsValid())
  {
    LONG objectID = editorProp.GetParameterValue("objectID");
    FabricSpliceBaseInterface * interf = FabricSpliceBaseInterface::getInstanceByObjectID(objectID);
    if(interf)
    {
      CString currentOpName = editorProp.GetParameterValue(L"operatorName");
      if(interf->getSpliceGraph().getName() == std::string(graphName) && currentOpName == opName)
      {
        gLastSourceCode = editorProp.GetParameterValue(L"klCode");
        if(gLastSourceCode.Length() > 0)
        {
          return gLastSourceCode.GetAsciiString();
        }
      }
    }
  }
  return NULL;
}

void updateSpliceEditorGrids(CustomProperty & prop)
{
  GridData portsGrid((CRef)prop.GetParameter("ports").GetValue());
  GridData operatorsGrid((CRef)prop.GetParameter("operators").GetValue());
  portsGrid.PutColumnCount(4);
  portsGrid.PutColumnLabel(0, "Name");
  portsGrid.PutColumnLabel(1, "Type");
  portsGrid.PutColumnLabel(2, "Mode");
  portsGrid.PutColumnLabel(3, "Target");
  portsGrid.PutColumnType(0, siColumnStandard);
  portsGrid.PutColumnType(1, siColumnStandard);
  portsGrid.PutColumnType(2, siColumnCombo);
  portsGrid.PutColumnType(3, siColumnStandard);
  CValueArray portsCombo;
  portsCombo.Add(L"IN");
  portsCombo.Add((LONG)0);
  portsCombo.Add(L"OUT");
  portsCombo.Add((LONG)1);
  portsCombo.Add(L"IO");
  portsCombo.Add((LONG)2);
  portsGrid.PutColumnComboItems(2, portsCombo);
  operatorsGrid.PutColumnCount(1);
  operatorsGrid.PutColumnLabel(0, "Name");

  LONG objectID = prop.GetParameterValue("objectID");
  FabricSpliceBaseInterface * interf = FabricSpliceBaseInterface::getInstanceByObjectID(objectID);
  if(interf)
  {
    FabricSplice::DGGraph graph = interf->getSpliceGraph();
    portsGrid.PutRowCount(graph.getDGPortCount());
    for(unsigned int i = 0; i < graph.getDGPortCount(); i++)
    {
      CString portName = graph.getDGPortName(i);
      FabricSplice::DGPort port = graph.getDGPort(portName.GetAsciiString());
      CString portType = port.getDataType();
      if(port.isArray())
        portType += L"[]";
      FabricSplice::Port_Mode portMode = port.getMode();
      CString targetsStr = interf->getXSIPortTargets(portName);

      if(port.getOption("ICEAttribute").isString())
      {
        targetsStr += ":";
        targetsStr += port.getOption("ICEAttribute").getStringData();
      }

      portsGrid.PutCell(0, i, portName);
      portsGrid.PutCell(1, i, portType);
      portsGrid.PutCell(2, i, (LONG)portMode);
      portsGrid.PutCell(3, i, targetsStr);
    }

    CStringArray operatorNames;
    for(unsigned int i=0;i<graph.getDGNodeCount();i++)
    {
      CString dgNodeName = graph.getDGNodeName(i);
      for(unsigned int j=0;j<graph.getKLOperatorCount(dgNodeName.GetAsciiString());j++)
      {
        CString operatorName = graph.getKLOperatorName(j, dgNodeName.GetAsciiString());
        operatorNames.Add(dgNodeName+DIALOGOPSPLITTER+operatorName);
      }
    }
    operatorsGrid.PutRowCount(operatorNames.GetCount());
    for(unsigned int i = 0; i < operatorNames.GetCount(); i++)
      operatorsGrid.PutCell(0, i, operatorNames[i]);

    if(operatorNames.GetCount() > 0)
    {
      CString opName = prop.GetParameterValue(L"operatorName");
      if(!interf->hasKLOperator(opName, "")) // query all DG nodes
      {
        opName = operatorNames[0].Split(DIALOGOPSPLITTER)[1];
        CString kl = interf->getKLOperatorCode(opName);
        prop.PutParameterValue(L"operatorName", opName);
        prop.PutParameterValue(L"availablePorts", interf->getParameterString());
        prop.PutParameterValue(L"klCode", kl);
      }
    }
    else 
    {
      prop.PutParameterValue(L"operatorName", L"");
      prop.PutParameterValue(L"kl", L"");
    }
  }
}

CLongArray getGridWidgetSelection(GridWidget & widget, GridData & data)
{
  CLongArray result;
  for(LONG c=0;c<data.GetColumnCount();c++)
  {
    for(LONG r=0;r<data.GetRowCount();r++)
    {
      if(widget.IsCellSelected(c, r))
      {
        result.Add(c);
        result.Add(r);
      }
    }
  }
  return result;
}

SICALLBACK SpliceEditor_PPGEvent( CRef& in_ctxt )
{
  PPGEventContext ctxt( in_ctxt );
  
  CRefArray inspected = ctxt.GetInspectedObjects();

  if(ctxt.GetEventID() == PPGEventContext::siOnInit)
  {
    CustomProperty prop = ctxt.GetSource();
    updateSpliceEditorGrids(prop);
  }
  else if(ctxt.GetEventID() == PPGEventContext::siTabChange)
  {
    CString tabName = ctxt.GetAttribute(L"Tab");
    if(tabName.IsEqualNoCase(L"Connections"))
    {
      CustomProperty prop(inspected[0]);
      updateSpliceEditorGrids(prop);
    }
    else if(tabName.IsEqualNoCase(L"KL"))
    {
    }
  }
  else if(ctxt.GetEventID() == PPGEventContext::siButtonClicked )
  {
    CustomProperty prop(inspected[0]);
    bool requiresRefresh = false;

    GridData portsGrid((CRef)prop.GetParameter("ports").GetValue());
    GridData operatorsGrid((CRef)prop.GetParameter("operators").GetValue());
    GridWidget portsWidget = portsGrid.GetGridWidget();
    GridWidget operatorsWidget = operatorsGrid.GetGridWidget();

    CString button = ctxt.GetAttribute(L"Button");
    if(button.IsEqualNoCase(L"createOutSpliceOp") || button.IsEqualNoCase(L"createIOSpliceOp"))
    {
      CRefArray items = PickObjectArray(L"Pick output object", L"Pick next output object");
      if(items.GetCount() > 0)
      {
        CString dataType = getSpliceDataTypeFromRefArray(items);
        if(!dataType.IsEmpty())
        {
          CString portMode = "out";
          if(button.IsEqualNoCase(L"createIOSpliceOp"))
            portMode = "io";
          CValue returnVal;
          CValueArray args(2);

          CString portName = "result";
          if(dataType == "PolygonMesh")
            portName = "mesh0";
          else if(dataType == "Lines")
            portName = "lines";
          else if(dataType == "Mat44")
            portName = "matrix";
          else if(dataType == "Mat44[]")
            portName = "matrices";
          else if(dataType == "Boolean")
            portName = "value";
          else if(dataType == "Integer")
            portName = "value";
          else if(dataType == "Scalar")
            portName = "value";
          else if(dataType == "String")
            portName = "value";

          args[0] = L"newSplice";
          args[1] = L"{\"targets\":\""+items.GetAsText()+"\", \"portName\":\""+portName+"\", \"portMode\":\""+portMode+"\"}";
          Application().ExecuteCommand(L"fabricSplice", args, returnVal);

          CRef opRef;
          opRef.Set(items[0].GetAsText()+L".SpliceOp");
          if(opRef.IsValid())
          {
            prop.PutParameterValue(L"objectID", (LONG)CustomOperator(opRef).GetObjectID());
            updateSpliceEditorGrids(prop);
            requiresRefresh = true;
          }
        }
      }
    }
    else if(button.IsEqualNoCase(L"loadSpliceOp"))
    {
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
      if(Application().ExecuteCommand(L"fabricSplice", args, returnVal).Succeeded())
      {
        LONG objectID = returnVal;
        prop.PutParameterValue(L"objectID", objectID);
        updateSpliceEditorGrids(prop);
        requiresRefresh = true;
      }
    }
    else
    {
      LONG objectID = prop.GetParameterValue("objectID");
      FabricSpliceBaseInterface * interf = FabricSpliceBaseInterface::getInstanceByObjectID(objectID);
      if(!interf)
      {
        LONG result;
        Application().GetUIToolkit().MsgBox(L"There's no Splice operator shown in this window. Create an operator first.", siMsgOkOnly, "Information", result);
        return CStatus::OK;
      }

      CRef objectRef = Application().GetObjectFromID(objectID);
      CString objectStr = objectRef.GetAsText();

      if(button.IsEqualNoCase(L"saveSpliceOp"))
      {
        CComAPIHandler toolkit;
        toolkit.CreateInstance(L"XSI.UIToolkit");
        CComAPIHandler filebrowser(toolkit.GetProperty(L"FileBrowser"));
        filebrowser.PutProperty(L"InitialDirectory", Application().GetActiveProject().GetPath());
        filebrowser.PutProperty(L"Filter",L"Splice Files(*.splice)|*.splice||");
        CValue returnVal;
        filebrowser.Call(L"ShowSave",returnVal);
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

        CValueArray args(3);
        args[0] = L"saveSplice";
        args[1] = objectStr;
        args[2] = L"{\"fileName\":\""+fileName+"\"}";
        Application().ExecuteCommand(L"fabricSplice", args, returnVal);
      }
      else if(button.IsEqualNoCase(L"inspectSpliceOp"))
      {
        CValue returnVal;
        CValueArray args(5);
        CRef opRef = Application().GetObjectFromID(objectID);
        args[0] = opRef.GetAsText();
        args[1] = L"";
        args[2] = L"";
        args[3] = siRecycle;
        args[4] = false;
        Application().ExecuteCommand(L"InspectObj", args, returnVal);
      }
      else if(button.IsEqualNoCase(L"selectSpliceOp"))
      {
        CValue returnVal;
        CValueArray args(1);
        CRef opRef = Application().GetObjectFromID(objectID);
        args[0] = opRef.GetAsText();
        Application().ExecuteCommand(L"SelectObj", args, returnVal);
      }
      else if(button.IsEqualNoCase(L"updateSpliceOpFromSelection"))
      {
        editorSetObjectIDFromSelection();

        LONG objectID = prop.GetParameterValue("objectID");
        interf = FabricSpliceBaseInterface::getInstanceByObjectID(objectID);
        updateSpliceEditorGrids(prop);
        if(!interf)
          return CStatus::OK;
        requiresRefresh = true;
      }
      else if(button.IsEqualNoCase(L"addParam"))
      {
        AddPortDialog dialog(0);
        if(dialog.show("Add Parameter"))
        {
          CString portName = dialog.getProp().GetParameterValue("portName");
          portName = processNameCString(portName);
          if(portName.IsEmpty())
            return CStatus::OK;

          CString dataType, extension;
          if(!dialog.getDataType(dataType, extension))
            return CStatus::Unexpected;

          CValue returnVal;
          CValueArray args(3);
          args[0] = L"addParameter";
          args[1] = objectStr;
          args[2] = L"{\"portName\":\""+portName+"\", \"dataType\":\""+dataType+"\", \"extension\":\""+extension+"\"}";
          Application().ExecuteCommand(L"fabricSplice", args, returnVal);
          updateSpliceEditorGrids(prop);
          prop.PutParameterValue(L"availablePorts", interf->getParameterString());
          requiresRefresh = true;
        }
      }
      else if(button.IsEqualNoCase(L"addICEPort"))
      {
        AddPortDialog dialog(-1);
        if(dialog.show("Add Port to ICE Attribute"))
        {
          CString portName = dialog.getProp().GetParameterValue("portName");
          portName = processNameCString(portName);
          if(portName.IsEmpty())
            return CStatus::OK;
          CString iceAttr = dialog.getProp().GetParameterValue("iceAttr");
          if(iceAttr.IsEmpty())
            return CStatus::OK;

          CString filter = L"geometry";
          CRefArray items = PickObjectArray(L"Pick object", L"", filter, 1);
          if(items.GetCount() == 1)
          {
            CValue returnVal;
            CValueArray args(3);
            args[0] = L"addInputPort";
            args[1] = objectStr;
            args[2] = L"{\"portName\":\""+portName+"\", \"iceAttribute\":\""+iceAttr+"\", \"targets\":\""+items.GetAsText()+"\"}";
            Application().ExecuteCommand(L"fabricSplice", args, returnVal);
            updateSpliceEditorGrids(prop);
            prop.PutParameterValue(L"availablePorts", interf->getParameterString());
            requiresRefresh = true;
          }
        }
      }
      else if(button.IsEqualNoCase(L"addPort"))
      {
        AddPortDialog dialog(1);
        if(dialog.show("Add Port"))
        {
          CString portName = dialog.getProp().GetParameterValue("portName");
          portName = processNameCString(portName);
          if(portName.IsEmpty())
            return CStatus::OK;

          CString dataType, extension;
          if(!dialog.getDataType(dataType, extension))
            return CStatus::Unexpected;

          CString portMode = dialog.getProp().GetParameterValue("portMode");
          bool isArray = dialog.getProp().GetParameterValue("arrayMode");
          if(isArray)
            dataType += L"[]";
          if(dataType == "KeyframeTrack[]")
            isArray = false;

          CString command = L"addInputPort";
          if(portMode == "OUT")
            command = L"addOutputPort";
          else if(portMode == "IO")
            command = L"addIOPort";

          // start a picking session
          CString filter = L"global";
          if(dataType.IsEqualNoCase(L"PolygonMesh") || dataType.IsEqualNoCase(L"PolygonMesh[]"))
            filter = L"polymsh";
          else if(dataType.IsEqualNoCase(L"Lines") || dataType.IsEqualNoCase(L"Lines[]"))
            filter = L"crvlist";
          CRefArray items = PickObjectArray(L"Pick object", L"Pick next object", filter, isArray ? 0 : 1);
          if(items.GetCount() > 0)
          {
            CString targetDataType = getSpliceDataTypeFromRefArray(items, dataType);
            if(dataType != targetDataType && dataType != targetDataType + "[]")
            {
              xsiLogErrorFunc("Data types do not match. The data type of your port is:" + dataType+ " and the picked element is of type:" + targetDataType);
              return CStatus::Unexpected;
            }
            CValue returnVal;
            CValueArray args(3);
            args[0] = command;
            args[1] = objectStr;
            args[2] = L"{\"portName\":\""+portName+"\", \"dataType\":\""+dataType+"\", \"extension\":\""+extension+"\", \"targets\":\""+items.GetAsText()+"\"}";
            Application().ExecuteCommand(L"fabricSplice", args, returnVal);
            updateSpliceEditorGrids(prop);
            prop.PutParameterValue(L"availablePorts", interf->getParameterString());
            requiresRefresh = true;
          }
        }
      }
      else if(button.IsEqualNoCase(L"addInternalPort"))
      {
        AddPortDialog dialog(2, L"IO");
        if(dialog.show("Add Internal Port"))
        {
          CString portName = dialog.getProp().GetParameterValue("portName");
          portName = processNameCString(portName);
          if(portName.IsEmpty())
            return CStatus::OK;
          CString dataType, extension;
          if(!dialog.getDataType(dataType, extension))
            return CStatus::Unexpected;
          CString portMode = dialog.getProp().GetParameterValue("portMode");
          bool isArray = dialog.getProp().GetParameterValue("arrayMode");

          if(isArray)
            dataType += L"[]";

          CValue returnVal;
          CValueArray args(3);
          args[0] = L"addInternalPort";
          args[1] = objectStr;
          args[2] = L"{\"portName\":\""+portName+"\", \"dataType\":\""+dataType+"\", \"extension\":\""+extension+"\", \"portMode\":\""+portMode+"\"}";
          Application().ExecuteCommand(L"fabricSplice", args, returnVal);
          updateSpliceEditorGrids(prop);
          prop.PutParameterValue(L"availablePorts", interf->getParameterString());
          requiresRefresh = true;
        }
      }
      else if(button.IsEqualNoCase(L"removePort"))
      {
        CLongArray gridSelection = getGridWidgetSelection(portsWidget, portsGrid);
        if(gridSelection.GetCount() == 0)
        {
          LONG result;
          Application().GetUIToolkit().MsgBox(L"You need to select a port first.", siMsgOkOnly, "Information", result);
          return CStatus::OK;
        }

        LONG result = 0;
        Application().GetUIToolkit().MsgBox(L"Are you sure?", siMsgYesNo, "Remove Port", result);
        if(result == siMsgYes)
        {
          FabricSplice::DGGraph graph = interf->getSpliceGraph();

          std::map<CString, bool> portsToRemove;
          for(LONG i=gridSelection.GetCount()-1;i>0;i-=2) {
            CString portName = graph.getDGPortName(gridSelection[i]);
            if(portsToRemove.find(portName) != portsToRemove.end())
              continue;
            portsToRemove.insert(std::pair<CString, bool>(portName, true));
          }
          for(std::map<CString,bool>::iterator it = portsToRemove.begin(); it!= portsToRemove.end(); it++)
          {
            CValue returnVal;
            CValueArray args(3);
            args[0] = L"removePort";
            args[1] = Application().GetObjectFromID((LONG)interf->getObjectID()).GetFullName();
            args[2] = L"{\"portName\":\""+it->first+"\"}";
            Application().ExecuteCommand(L"fabricSplice", args, returnVal);
          }
          updateSpliceEditorGrids(prop);
          prop.PutParameterValue(L"availablePorts", interf->getParameterString());
          requiresRefresh = true;
        }
      }
      else if(button.IsEqualNoCase(L"reroutePort"))
      {
        CLongArray gridSelection = getGridWidgetSelection(portsWidget, portsGrid);
        if(gridSelection.GetCount() == 0)
        {
          LONG result;
          Application().GetUIToolkit().MsgBox(L"You need to select a port first.", siMsgOkOnly, "Information", result);
          return CStatus::OK;
        }

        FabricSplice::DGGraph graph = interf->getSpliceGraph();
        CString portName = graph.getDGPortName(gridSelection[1]);

        FabricSplice::DGPort port = graph.getDGPort(gridSelection[1]);
        CString dataType = port.getDataType();
        bool isArray = port.isArray();
        bool isICEAttribute = port.getOption("ICEAttribute").isString();
        CString filter = L"global";
        if(dataType == L"PolygonMesh")
          filter = L"polymsh";
        if(dataType == L"Lines")
          filter = L"crvlist";
        else if(isICEAttribute)
          filter = L"geometry";

        CRefArray targetRefs = PickObjectArray(
          L"Pick new target for "+portName, L"Pick next target for "+portName, 
          filter, (!isArray || isICEAttribute) ? 1 : 0);

        // the action was canceled.
        if(targetRefs.GetCount() == 0)
          return CStatus::OK;

        CValue returnVal;
        CValueArray args(3);
        args[0] = L"reroutePort";
        args[1] = objectStr;
        args[2] = L"{\"portName\":\""+portName+"\", \"targets\":\""+targetRefs.GetAsText()+"\" }";
        Application().ExecuteCommand(L"fabricSplice", args, returnVal);
        updateSpliceEditorGrids(prop);
        requiresRefresh = true;
      }
      else if(button.IsEqualNoCase(L"addOperator"))
      {
        AddOperatorDialog dialog;
        if(dialog.show("Add Operator"))
        {
          CString opName = dialog.getProp().GetParameterValue("opName");
          opName = processNameCString(opName);

          CValue returnVal;
          CValueArray args(3);
          args[0] = L"addKLOperator";
          args[1] = objectStr;
          args[2] = L"{\"opName\":\""+opName+"\"}";
          Application().ExecuteCommand(L"fabricSplice", args, returnVal);
          updateSpliceEditorGrids(prop);
          requiresRefresh = true;
          prop.PutParameterValue(L"operatorName", opName);
          CString kl = interf->getKLOperatorCode(opName);
          prop.PutParameterValue(L"availablePorts", interf->getParameterString());
          prop.PutParameterValue(L"klCode", kl);
        }
      }
      else if(button.IsEqualNoCase(L"editOperator") || button.IsEqualNoCase(L"removeOperator"))
      {
        CLongArray gridSelection = getGridWidgetSelection(operatorsWidget, operatorsGrid);
        if(gridSelection.GetCount() == 0)
        {
          LONG result;
          Application().GetUIToolkit().MsgBox(L"You need to select an operator first.", siMsgOkOnly, "Information", result);
          return CStatus::OK;
        }

        CString opName = operatorsGrid.GetCell(gridSelection[0], gridSelection[1]);
        if(button.IsEqualNoCase(L"removeOperator"))
        {
          CString dgNodeName = opName.Split(DIALOGOPSPLITTER)[0];
          opName = opName.Split(DIALOGOPSPLITTER)[1];

          LONG result = 0;
          Application().GetUIToolkit().MsgBox(L"Are you sure?", siMsgYesNo, "Remove Operator", result);
          if(result == siMsgYes)
          {
            CValue returnVal;
            CValueArray args(3);
            args[0] = L"removeKLOperator";
            args[1] = objectStr;
            args[2] = L"{\"opName\":\""+opName+"\", \"dgNode\":\""+dgNodeName+"\"}";
            Application().ExecuteCommand(L"fabricSplice", args, returnVal);
            updateSpliceEditorGrids(prop);
            requiresRefresh = true;
          }          
        }
        else
        {
          opName = opName.Split(DIALOGOPSPLITTER)[1];
          prop.PutParameterValue(L"operatorName", opName);
          CString kl = interf->getKLOperatorCode(opName);
          prop.PutParameterValue(L"availablePorts", interf->getParameterString());
          prop.PutParameterValue(L"klCode", kl);
        }
      }
      else if(button.IsEqualNoCase(L"compileOperator"))
      {
        CString opName = prop.GetParameterValue(L"operatorName");
        if(!opName.IsEmpty())
        {
          CString kl = prop.GetParameterValue(L"klCode");
          if(!kl.IsEmpty())
          {
            prop.PutParameterValue(L"klErrors", CString());
            CValue returnVal;
            CValueArray args(4);
            args[0] = L"setKLOperatorCode";
            args[1] = objectStr;
            args[2] = L"{\"opName\":\""+opName+"\"}";
            args[3] = kl;
            Application().ExecuteCommand(L"fabricSplice", args, returnVal);
          }          
        }
      }

      inspected = ctxt.GetInspectedObjects();
      prop = CustomProperty(inspected[0]);
      if(prop.IsValid())
      {
        prop.PutParameterValue("objectID", (LONG)interf->getObjectID());
      }
      if(requiresRefresh)
        ctxt.PutAttribute(L"Refresh", true);
    }
// #if _SPLICE_SOFTIMAGE_VERSION >= 2014
//     else
//     {
//       CValueArray extraParams = ctxt.GetAttribute("ExtraParams");
//       CValueArray extraParams0 = extraParams[0];
//       LONG col = extraParams0[0];
//       LONG row = extraParams0[1];
//       Parameter gridParam = ctxt.GetSource();

//       if(ctxt.GetEventID() == PPGEventContext::siGridDataOnContextMenuInit)
//       {    
//         CValueArray items;
//         if(gridParam.GetScriptName().IsEqualNoCase(L"ports"))
//         {
//           items.Add(L"Add new Port");
//           items.Add(0);
//           items.Add(L"Remove selected port");
//           items.Add(1);
//         }
//         else
//         {
//           items.Add(L"Add new Operator");
//           items.Add(0);
//           items.Add(L"Remove selected Operator");
//           items.Add(1);
//           items.Add(L"Edit selected Operator");
//           items.Add(2);
//         }
//         ctxt.PutAttribute("Return", items);
//       }
//       else if(ctxt.GetEventID() == PPGEventContext::siGridDataOnContextMenuInit)
//       {    
//       }
//     }
// #endif
  }
  return CStatus::OK;
}

AddPortDialog::AddPortDialog(int portPurpose, CString defaultPortMode)
{
  CValueArray addpropArgs(5) ;
  addpropArgs[0] = L"CustomProperty";
  addpropArgs[1] = L"Scene_Root";
  addpropArgs[3] = L"AddPortDialog";

  CValue retVal;
  Application().ExecuteCommand( L"SIAddProp", addpropArgs, retVal );
  CValueArray retValArray = retVal;
  retVal = retValArray[0];
  retValArray = retVal;
  retVal = retValArray[0];
  Application().LogMessage(retVal.GetAsText());
  _prop = (CRef)retVal;


  Parameter param;
  _prop.AddParameter(L"portName", CValue::siString, siPersistable, "", "", "", param);
  if(portPurpose < 0)
    _prop.AddParameter(L"iceAttr", CValue::siString, siPersistable, "", "", "", param);
  else
  {
    _prop.AddParameter(L"portType", CValue::siString, siPersistable, "", "", "Scalar", param);
    _prop.AddParameter(L"customRT", CValue::siString, siPersistable | siReadOnly, "", "", "", param);
    _prop.AddParameter(L"extension", CValue::siString, siPersistable | siReadOnly, "", "", "", param);
  }
  if(portPurpose > 0)
    _prop.AddParameter(L"portMode", CValue::siString, siPersistable, "", "", defaultPortMode, param);
  if(portPurpose > 0)
    _prop.AddParameter(L"arrayMode", CValue::siBool, siPersistable, "", "", false, param);

  PPGLayout layout = _prop.GetPPGLayout();
  layout.Clear();
  layout.AddItem("portName", "Name");

  CValueArray combo;
  if(portPurpose == 0)
    combo = FabricSpliceBaseInterface::getSpliceParamTypeCombo();
  else if(portPurpose == 1)
    combo = FabricSpliceBaseInterface::getSpliceXSIPortTypeCombo();
  else
    combo = FabricSpliceBaseInterface::getSpliceInternalPortTypeCombo();

  if(portPurpose < 0)
    layout.AddItem("iceAttr", "ICE Attribute");
  else
  {
    layout.AddEnumControl("portType", combo, "Type");
    layout.AddItem("customRT", "Custom");
    layout.AddItem("extension", "Extension");

    layout.PutLanguage(L"JScript");

    CString logic;
    logic += L"function portType_OnChanged() {\n";
    logic += L"  var prop = PPG.Inspected(0);\n";
    logic += L"  var readOnly = prop.Parameters.Item('portType').value != 'Custom';\n";
    logic += L"  prop.Parameters.Item('customRT').SetCapabilityFlag(siReadOnly, readOnly);\n";
    logic += L"  prop.Parameters.Item('extension').SetCapabilityFlag(siReadOnly, readOnly);\n";
    logic += L"}\n";
    layout.PutLogic(logic);
  }

  combo.Clear();
  if(portPurpose > 0)
  {
    combo.Add(L"IN"); combo.Add(L"IN");
    combo.Add(L"OUT"); combo.Add(L"OUT");
    combo.Add(L"IO"); combo.Add(L"IO");
    layout.AddEnumControl("portMode", combo, "Mode");
  }
  if(portPurpose > 0)
  {
    layout.AddItem("arrayMode", "Array");
  }
}

AddPortDialog::~AddPortDialog()
{
  CValue returnVal;
  CValueArray args(1);
  args[0] = _prop.GetFullName();
  Application().ExecuteCommand(L"DeleteObj", args, returnVal);
}

bool AddPortDialog::getDataType(CString & portType, CString & extension)
{
  portType = _prop.GetParameterValue(L"portType");
  if(portType == L"Custom")
  {
    portType = _prop.GetParameterValue(L"customRT");
    if(portType.IsEmpty())
      return false;
    extension = _prop.GetParameterValue(L"extension");
  }
  return true;
}


bool AddPortDialog::show(CString title)
{
  CValue returnVal;
  CValueArray args(5);
  args[0] = _prop.GetFullName();
  args[1] = L"";
  args[2] = title;
  args[3] = siModal;
  args[4] = false;
  Application().ExecuteCommand(L"InspectObj", args, returnVal);
  return !(bool)returnVal;
}

AddOperatorDialog::AddOperatorDialog()
{
  CValueArray addpropArgs(5) ;
  addpropArgs[0] = L"CustomProperty";
  addpropArgs[1] = L"Scene_Root";
  addpropArgs[3] = L"AddOperatorDialog";

  CValue retVal;
  Application().ExecuteCommand( L"SIAddProp", addpropArgs, retVal );
  CValueArray retValArray = retVal;
  retVal = retValArray[0];
  retValArray = retVal;
  retVal = retValArray[0];
  _prop = (CRef)retVal;

  Parameter param;
  _prop.AddParameter(L"opName", CValue::siString, siPersistable, "", "", "", param);

  PPGLayout layout = _prop.GetPPGLayout();
  layout.Clear();
  layout.AddItem("opName", "Name");
}

AddOperatorDialog::~AddOperatorDialog()
{
  CValue returnVal;
  CValueArray args(1);
  args[0] = _prop.GetFullName();
  Application().ExecuteCommand(L"DeleteObj", args, returnVal);
}

bool AddOperatorDialog::show(CString title)
{
  CValue returnVal;
  CValueArray args(5);
  args[0] = _prop.GetFullName();
  args[1] = L"";
  args[2] = title;
  args[3] = siModal;
  args[4] = false;
  Application().ExecuteCommand(L"InspectObj", args, returnVal);
  return !(bool)returnVal;
}

ImportSpliceDialog::ImportSpliceDialog(CString fileName)
{
  CValueArray addpropArgs(5) ;
  addpropArgs[0] = L"ImportSpliceDialog";
  addpropArgs[1] = L"Scene_Root";
  addpropArgs[3] = L"ImportSpliceDialog";

  CValue retVal;
  Application().ExecuteCommand( L"SIAddProp", addpropArgs, retVal );
  CValueArray retValArray = retVal;
  retVal = retValArray[0];
  retValArray = retVal;
  retVal = retValArray[0];
  _prop = (CRef)retVal;
  _prop.PutParameterValue(L"objectID", (LONG)_prop.GetObjectID());
  _prop.PutParameterValue(L"fileName", fileName);
}

ImportSpliceDialog::~ImportSpliceDialog()
{
}

bool ImportSpliceDialog::show(CString title)
{
  CValue returnVal;
  CValueArray args(5);
  args[0] = _prop.GetFullName();
  args[1] = L"";
  args[2] = title;
  args[3] = siLock;
  args[4] = false;
  Application().ExecuteCommand(L"InspectObj", args, returnVal);
  return !(bool)returnVal;
}

SICALLBACK ImportSpliceDialog_Define( CRef& in_ctxt )
{
  Context ctxt( in_ctxt );
  CustomProperty prop = ctxt.GetSource();
  Parameter param;

  prop.AddParameter(L"logoBitmap", CValue::siInt4, siReadOnly, "", "", 0, param);
  prop.AddParameter(L"objectID", CValue::siInt4, siReadOnly, "", "", 0, param);
  prop.AddParameter(L"fileName", CValue::siString, siReadOnly, "", "", "", param);
  prop.AddGridParameter(L"ports");
  return CStatus::OK;
}

SICALLBACK ImportSpliceDialog_DefineLayout( CRef& in_ctxt )
{
  Context ctxt( in_ctxt );
  PPGLayout layout = ctxt.GetSource();
  PPGItem item;
  layout.Clear();

  CString logoPath = xsiGetWorkgroupPath()+CUtils::Slash()+"Application"+CUtils::Slash()+"UI"+CUtils::Slash()+"FE_logo.bmp";
  
  item = layout.AddItem("logoBitmap", "SpliceLogo", siControlBitmap);
  item.PutAttribute(siUIFilePath, logoPath);
  item.PutAttribute(siUINoLabel, true);

  layout.AddGroup("Ports");
  item = layout.AddItem( "ports", "Ports", siControlGrid);
  item.PutAttribute(siUIGridHideRowHeader, true);
  item.PutAttribute(siUIGridLockColumnHeader, true);
  item.PutAttribute(siUINoLabel, true);
  item.PutAttribute(siUIWidthPercentage, 100);
  item.PutAttribute(siUIGridColumnWidths, "0:150:80:40:200");
  item.PutAttribute(siUIGridReadOnlyColumns, "1:1:1:1:1");
  layout.AddRow();
  item = layout.AddButton("createDefault", "Create Default");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  item = layout.AddButton("pickTarget", "Pick Target");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  layout.EndRow();
  layout.EndGroup();
  layout.AddRow();
  item = layout.AddButton("cancelButton", "Cancel");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  item = layout.AddButton("okButton", "OK");
  item.PutAttribute(siUICX, gButtonWidth);
  item.PutAttribute(siUICY, gButtonHeight);
  layout.EndRow();

  return CStatus::OK;
}

void updateImportSpliceDialogGrids(CustomProperty & prop)
{
  GridData portsGrid((CRef)prop.GetParameter("ports").GetValue());
  portsGrid.PutColumnCount(4);
  portsGrid.PutColumnLabel(0, "Name");
  portsGrid.PutColumnLabel(1, "Type");
  portsGrid.PutColumnLabel(2, "Mode");
  portsGrid.PutColumnLabel(3, "Target");
  portsGrid.PutColumnType(0, siColumnStandard);
  portsGrid.PutColumnType(1, siColumnStandard);
  portsGrid.PutColumnType(2, siColumnCombo);
  portsGrid.PutColumnType(3, siColumnStandard);
  CValueArray portsCombo;
  portsCombo.Add(L"IN");
  portsCombo.Add((LONG)0);
  portsCombo.Add(L"OUT");
  portsCombo.Add((LONG)1);
  portsCombo.Add(L"IO");
  portsCombo.Add((LONG)2);
  portsGrid.PutColumnComboItems(2, portsCombo);

  LONG objectID = prop.GetParameterValue("objectID");
  FabricSpliceBaseInterface * interf = FabricSpliceBaseInterface::getInstanceByObjectID(objectID);
  if(interf)
  {
    FabricSplice::DGGraph graph = interf->getSpliceGraph();
    ULONG nbRows = 0;
    portsGrid.PutRowCount(0);
    for(unsigned int i = 0; i < graph.getDGPortCount(); i++)
    {
      CString portName = graph.getDGPortName(i);
      FabricSplice::DGPort port = graph.getDGPort(portName.GetAsciiString());
      CString portType = port.getDataType();
      if(port.isArray())
        portType += L"[]";
      FabricSplice::Port_Mode portMode = port.getMode();
      CString targetsStr = interf->getXSIPortTargets(portName);

      SoftimagePortType xsiPortType = SoftimagePortType_Port;
      FabricCore::Variant xsiPortTypeFlag = port.getOption("SoftimagePortType");
      if(xsiPortTypeFlag.isSInt32())
        xsiPortType = (SoftimagePortType)xsiPortTypeFlag.getSInt32();

      if(port.getOption("ICEAttribute").isString())
      {
        targetsStr += ":";
        targetsStr += port.getOption("ICEAttribute").getStringData();
      }
      else if(!(
        portType == L"Boolean" || 
        portType == L"Integer" || 
        portType == L"Scalar" || 
        portType == L"String" || 
        portType == L"Mat44" || 
        portType == L"Mat44[]" || 
        portType == L"PolygonMesh" ||
        portType == L"Lines") || 
        xsiPortType != SoftimagePortType_Port)
        continue;

      portsGrid.PutRowCount(nbRows+1);
      portsGrid.PutCell(0, nbRows, portName);
      portsGrid.PutCell(1, nbRows, portType);
      portsGrid.PutCell(2, nbRows, (LONG)portMode);
      portsGrid.PutCell(3, nbRows, targetsStr);
      nbRows++;
    }
  }
}

SICALLBACK ImportSpliceDialog_PPGEvent( CRef& in_ctxt )
{
  PPGEventContext ctxt( in_ctxt );
  
  CRefArray inspected = ctxt.GetInspectedObjects();

  if(ctxt.GetEventID() == PPGEventContext::siOnInit)
  {
    CustomProperty prop = ctxt.GetSource();
    updateImportSpliceDialogGrids(prop);
  }
  else if(ctxt.GetEventID() == PPGEventContext::siOnClosed)
  {
    CustomProperty prop = ctxt.GetSource();
    LONG objectID = prop.GetParameterValue("objectID");
    FabricSpliceBaseInterface * interf = FabricSpliceBaseInterface::getInstanceByObjectID(objectID);
    if(interf)
      delete(interf);
    pushRefToDelete(prop.GetRef());
  }
  else if(ctxt.GetEventID() == PPGEventContext::siButtonClicked )
  {
    CustomProperty prop(inspected[0]);

    LONG objectID = prop.GetParameterValue("objectID");
    FabricSpliceBaseInterface * interf = FabricSpliceBaseInterface::getInstanceByObjectID(objectID);
    if(!interf)
      return CStatus::OK;

    CString button = ctxt.GetAttribute(L"Button");
    if(button.IsEqualNoCase(L"cancelButton"))
    {
      delete(interf);
      ctxt.PutAttribute(L"Close", true);
      return CStatus::OK;
    }
    else if(button.IsEqualNoCase(L"okButton"))
    {
      delete(interf);
      ctxt.PutAttribute(L"Close", true);
      CString fileName = prop.GetParameterValue(L"fileName");
      CString json = L"{\"fileName\":\""+fileName+"\", \"hideUI\": true";

      GridData portsGrid((CRef)prop.GetParameter("ports").GetValue());
      for(ULONG i=0;i<portsGrid.GetRowCount();i++)
      {
        CString portName = portsGrid.GetCell(0, i);
        CString portTarget = portsGrid.GetCell(3, i);
        if(portTarget.IsEmpty())
          continue;
        if(portTarget.GetSubString(0,1) == ":")
          continue;
        json += L",\""+portName+"\":\""+portTarget+"\"";
      }

      json += "}";
      CValue returnVal;
      CValueArray args(2);
      args[0] = L"loadSplice";
      args[1] = json;
      Application().ExecuteCommand(L"fabricSplice", args, returnVal);
      return CStatus::OK;
    }

    GridData portsGrid((CRef)prop.GetParameter("ports").GetValue());
    GridWidget portsWidget = portsGrid.GetGridWidget();

    CLongArray gridSelection = getGridWidgetSelection(portsWidget, portsGrid);
    if(gridSelection.GetCount() == 0)
    {
      LONG result;
      Application().GetUIToolkit().MsgBox(L"You need to select a port first.", siMsgOkOnly, "Information", result);
      return CStatus::OK;
    }

    if(button.IsEqualNoCase(L"createDefault"))
    {
      for(ULONG i=1;i<gridSelection.GetCount();i+=2)
      {
        CString portName = portsGrid.GetCell(0, gridSelection[i]);
        CString dataType = portsGrid.GetCell(1, gridSelection[i]);
        LONG portMode = portsGrid.GetCell(2, gridSelection[i]);
        if(dataType == L"Mat44")
        {
          Null target;
          Application().GetActiveSceneRoot().AddNull(portName, target);
          interf->setXSIPortTargets(portName, target.GetKinematics().GetGlobal().GetFullName());
          FabricCore::RTVal rtVal = FabricSplice::constructRTVal("Mat44");
          FabricSplice::DGPort port = interf->getSpliceGraph().getDGPort(portName.GetAsciiString());
          std::string jsonForMat = port.getDefault().getJSONEncoding().getStringData();
          rtVal.setJSON(FabricSplice::constructStringRTVal(jsonForMat.c_str()));
          MATH::CMatrix4 matrix;
          getCMatrix4FromRTVal(rtVal, matrix);
          
          MATH::CTransformation transform;
          transform.SetMatrix4(matrix);
          target.GetKinematics().GetGlobal().PutTransform(transform);
          portsGrid.PutCell(3, gridSelection[i], target.GetKinematics().GetGlobal().GetFullName());
        }
        else if(dataType == L"PolygonMesh" && portMode == 1)
        {
          X3DObject target;
          CMeshBuilder meshBuilder;
          Application().GetActiveSceneRoot().AddPolygonMesh( portName, target, meshBuilder );
          portsGrid.PutCell(3, gridSelection[i], target.GetActivePrimitive().GetFullName());
        }
      }
    }
    else if(button.IsEqualNoCase(L"pickTarget"))
    {
      for(ULONG i=1;i<gridSelection.GetCount();i+=2)
      {
        CString portName = portsGrid.GetCell(0, gridSelection[i]);
        CString dataType = portsGrid.GetCell(1, gridSelection[i]);
        CString portTarget = portsGrid.GetCell(3, gridSelection[i]);
        bool isArray = dataType.GetSubString(dataType.Length()-2,2) == L"[]";
        bool isIceAttribute = portTarget.GetSubString(0,1) == ":";
        if(!isIceAttribute)
          isIceAttribute = portTarget.Split(":").GetCount() > 1;
        CString filter = L"global";
        if(dataType.IsEqualNoCase(L"PolygonMesh"))
          filter = L"polymsh";
        else if(dataType.IsEqualNoCase(L"Lines"))
          filter = L"crvlist";
        if(isIceAttribute)
          filter = L"geometry";
        CRefArray items = PickObjectArray(
          L"Pick target for "+portName, L"Pick next target for "+portName, 
          filter, (!isArray || isIceAttribute) ? 1 : 0);
        if(items.GetCount() > 0)
        {
          if(isIceAttribute)
          {
            CString errorMessage;
            CString iceAttribute = portTarget.Split(":").GetCount() > 1 ? portTarget.Split(":")[1] : portTarget.Split(":")[0];
            if(dataType != getSpliceDataTypeFromICEAttribute(items, iceAttribute, errorMessage))
            {
              xsiLogErrorFunc(errorMessage);
              xsiLogErrorFunc("Data type of picked item(s) does not match '"+dataType+"'.");
              return CStatus::OK;
            }
            portsGrid.PutCell(3, gridSelection[i], items.GetAsText()+L":"+iceAttribute);
          }
          else
          {
            CString pickedDataType = getSpliceDataTypeFromRefArray(items, dataType);
            if(dataType != pickedDataType && dataType != pickedDataType + "[]")
            {
              xsiLogErrorFunc("Data type of picked item(s) does not match '"+dataType+"'.");
              return CStatus::OK;
            }
            portsGrid.PutCell(3, gridSelection[i], items.GetAsText());
          }
        }
      }
    }
  }
  return CStatus::OK;
}

LicenseDialog::LicenseDialog()
{
  CValueArray addpropArgs(5) ;
  addpropArgs[0] = L"CustomProperty";
  addpropArgs[1] = L"Scene_Root";
  addpropArgs[3] = L"LicenseDialog";

  CString logoPath = xsiGetWorkgroupPath()+CUtils::Slash()+"Application"+CUtils::Slash()+"UI"+CUtils::Slash()+"FE_logo.bmp";

  CValue retVal;
  Application().ExecuteCommand( L"SIAddProp", addpropArgs, retVal );
  CValueArray retValArray = retVal;
  retVal = retValArray[0];
  retValArray = retVal;
  retVal = retValArray[0];
  _prop = (CRef)retVal;

  Parameter param;
  _prop.AddParameter(L"opName", CValue::siString, siPersistable, "", "", "", param);
  _prop.AddParameter(L"logoBitmap", CValue::siInt4, siReadOnly, "", "", 0, param);
  _prop.AddParameter(L"licenseServer", CValue::siString, siPersistable, "", "", "", param);
  
  CString licenseText;
  licenseText += "Please enter the license server below.\nThe server format needs to be rlm://host:port,\nso for example rlm://127.0.0.1:5053.\n\n";
  licenseText += "You may also follow the link below to request a new license.\n";
  licenseText += "http://fabricengine.com/get-fabric/\n";
    
  PPGLayout layout = _prop.GetPPGLayout();
  layout.Clear();
  PPGItem item = layout.AddItem("logoBitmap", "SpliceLogo", siControlBitmap);
  item.PutAttribute(siUIFilePath, logoPath);
  item.PutAttribute(siUINoLabel, true);
  layout.AddStaticText(licenseText);
  item = layout.AddItem("licenseServer", "Server");
}

LicenseDialog::~LicenseDialog()
{
  CValue returnVal;
  CValueArray args(1);
  args[0] = _prop.GetFullName();
  Application().ExecuteCommand(L"DeleteObj", args, returnVal);
}

bool LicenseDialog::show()
{
  CValue returnVal;
  CValueArray args(5);
  args[0] = _prop.GetFullName();
  args[1] = L"";
  args[2] = L"Fabric:Splice Licensing";
  args[3] = siModal;
  args[4] = false;
  Application().ExecuteCommand(L"InspectObj", args, returnVal);
  return !(bool)returnVal;
}
