
#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_pluginregistrar.h>
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
#include <xsi_graphicsequencer.h>
#include <xsi_graphicsequencercontext.h>
#include <xsi_x3dobject.h>
#include <xsi_camera.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_primitive.h>
#include <xsi_parameter.h>
#include <xsi_customproperty.h>
#include <vector>

#include "plugin.h"
#include "renderpass.h"
#include "tools.h"
#include "FabricSpliceBaseInterface.h"

using namespace XSI;

bool gRTRPassEnabled = true;
FabricCore::RTVal sDrawContext;
CRefArray gRefsToDelete;

bool isRTRPassEnabled()
{
  return gRTRPassEnabled;
}

void enableRTRPass(bool enable)
{
  gRTRPassEnabled = enable;
}

void destroyDrawContext()
{
  sDrawContext.invalidate();
}

FabricCore::RTVal & getDrawContext(int viewportWidth, int viewportHeight, XSI::Camera & camera)
{
  FabricSplice::Logging::AutoTimer("getDrawContext");

  if(!sDrawContext.isValid())
    sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");
  else if(sDrawContext.isNullObject())
    sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");

  //////////////////////////
  // Setup the viewport
  FabricCore::RTVal inlineViewport = FabricSplice::constructObjectRTVal("InlineViewport");
  {
    FabricCore::RTVal viewportDim = FabricSplice::constructRTVal("Vec2");
    viewportDim.setMember("x", FabricSplice::constructFloat64RTVal(viewportWidth));
    viewportDim.setMember("y", FabricSplice::constructFloat64RTVal(viewportHeight));
    inlineViewport.setMember("dimensions", viewportDim);
  }

  {
    FabricCore::RTVal inlineCamera = FabricSplice::constructObjectRTVal("InlineCamera");

    Primitive cameraPrim = camera.GetActivePrimitive();
    LONG projType = cameraPrim.GetParameterValue(L"proj");
    inlineCamera.setMember("isOrthographic", FabricSplice::constructBooleanRTVal(projType == 0));

    double xsiViewportAspect = double(viewportWidth) / double(viewportHeight);
    double cameraAspect = cameraPrim.GetParameterValue(L"aspect");
    
    if(projType == 0){ // orthographic projection
      double orthoheight = cameraPrim.GetParameterValue(L"orthoheight");
      inlineCamera.setMember("orthographicFrustumH", FabricSplice::constructFloat64RTVal(orthoheight));
    }
    else{ // Perspective projection.
      double fovX = MATH::DegreesToRadians(cameraPrim.GetParameterValue(L"fov"));
      double fovY;
      if(xsiViewportAspect < cameraAspect){
        // bars top and bottom
        fovY = atan( tan(fovX * 0.5) / xsiViewportAspect ) * 2.0;
      }
      else{
        // bars left and right
        fovY = atan( tan(fovX * 0.5) / cameraAspect ) * 2.0;
      }
      inlineCamera.setMember("fovY", FabricSplice::constructFloat64RTVal(fovY));
    }


    double near = cameraPrim.GetParameterValue(L"near");
    double far = cameraPrim.GetParameterValue(L"far");
    inlineCamera.setMember("nearDistance", FabricSplice::constructFloat64RTVal(near));
    inlineCamera.setMember("farDistance", FabricSplice::constructFloat64RTVal(far));

    FabricCore::RTVal cameraMat = inlineCamera.maybeGetMember("mat44");
    FabricCore::RTVal cameraMatData = cameraMat.callMethod("Data", "data", 0, 0);

    MATH::CTransformation xsiCameraXfo = camera.GetKinematics().GetGlobal().GetTransform();
    MATH::CMatrix4 xsiCameraMat = xsiCameraXfo.GetMatrix4();
    const double * xsiCameraMatDoubles = (const double*)&xsiCameraMat;

    float * cameraMatFloats = (float*)cameraMatData.getData();
    if(cameraMatFloats != NULL) {
      cameraMatFloats[0] = (float)xsiCameraMatDoubles[0];
      cameraMatFloats[1] = (float)xsiCameraMatDoubles[4];
      cameraMatFloats[2] = (float)xsiCameraMatDoubles[8];
      cameraMatFloats[3] = (float)xsiCameraMatDoubles[12];
      cameraMatFloats[4] = (float)xsiCameraMatDoubles[1];
      cameraMatFloats[5] = (float)xsiCameraMatDoubles[5];
      cameraMatFloats[6] = (float)xsiCameraMatDoubles[9];
      cameraMatFloats[7] = (float)xsiCameraMatDoubles[13];
      cameraMatFloats[8] = (float)xsiCameraMatDoubles[2];
      cameraMatFloats[9] = (float)xsiCameraMatDoubles[6];
      cameraMatFloats[10] = (float)xsiCameraMatDoubles[10];
      cameraMatFloats[11] = (float)xsiCameraMatDoubles[14];
      cameraMatFloats[12] = (float)xsiCameraMatDoubles[3];
      cameraMatFloats[13] = (float)xsiCameraMatDoubles[7];
      cameraMatFloats[14] = (float)xsiCameraMatDoubles[11];
      cameraMatFloats[15] = (float)xsiCameraMatDoubles[15];

      inlineCamera.setMember("mat44", cameraMat);
    }

    inlineViewport.setMember("camera", inlineCamera);
  }

  sDrawContext.setMember("viewport", inlineViewport);

  return sDrawContext;
}


XSIPLUGINCALLBACK void SpliceRenderPass_Init(CRef & in_ctxt, void ** in_pUserData)
{
  GraphicSequencerContext ctxt = in_ctxt;
  CGraphicSequencer sequencer = ctxt.GetGraphicSequencer();
  sequencer.RegisterViewportCallback(L"CPRenderCallback", 0, siPass, siOpenGL20, siAll, CStringArray());
}

XSIPLUGINCALLBACK void SpliceRenderPass_Execute(CRef & in_ctxt, void ** in_pUserData)
{
  if(gRefsToDelete.GetCount() > 0)
  {
    CValue returnVal;
    CValueArray args(1);
    args[0] = gRefsToDelete.GetAsText();
    gRefsToDelete.Clear();
    Application().ExecuteCommand(L"DeleteObj", args, returnVal);
  }

  if(!gRTRPassEnabled)
    return;

  try
  {
    if(!FabricSplice::SceneManagement::hasRenderableContent())
      return;
  }
  catch(FabricCore::Exception e)
  {
    xsiLogErrorFunc(e.getDesc_cstr());
  }
  catch(FabricSplice::Exception e)
  {
    xsiLogErrorFunc(e.what());
  }

  FabricSplice::Logging::AutoTimer("SpliceRenderPass_Execute");
  
  // check if we should render this or not
  GraphicSequencerContext ctxt(in_ctxt);
  CGraphicSequencer sequencer = ctxt.GetGraphicSequencer();

  UINT left, top, width, height;
  sequencer.GetViewportSize(left, top, width, height);
  Camera camera(sequencer.GetCamera());

  // draw all gizmos
  try
  {
    FabricSplice::SceneManagement::drawOpenGL(getDrawContext(width, height, camera));
  }
  catch(FabricCore::Exception e)
  {
    xsiLogErrorFunc(e.getDesc_cstr());
  }
  catch(FabricSplice::Exception e)
  {
    xsiLogErrorFunc(e.what());
  }
}

XSIPLUGINCALLBACK void SpliceRenderPass_Term(CRef & in_ctxt, void ** in_pUserData)
{
}

void pushRefToDelete(const XSI::CRef & ref)
{
  gRefsToDelete.Add(ref);
}
