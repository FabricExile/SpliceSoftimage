#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_pluginregistrar.h>
#include <xsi_progressbar.h>
#include <xsi_uitoolkit.h>
#include <xsi_customproperty.h>
#include <xsi_time.h>
#include <xsi_comapihandler.h>
#include <xsi_project.h>
#include <xsi_plugin.h>
#include <xsi_utils.h>
#include <xsi_desktop.h>
#include <xsi_layout.h>
#include <xsi_view.h>
#include <xsi_command.h>
#include <xsi_model.h>
#include <xsi_primitive.h>
#include <xsi_polygonmesh.h>
#include <xsi_cluster.h>
#include <xsi_clusterproperty.h>
#include <xsi_material.h>
#include <xsi_materiallibrary.h>
#include <xsi_geometryaccessor.h>
#include <xsi_clusterpropertybuilder.h>
#include <xsi_property.h>
#include <xsi_argument.h>

#include "plugin.h"
#include "FabricSplicePlugin.h"
#include "FabricSpliceOperators.h"
#include "FabricSpliceICENodes.h"
#include "FabricSpliceDialogs.h"
#include "FabricSpliceRenderPass.h"
#include "FabricSpliceBaseInterface.h"

using namespace XSI;

SICALLBACK XSILoadPlugin(PluginRegistrar& in_reg)
{
  // set plugin's name, version and author.
  in_reg.PutAuthor(L"Fabric Engine");
  in_reg.PutName  (L"Fabric Engine Plugin");
  in_reg.PutVersion(FabricSplice::GetFabricVersionMaj(),
                    FabricSplice::GetFabricVersionMin());

  // register the (old) Fabric Splice things.
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

  // register the (new) Fabric DFG/Canvas things.
  {
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

