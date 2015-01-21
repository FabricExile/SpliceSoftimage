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

#include <boost/filesystem.hpp>

#include "plugin.h"
#include "operators.h"
#include "icenodes.h"
#include "dialogs.h"
#include "renderpass.h"
#include "FabricSpliceBaseInterface.h"

using namespace XSI;

void xsiLogFunc(const char * message, unsigned int length)
{
  Application().LogMessage(CString("[Splice] ")+CString(message), siVerboseMsg);
}

bool gErrorEnabled = true;
void xsiErrorLogEnable(bool enable)
{
  gErrorEnabled = enable;
}

bool gErrorOccured = false;
void xsiLogErrorFunc(const char * message, unsigned int length)
{
  if(!gErrorEnabled)
    return;
  Application().LogMessage(CString("[Splice] ")+CString(message), siErrorMsg);
  gErrorOccured = true;
}

void xsiLogFunc(CString message)
{
  xsiLogFunc(message.GetAsciiString(), message.Length());
}

void xsiLogErrorFunc(CString message)
{
  xsiLogErrorFunc(message.GetAsciiString(), message.Length());
}

void xsiClearError()
{
  gErrorOccured = false;
}

CStatus xsiErrorOccured()
{
  CStatus result = CStatus::OK;;
  if(gErrorOccured)
    result = CStatus::Fail;
  gErrorOccured = false;
  return result;
}

void xsiKLReportFunc(const char * message, unsigned int length)
{
  Application().LogMessage(CString("[KL]: ")+CString(message));
}

void xsiCompilerErrorFunc(unsigned int row, unsigned int col, const char * file, const char * level, const char * desc)
{
  CString line((LONG)row);
  if(CString(level) == CString("error"))
    Application().LogMessage("[KL Compiler "+CString(level)+"]: line "+line+", op '"+CString(file)+"': "+CString(desc), siErrorMsg);
  else if(CString(level) == CString("warning"))
    Application().LogMessage("[KL Compiler "+CString(level)+"]: line "+line+", op '"+CString(file)+"': "+CString(desc), siWarningMsg);
  else
    Application().LogMessage("[KL Compiler "+CString(level)+"]: line "+line+", op '"+CString(file)+"': "+CString(desc), siInfoMsg);

  CustomProperty klEditor = editorPropGet();
  if(klEditor.IsValid())
  {
    CString operatorName = klEditor.GetParameterValue(L"operatorName");
    if(operatorName == file || operatorName + ".kl" == file)
    {
      CString klErrors = klEditor.GetParameterValue(L"klErrors");
      if(!klErrors.IsEmpty())
        klErrors += L"\n";
      klErrors += L"Line "+line + L": " + CString(desc);
      klEditor.PutParameterValue(L"klErrors", klErrors);
    }
  }
}

void xsiKLStatusFunc(const char * topic, unsigned int topicLength,  const char * message, unsigned int messageLength)
{
  Application().LogMessage(CString("[KL Status]: ")+CString(message), siVerboseMsg);
}

CString xsiGetWorkgroupPath()
{
  // look for the plugin location
  CRefArray plugins = Application().GetPlugins();
  CString pluginPath;
  for(LONG i=0;i<plugins.GetCount();i++)
  {
    Plugin plugin(plugins[i]);
    if(plugin.GetName() != L"Fabric:Splice Plugin")
      continue;

    CStringArray parts = plugin.GetFilename().Split(CUtils::Slash());
    pluginPath = parts[0];
    for(LONG j=1;j<parts.GetCount()-3;j++)
    {
      pluginPath += CUtils::Slash();
      pluginPath += parts[j];
    }

#ifndef _WIN32
    pluginPath = "/" + pluginPath;
#endif

    return pluginPath;
  }
  return CString();
}

