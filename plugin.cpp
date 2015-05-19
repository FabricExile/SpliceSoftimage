#include <xsi_application.h>
#include <xsi_pluginregistrar.h>

#include "plugin.h"
#include "FabricDFGBaseInterface.h"
#include "FabricDFGPlugin.h"
#include "FabricSpliceICENodes.h"

using namespace XSI;

SICALLBACK XSILoadPlugin(PluginRegistrar& in_reg)
{
  // set plugin's name, version and author.
  in_reg.PutAuthor(L"Fabric Engine");
  in_reg.PutName  (L"Fabric Engine Plugin");
  in_reg.PutVersion(FabricCore::GetVersionMaj(),
                    FabricCore::GetVersionMin());

  // register the (old) Fabric Splice.
  {
    // rendering.
    in_reg.RegisterDisplayCallback(L"SpliceRenderPass");

    // properties.
    in_reg.RegisterProperty(L"SpliceInfo");
  
    // dialogs.
    in_reg.RegisterProperty(L"SpliceEditor");
    in_reg.RegisterProperty(L"ImportSpliceDialog");

    // operators.
    in_reg.RegisterOperator(L"SpliceOp");

    // commands.
    in_reg.RegisterCommand(L"fabricSplice",             L"fabricSplice");
    in_reg.RegisterCommand(L"fabricSpliceManipulation", L"fabricSpliceManipulation");
    in_reg.RegisterCommand(L"proceedToNextScene",       L"proceedToNextScene");

    // tools.
    in_reg.RegisterTool(L"fabricSpliceTool");

    // menu.
    in_reg.RegisterMenu(siMenuMainTopLevelID, "Fabric:Splice", true, true);

    // events.
    in_reg.RegisterEvent(L"FabricSpliceNewScene",       siOnEndNewScene);
    in_reg.RegisterEvent(L"FabricSpliceCloseScene",     siOnCloseScene);
    in_reg.RegisterEvent(L"FabricSpliceOpenBeginScene", siOnBeginSceneOpen);
    in_reg.RegisterEvent(L"FabricSpliceOpenEndScene",   siOnEndSceneOpen);
    in_reg.RegisterEvent(L"FabricSpliceSaveScene",      siOnBeginSceneSave);
    in_reg.RegisterEvent(L"FabricSpliceSaveAsScene",    siOnBeginSceneSaveAs);
    in_reg.RegisterEvent(L"FabricSpliceTerminate",      siOnTerminate);
    in_reg.RegisterEvent(L"FabricSpliceSaveScene2",     siOnBeginSceneSave2);
    in_reg.RegisterEvent(L"FabricSpliceImport",         siOnEndFileImport);
    in_reg.RegisterEvent(L"FabricSpliceBeginExport",    siOnBeginFileExport);
    in_reg.RegisterEvent(L"FabricSpliceEndExport",      siOnEndFileExport);
    in_reg.RegisterEvent(L"FabricSpliceValueChange",    siOnValueChange);

    // ice nodes.
    Register_spliceGetData(in_reg);
  }

  // register the (new) Fabric DFG/Canvas.
  {
    // operators.
    in_reg.RegisterOperator(L"dfgSoftimageOp");

    // commands.
    in_reg.RegisterCommand(L"dfgSoftimageOpApply", L"dfgSoftimageOpApply");
    in_reg.RegisterCommand(L"dfgImportJSON",       L"dfgImportJSON");
    in_reg.RegisterCommand(L"dfgExportJSON",       L"dfgExportJSON");
    in_reg.RegisterCommand(L"dfgLogStatus",        L"dfgLogStatus");

    // menu.
    in_reg.RegisterMenu(siMenuMainTopLevelID, "Fabric:DFG", true, true);
  }

  // done.
  return CStatus::OK;
}

SICALLBACK XSIUnloadPlugin(const PluginRegistrar& in_reg)
{
  CString strPluginName;
  strPluginName = in_reg.GetName();
  Application().LogMessage(strPluginName + L" has been unloaded.",siVerboseMsg);

  return CStatus::OK;
}

