
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

#include "FabricSplicePlugin.h"
#include "FabricSpliceRenderPass.h"
#include "FabricSpliceTools.h"
#include "FabricSpliceBaseInterface.h"


#ifdef _WIN32
# include <windows.h>
#endif

#include <GL/gl.h>

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

FabricCore::RTVal & getDrawContext(XSI::Camera & camera)
{
  FabricSplice::Logging::AutoTimer("getDrawContext");

  // A bug in the CGraphicSequencer means that the viewport dimensions returned by 
  // 'GetViewportSize' are corrupt in certain scenarios. We no longer rely on these values. 
  // OpenGL can always give us the correct viewport values.
  int viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  int viewportWidth = viewport[2];
  int viewportHeight = viewport[3];

  if(!sDrawContext.isValid())
    sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");
  else if(sDrawContext.isNullObject())
    sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");

  // sync the time
  sDrawContext.setMember("time", FabricSplice::constructFloat32RTVal(CTime().GetTime( CTime::Seconds )));

  //////////////////////////
  // Setup the viewport
  FabricCore::RTVal inlineViewport = sDrawContext.maybeGetMember("viewport");
  {
    std::vector<FabricCore::RTVal> args(3);
    args[0] = sDrawContext;
    args[1] = FabricSplice::constructFloat64RTVal(viewportWidth);
    args[2] = FabricSplice::constructFloat64RTVal(viewportHeight);
    inlineViewport.callMethod("", "resize", 3, &args[0]);
  }

  {
    FabricCore::RTVal inlineCamera = FabricSplice::constructObjectRTVal("InlineCamera");

    Primitive cameraPrim = camera.GetActivePrimitive();
    LONG projType = cameraPrim.GetParameterValue(L"proj");
    FabricCore::RTVal param = FabricSplice::constructBooleanRTVal(projType == 0);
    inlineCamera.callMethod("", "setOrthographic", 1, &param);

    double xsiViewportAspect = double(viewportWidth) / double(viewportHeight);
    
    if(projType == 0)
    {  
      // Orthographic projection
      double orthoheight = cameraPrim.GetParameterValue(L"orthoheight");
      param = FabricSplice::constructFloat64RTVal(orthoheight);
      inlineCamera.callMethod("", "setOrthographicFrustumHeight", 1, &param);
    }
    else
    {
      // Perspective projection.
      //
      // XSI returns always the camera FOV.  We have four cases:
      // If the camera FOV is horizontal and the viewport is tall then the camera FOV is equal to the rendering FOVX, so to compute FOVY we use the viewport aspect ratio.
      // If the camera FOV is horizontal and the viewport is wide then the camera FOV is less than the rendering FOVX.  However, using the camera aspect ratio for conversion converts to the rendering FOV
      // If the camera FOV is vertical and the viewport is tall then the camera FOV is less than the rendering FOVY.  We therefore need to multiply it by cameraAspect / viewportAspect via the tan/atan conversion
      // If the camera FOV is vertical and the viewport is wide then the camera FOV is equal to the rendering FOVY and we can use it directly.

      double cameraAspect = cameraPrim.GetParameterValue(L"aspect");
      double fovParam =
        MATH::DegreesToRadians( cameraPrim.GetParameterValue(L"fov") );
      LONG fovTypeParam = cameraPrim.GetParameterValue(L"fovtype");

      // char buf[1024];
      // sprintf( buf, "xsiViewportAspect=%lg cameraAspect=%lg fovParam=%lg fovTypeParam=%ld",
      //   xsiViewportAspect, cameraAspect, fovParam, fovTypeParam);
      // xsiLogErrorFunc(buf);

      double fovY;
      switch ( fovTypeParam )
      {
        case 1: // horizontal
        {
          if ( cameraAspect < xsiViewportAspect ) // wide
            fovY = atan2( tan(fovParam * 0.5), cameraAspect ) * 2.0;
          else // tall
            fovY = atan2( tan(fovParam * 0.5), xsiViewportAspect ) * 2.0;
        }
        break;

        case 0: // vertical
        {
          if ( cameraAspect < xsiViewportAspect ) // wide
            fovY = fovParam;
          else
            fovY = atan2( tan(fovParam * 0.5) * cameraAspect, xsiViewportAspect ) * 2.0;
        }
        break;

        default:
          xsiLogErrorFunc("Unexpected camera FOV type");
          break;
      }

      param = FabricSplice::constructFloat64RTVal(fovY);
      inlineCamera.callMethod("", "setFovY", 1, &param);
    }


    double nearDist = cameraPrim.GetParameterValue(L"near");
    double farDist = cameraPrim.GetParameterValue(L"far");
    param = FabricSplice::constructFloat64RTVal(nearDist);
    inlineCamera.callMethod("", "setNearDistance", 1, &param);
    param = FabricSplice::constructFloat64RTVal(farDist);
    inlineCamera.callMethod("", "setFarDistance", 1, &param);

    FabricCore::RTVal cameraMat = FabricSplice::constructRTVal("Mat44");
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

      inlineCamera.callMethod("", "setFromMat44", 1, &cameraMat);
    }
    inlineViewport.callMethod("", "setCamera", 1, &inlineCamera);
  }

  return sDrawContext;
}


XSIPLUGINCALLBACK void SpliceRenderPass_Init(CRef & in_ctxt, void ** in_pUserData)
{
  GraphicSequencerContext ctxt = in_ctxt;
  CGraphicSequencer sequencer = ctxt.GetGraphicSequencer();
  sequencer.RegisterViewportCallback(L"SpliceRenderPass", 0, siPass, siOpenGL20, siAll, CStringArray());
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
  Camera camera(sequencer.GetCamera());

  // draw all gizmos
  try
  {
    FabricSplice::SceneManagement::drawOpenGL(getDrawContext(camera));
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