SICALLBACK XSILoadPlugin( PluginRegistrar& in_reg )
{
  in_reg.PutAuthor(L"Fabric Engine");
  in_reg.PutName(L"Fabric:Splice Plugin");
  in_reg.PutVersion(1,0);

  // rendering
  in_reg.RegisterDisplayCallback(L"SpliceRenderPass");

  // properties
  in_reg.RegisterProperty(L"SpliceInfo");
  
  // dialogs
  in_reg.RegisterProperty(L"SpliceEditor");
  in_reg.RegisterProperty(L"ImportSpliceDialog");

  // operators
  in_reg.RegisterOperator(L"SpliceOp");

  // commands
  in_reg.RegisterCommand(L"fabricSplice", L"fabricSplice");
  in_reg.RegisterCommand(L"fabricSpliceManipulation", L"fabricSpliceManipulation");
  in_reg.RegisterCommand(L"proceedToNextScene", L"proceedToNextScene");

  // tools
  in_reg.RegisterTool(L"fabricSpliceTool");

  // menu
  in_reg.RegisterMenu(siMenuMainTopLevelID, "Fabric:Splice", true, true);

  // events
  in_reg.RegisterEvent(L"FabricSpliceNewScene", siOnEndNewScene);
  in_reg.RegisterEvent(L"FabricSpliceCloseScene", siOnCloseScene);
  in_reg.RegisterEvent(L"FabricSpliceOpenBeginScene", siOnBeginSceneOpen);
  in_reg.RegisterEvent(L"FabricSpliceOpenEndScene", siOnEndSceneOpen);
  in_reg.RegisterEvent(L"FabricSpliceSaveScene", siOnBeginSceneSave);
  in_reg.RegisterEvent(L"FabricSpliceSaveAsScene", siOnBeginSceneSaveAs);
  in_reg.RegisterEvent(L"FabricSpliceTerminate", siOnTerminate);
  in_reg.RegisterEvent(L"FabricSpliceSaveScene2", siOnBeginSceneSave2);
  in_reg.RegisterEvent(L"FabricSpliceImport", siOnEndFileImport);
  in_reg.RegisterEvent(L"FabricSpliceBeginExport", siOnBeginFileExport);
  in_reg.RegisterEvent(L"FabricSpliceEndExport", siOnEndFileExport);

  // ice nodes
  Register_spliceGetData(in_reg);

  return CStatus::OK;
}

bool gSpliceInitialized = false;
void xsiInitializeSplice()
{
  if(gSpliceInitialized)
    return;

  gSpliceInitialized = true;
  FabricSplice::Initialize();
  FabricSplice::Logging::setLogFunc(xsiLogFunc);
  FabricSplice::Logging::setLogErrorFunc(xsiLogErrorFunc);
  FabricSplice::Logging::setKLReportFunc(xsiKLReportFunc);
  FabricSplice::Logging::setKLStatusFunc(xsiKLStatusFunc);
  FabricSplice::Logging::setCompilerErrorFunc(xsiCompilerErrorFunc);

  CString workgroupFolder = xsiGetWorkgroupPath();
  CString extFolder = workgroupFolder + "/../../Exts"; // above the 'SpliceIntegrations' folder
  if(boost::filesystem::exists(extFolder.GetAsciiString()))
  {
    boost::filesystem::path extFolderAbs = boost::filesystem::canonical(extFolder.GetAsciiString());
    extFolderAbs.make_preferred();
    FabricSplice::addExtFolder(extFolderAbs.string().c_str());
  }
  else
    xsiLogErrorFunc("The standard fabric exts folder '"+extFolder+" does not exist.");
}

SICALLBACK XSIUnloadPlugin( const PluginRegistrar& in_reg )
{
  CString strPluginName;
  strPluginName = in_reg.GetName();
  Application().LogMessage(strPluginName + L" has been unloaded.",siVerboseMsg);

  return CStatus::OK;
}

XSIPLUGINCALLBACK CStatus FabricSpliceNewScene_OnEvent(CRef & ctxt)
{
  destroyDrawContext();
  FabricSplice::DestroyClient();
  return true;
}

XSIPLUGINCALLBACK CStatus FabricSpliceCloseScene_OnEvent(CRef & ctxt)
{
  destroyDrawContext();
  FabricSplice::DestroyClient();
  return true;
}

XSIPLUGINCALLBACK CStatus FabricSpliceTerminate_OnEvent(CRef & ctxt)
{
  destroyDrawContext();
  FabricSplice::DestroyClient(true);
  FabricSplice::Finalize();
  return true;
}

bool gIsOpeningScene = false;
bool xsiIsLoadingScene()
{
  return gIsOpeningScene;
}

CString gScenePath;
CString xsiGetLastLoadedScene()
{
  return gScenePath;
}

XSIPLUGINCALLBACK CStatus FabricSpliceOpenBeginScene_OnEvent(CRef & ctxt)
{
  Context c(ctxt);
  gScenePath = c.GetAttribute("FileName");
  gIsOpeningScene = true;
  return true;
}

XSIPLUGINCALLBACK CStatus FabricSpliceOpenEndScene_OnEvent(CRef & ctxt)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceOpenEndScene_OnEvent");
  
  Context context(ctxt);
  CString fileName = context.GetAttribute("FileName");
  gIsOpeningScene = false;
  try
  {
    std::vector<opUserData*> instances = getXSIOperatorInstances();
    for(size_t i=0;i<instances.size();i++)
    {
      if(instances[i]->getInterf() == NULL)
      {
        FabricSpliceBaseInterface * interf = new FabricSpliceBaseInterface();
        interf->setObjectID(instances[i]->getObjectID());
      }

      // if the interface is still null, then there has been
      // a problem loading one of the extension or the operator code
      if(instances[i]->getInterf() == NULL)
      {
        xsiLogErrorFunc("Problem loading extensions / dependencies.");
        return CStatus::Unexpected;
      }
      instances[i]->getInterf()->restoreFromPersistenceData(fileName);
      
    }
    CustomProperty dialog = editorPropGet();
    if(dialog.IsValid())
    {
      CValue returnVal;
      CValueArray args(1);
      args[0] = dialog.GetFullName();
      Application().ExecuteCommand("DeleteObj", args, returnVal);
    }
  }
  catch(FabricCore::Exception e)
  {
    xsiLogErrorFunc(e.getDesc_cstr());
  }
  return FabricSpliceNewScene_OnEvent(ctxt);
}

XSIPLUGINCALLBACK CStatus FabricSpliceImport_OnEvent(CRef & ctxt)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceImport_OnEvent");
  Context context(ctxt);
  if(LONG(context.GetAttribute("FileType")) != siFileTypeModel)
    return true;

  CString fileName = context.GetAttribute("FileName");
  CString modelName = context.GetAttribute("Name");
  bool reference = context.GetAttribute("Reference");
  CRef modelRef;
  modelRef.Set(modelName);
  Model model(modelRef);
  CString modelPrefix = modelName + ".";

  try
  {
    std::vector<opUserData*> instances = getXSIOperatorInstances();
    for(size_t i=0;i<instances.size();i++)
    {
      if(instances[i]->getInterf() != NULL)
        continue;
      CustomOperator op(Application().GetObjectFromID(instances[i]->getObjectID()));
      if(!op.IsValid())
        continue;
      if(reference)
      {
        CString opFullName = op.GetFullName();
        CString opModelPrefix = opFullName.GetSubString(0, modelPrefix.Length());
        if(!opModelPrefix.IsEqualNoCase(modelPrefix))
          continue;
      }

      FabricSpliceBaseInterface * interf = new FabricSpliceBaseInterface();
      interf->setObjectID(instances[i]->getObjectID());
      if(instances[i]->getInterf() != NULL)
      {
        instances[i]->getInterf()->restoreFromPersistenceData(fileName);
        instances[i]->getInterf()->reconnectForImport(model);
      }
    }

    FabricSpliceBaseInterface::cleanupForImport(model);
  }
  catch(FabricCore::Exception e)
  {
    xsiLogErrorFunc(e.getDesc_cstr());
  }
  return true;
}


CRefArray getXSIOperatorsFromModel(Model & model)
{
  CRefArray spliceOps;
  CRefArray x3ds;
  x3ds.Add(model.GetRef());
  for(ULONG i=0;i<x3ds.GetCount();i++)
  {
    X3DObject x3d(x3ds[i]);
    if(!x3d.IsValid())
      continue;

    CStringArray itemStr;
    itemStr.Add(x3ds[i].GetAsText());
    itemStr.Add(itemStr[0]+L".kine.local.SpliceOp");
    itemStr.Add(itemStr[0]+L".kine.global.SpliceOp");
    itemStr.Add(x3d.GetActivePrimitive().GetFullName()+L".SpliceOp");

    for(LONG j=0;j<itemStr.GetCount();j++)
    {
      CRef target;
      target.Set(itemStr[j]);
      if(!target.IsValid())
        continue;
      CustomOperator op(target);
      if(op.IsValid() && op.GetType().IsEqualNoCase(L"SpliceOp"))
      {
        spliceOps.Add(target);
      }
    }

    CRefArray children = x3d.GetChildren();
    for(ULONG j=0;j<children.GetCount();j++)
      x3ds.Add(children[j]);
  }
  return spliceOps;
}


XSIPLUGINCALLBACK CStatus FabricSpliceBeginExport_OnEvent(CRef & ctxt)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceBeginExport_OnEvent");
  Context context(ctxt);
  if(LONG(context.GetAttribute("FileType")) != siFileTypeModel)
    return true;

  CRef modelRef = context.GetAttribute("Input");
  Model model(modelRef);

  CString fileName = context.GetAttribute("FileName");
  try
  {
    CRefArray spliceOps = getXSIOperatorsFromModel(model);

    std::vector<opUserData*> instances = getXSIOperatorInstances();
    for(size_t i=0;i<instances.size();i++)
    {
      if(instances[i]->getInterf() == NULL)
        continue;
      for(size_t j=0;j<spliceOps.GetCount();j++)
      {
        CustomOperator op(spliceOps[j]);
        if(instances[i]->getObjectID() == op.GetObjectID())
        {
          instances[i]->getInterf()->storePersistenceData(fileName);
          instances[i]->getInterf()->disconnectForExport(fileName, model);
        }
      }
    }
  }
  catch(FabricCore::Exception e)
  {
    xsiLogErrorFunc(e.getDesc_cstr());
  }
  return true;
}

XSIPLUGINCALLBACK CStatus FabricSpliceEndExport_OnEvent(CRef & ctxt)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceEndExport_OnEvent");
  Context context(ctxt);
  if(LONG(context.GetAttribute("FileType")) != siFileTypeModel)
    return true;

  CRef modelRef = context.GetAttribute("Input");
  Model model(modelRef);

  try
  {

    CRefArray spliceOps = getXSIOperatorsFromModel(model);

    std::vector<opUserData*> instances = getXSIOperatorInstances();
    for(size_t i=0;i<instances.size();i++)
    {
      if(instances[i]->getInterf() == NULL)
        continue;
      for(size_t j=0;j<spliceOps.GetCount();j++)
      {
        CustomOperator op(spliceOps[j]);
        if(instances[i]->getObjectID() == op.GetObjectID())
        {
          instances[i]->getInterf()->reconnectForImport(model);
        }
      }
    }

    FabricSpliceBaseInterface::cleanupForImport(model);
  }
  catch(FabricCore::Exception e)
  {
    xsiLogErrorFunc(e.getDesc_cstr());
  }
  return true;
}

CStatus onSaveScene(CRef & ctxt)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceSaveScene_OnEvent");

  Context context(ctxt);
  CString fileName = context.GetAttribute("FileName");

  std::vector<opUserData*> instances = getXSIOperatorInstances();
  for(size_t i=0;i<instances.size();i++)
  {
    if(instances[i]->getInterf() == NULL)
      continue;
    instances[i]->getInterf()->storePersistenceData(fileName);
  }
  return true;
}

XSIPLUGINCALLBACK CStatus FabricSpliceSaveScene_OnEvent(CRef & ctxt)
{ 
  return onSaveScene(ctxt);
}

XSIPLUGINCALLBACK CStatus FabricSpliceSaveAsScene_OnEvent(CRef & ctxt)
{
  return onSaveScene(ctxt);
}

XSIPLUGINCALLBACK CStatus FabricSpliceSaveScene2_OnEvent(CRef & ctxt)
{
  return onSaveScene(ctxt);
}

CString xsiGetKLKeyWords()
{
  CString result = "<<<";
  result += " >>>";
  result += " abs";
  result += " acos";
  result += " asin";
  result += " atan";
  result += " break";
  result += " const";
  result += " continue";
  result += " cos";
  result += " createArrayGenerator";
  result += " createConstValue";
  result += " createReduce";
  result += " createValueGenerator";
  result += " else";
  result += " for";
  result += " function";
  result += " if";
  result += " in";
  result += " io";
  result += " operator";
  result += " setError";
  result += " report";
  result += " return";
  result += " sin";
  result += " sqrt";
  result += " struct";
  result += " tan";
  result += " type";
  result += " use";
  result += " require";
  result += " var";
  result += " true";
  result += " false";
  result += " ValueProducer";
  result += " while";
  result += " Boolean";
  result += " Byte";
  result += " Integer";
  result += " Size";
  result += " Index";
  result += " SInt32";
  result += " SInt64";
  result += " UInt64";
  result += " Scalar";
  result += " Float32";
  result += " Float64";
  result += " String";
  result += " Vec2";
  result += " Vec3";
  result += " Vec4";
  result += " Quat";
  result += " Euler";
  result += " RotationOrder";
  result += " Mat22";
  result += " Mat33";
  result += " Mat44";
  result += " Xfo";
  result += " Math";
  result += " Color";
  result += " RGB";
  result += " RGBA";
  result += " BoundingBox3";
  result += " Ray";
  result += " Keyframe";
  result += " KeyframeTrack";
  result += " BezierXfo";
  result += " Points";
  result += " Lines";
  result += " PolygonMesh";
  result += " GeometryAttributes";
  result += " GeometryAttribute";
  result += " ScalarAttribute";
  result += " Vec2Attribute";
  result += " Vec3Attribute";
  result += " Vec4Attribute";
  result += " RGBAttribute";
  result += " RGBAAttribute";
  result += " ColorAttribute";
  result += " GeometryLocation";
  result += " DrawContext";
  result += " InlineDrawing";
  result += " InlineInstance";
  result += " InlineMaterial";
  result += " InlineShader";
  result += " InlineShape";
  result += " InlineTransform";
  result += " InlineUniform";
  result += " InlineTexture";
  result += " InlineFileBaseTexture";
  result += " InlineProceduralTexture";
  result += " InlineMatrixArrayTexture";
  result += " InlineDebugShape";
  result += " SimpleInlineInstance";
  result += " StaticInlineTransform";
  result += " InlineLinesShape";
  result += " InlineLinesShader";
  result += " InlineMeshShape";
  result += " OGLFlatShader";
  result += " OGLFlatVertexColorShader";
  result += " OGLFlatTextureShader";
  result += " OGLInlineDrawing";
  result += " OGLInlineShader";
  result += " OGLNormalShader";
  result += " OGLPointsShape";
  result += " OGLSurfaceShader";
  result += " OGLSurfaceVertexColorShader";
  result += " OGLSurfaceTextureShader";
  result += " OGLSurfaceNormalMapShader";

  return result;
}
