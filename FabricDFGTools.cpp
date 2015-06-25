#pragma warning (disable : 4127)  // conditional expression is constant
#pragma warning (disable : 4714)  // marked as __forceinline not inlined
#pragma warning (disable : 4244)  // conversion ... possible loss of data
#include <xsi_context.h>
#include <xsi_command.h>
#include <xsi_argument.h>
#include <xsi_uitoolkit.h>
#include <xsi_selection.h>
#include <xsi_customoperator.h>
#include <xsi_operatorcontext.h>
#include <xsi_comapihandler.h>
#include <xsi_port.h>
#include <xsi_outputport.h>
#include <xsi_portgroup.h>
#include <xsi_inputport.h>
#include <xsi_factory.h>
#include <xsi_status.h>
#include <xsi_parameter.h>
#include <xsi_x3dobject.h>
#include <xsi_application.h>
#include <xsi_doublearray.h>
#include <xsi_factory.h>
#include <xsi_geometryaccessor.h>
#include <xsi_iceattribute.h>
#include <xsi_iceattributedataarray.h>
#include <xsi_iceattributedataarray2D.h>
#include <xsi_kinematics.h>
#include <xsi_math.h>
#include <xsi_camera.h>
#include <xsi_light.h>
#include <xsi_model.h>
#include <xsi_pluginregistrar.h>
#include <xsi_point.h>
#include <xsi_polygonmesh.h>
#include <xsi_ppglayout.h>
#include <xsi_primitive.h>
#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_shader.h>
#include <xsi_transformation.h>
#include <xsi_utils.h>

#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"
#include "FabricDFGTools.h"

using namespace XSI;
using namespace XSI::MATH;

XSI::CStatus dfgTools::ExecuteCommand0(XSI::CString commandName)
{
  CValueArray args;
  CValue value;
  return Application().ExecuteCommand(commandName, args, value);
}
XSI::CStatus dfgTools::ExecuteCommand1(XSI::CString commandName, XSI::CValue arg1)
{
  CValueArray args;
  args.Add(arg1);
  CValue value;
  return Application().ExecuteCommand(commandName, args, value);
}
XSI::CStatus dfgTools::ExecuteCommand2(XSI::CString commandName, XSI::CValue arg1, XSI::CValue arg2)
{
  CValueArray args;
  args.Add(arg1);
  args.Add(arg2);
  CValue value;
  return Application().ExecuteCommand(commandName, args, value);
}
XSI::CStatus dfgTools::ExecuteCommand3(XSI::CString commandName, XSI::CValue arg1, XSI::CValue arg2, XSI::CValue arg3)
{
  CValueArray args;
  args.Add(arg1);
  args.Add(arg2);
  args.Add(arg3);
  CValue value;
  return Application().ExecuteCommand(commandName, args, value);
}
XSI::CStatus dfgTools::ExecuteCommand4(XSI::CString commandName, XSI::CValue arg1, XSI::CValue arg2, XSI::CValue arg3, XSI::CValue arg4)
{
  CValueArray args;
  args.Add(arg1);
  args.Add(arg2);
  args.Add(arg3);
  args.Add(arg4);
  CValue value;
  return Application().ExecuteCommand(commandName, args, value);
}

bool dfgTools::FileBrowserJSON(bool isSave, XSI::CString &out_filepath)
{
  // init output.
  out_filepath = L"";

  // open file browser.
  CComAPIHandler toolkit;
  toolkit.CreateInstance(L"XSI.UIToolkit");
  CString ext = ".dfg.json";

  CComAPIHandler filebrowser(toolkit.GetProperty(L"FileBrowser"));
  filebrowser.PutProperty(L"InitialDirectory", Application().GetActiveProject().GetPath());
  filebrowser.PutProperty(L"Filter", L"DFG Preset (*" + ext + L")|*" + ext + L"||");

  CValue returnVal;
  filebrowser.Call(isSave ? L"ShowSave" : L"ShowOpen", returnVal);
  CString uiFileName = filebrowser.GetProperty(L"FilePathName").GetAsText();
  if(uiFileName.IsEmpty())
    return false;

  // convert backslashes to slashes.
  for(ULONG i=0;i<uiFileName.Length();i++)
  {
    if(uiFileName.GetSubString(i, 1).IsEqualNoCase(L"\\"))
      out_filepath += L"/";
    else
      out_filepath += uiFileName[i];
  }

  // take care of possible double extension (e.g. "myProfile.dfg.json.dfg.json").
  ULONG dfg1 = out_filepath.ReverseFindString(ext, UINT_MAX);
  if (dfg1 != UINT_MAX)
  {
    ULONG dfg2 = out_filepath.ReverseFindString(ext, dfg1 - 1);
    if (dfg2 != UINT_MAX && dfg2 + ext.Length() == dfg1)
    {
      out_filepath = out_filepath.GetSubString(0, out_filepath.Length() - ext.Length());
    }
  }

  // done.
  return (out_filepath.Length() > 0);
}

int dfgTools::GetRefsAtOps(XSI::X3DObject &in_obj, XSI::CString &in_opName, XSI::CRefArray &out_refs)
{
  // init output.
  out_refs.Clear();

  // fill out_refs.
  if (in_obj.IsValid())
  {
    CRefArray refs = Application().FindObjects(siCustomOperatorID);
    for (int i=0;i<refs.GetCount();i++)
    {
      CustomOperator op(refs[i]);
      if (   op.GetType() == in_opName
          && op.GetParent3DObject().GetUniqueName() == in_obj.GetUniqueName())
      {
        out_refs.Add(refs[i]);
      }
    }
  }

  // done.
  return out_refs.GetCount();
}

bool dfgTools::GetOperatorPortMapping(XSI::CRef &in_op, std::vector<_portMapping> &out_pmap, XSI::CString &out_err)
{
  // init output.
  out_pmap.clear();
  out_err   = L"";

  // get the operator and its user data.
  if (!in_op.IsValid())
  { out_err = L"in_op is invalid.";
    return false; }
  CustomOperator op(in_op);
  if (!op.IsValid())
  { out_err = L"op is invalid.";
    return false; }
  _opUserData *pud = _opUserData::GetUserData(op.GetObjectID());
  if (!pud)
  { out_err = L"no user data found.";
    return false; }

  // check if the Fabric stuff is not valid.
  if (   !pud->GetBaseInterface()
      || !pud->GetBaseInterface()->getBinding().isValid())
  { out_err = L"failed to get base interface or graph.";
    return false; }

  // get the DFG ports.
  FabricCore::DFGExec exec = pud->GetBaseInterface()->getBinding().getExec();
  if (exec.getExecPortCount() == 0)
    return true;

  // get the op's port groups and ports.
  CRefArray opPortGroups  = op.GetPortGroups();
  CRefArray opPortsInput  = op.GetInputPorts();
  CRefArray opPortsOutput = op.GetOutputPorts();

  // fill out_pmap.
  for (int i=0;i<exec.getExecPortCount();i++)
  {
    // init port mapping.
    _portMapping pmap;

    // dfg port name.
    pmap.dfgPortName = exec.getExecPortName(i);

    // dfg port type (in/out)
    if      (exec.getExecPortType(i) == FabricCore::DFGPortType_In)   pmap.dfgPortType = DFG_PORT_TYPE_IN;
    else if (exec.getExecPortType(i) == FabricCore::DFGPortType_Out)  pmap.dfgPortType = DFG_PORT_TYPE_OUT;
    else                                                                continue;

    // dfg port data type (resolved type).
    pmap.dfgPortDataType = exec.getExecPortResolvedType(i);

    // mapping type.
    {
      // init the type with 'internal'.
      pmap.mapType = DFG_PORT_MAPTYPE_INTERNAL;

      // process input port.
      if (pmap.dfgPortType == DFG_PORT_TYPE_IN)
      {
        // check if the operator has a parameter with the same name as the port.
        Parameter p = op.GetParameter(pmap.dfgPortName);
        if (p.IsValid())
        {
          pmap.mapType = DFG_PORT_MAPTYPE_XSI_PARAMETER;
        }

        // check if the operator has a XSI input port group with the same name as the port.
        else
        {
          for (int j=0;j<opPortGroups.GetCount();j++)
          {
            PortGroup pg(opPortGroups[j]);
            if (   pg.IsValid()
                && pg.GetName() == pmap.dfgPortName)
            {
              // found one.
              pmap.mapType = DFG_PORT_MAPTYPE_XSI_PORT;

              // now look if there also is a connected input port of the same name.
              for (int k=0;k<opPortsInput.GetCount();k++)
              {
                InputPort port(opPortsInput[k]);
                if (   port.IsValid()
                    && port.GetName() == pmap.dfgPortName
                    && port.IsConnected())
                    pmap.mapTarget = port.GetTarget().GetAsText();
              }

              // done.
              break;
            }
          }
        }
      }

      // process output port.
      else if (pmap.dfgPortType == DFG_PORT_TYPE_OUT)
      {
        for (int j=0;j<opPortGroups.GetCount();j++)
        {
          PortGroup pg(opPortGroups[j]);
          if (   pg.IsValid()
              && pg.GetName() == pmap.dfgPortName)
          {
            // found one.
            pmap.mapType = DFG_PORT_MAPTYPE_XSI_PORT;

            // now look if there also is a connected output port of the same name.
            for (int k=0;k<opPortsOutput.GetCount();k++)
            {
              OutputPort port(opPortsOutput[k]);
              if (   port.IsValid()
                  && port.GetName() == pmap.dfgPortName
                  && port.IsConnected())
                  pmap.mapTarget = port.GetTarget().GetAsText();
            }

            // done.
            break;
          }
        }
      }

      // unsupported port.
      else
      {
        continue;
      }

      // add it to out_pmap.
      out_pmap.push_back(pmap);
    }
  }

  // done.
  return true;
}

XSI::siClassID dfgTools::GetSiClassIdFromResolvedDataType(const XSI::CString &resDataType)
{
  if (   resDataType == L"Boolean"

      || resDataType == L"Float32"
      || resDataType == L"Float64"

      || resDataType == L"SInt8"
      || resDataType == L"SInt16"
      || resDataType == L"SInt32"
      || resDataType == L"SInt64"

      || resDataType == L"UInt8"
      || resDataType == L"UInt16"
      || resDataType == L"UInt32"
      || resDataType == L"UInt64")    return siParameterID;

  if (resDataType == L"Mat44")        return siKinematicStateID;

  if (resDataType == L"PolygonMesh")  return siPolygonMeshID;
  
  return siUnknownClassID;  // no match.
}

XSI::CString &dfgTools::GetSiClassIdDescription(const XSI::siClassID in_siClassID, XSI::CString &out_description)
{
  switch (in_siClassID)
  {
    case siUnknownClassID:                    out_description = L"Unknown object";                         break;
    case siSIObjectID:                        out_description = L"SIObject object";                        break;
    case siOGLMaterialID:                     out_description = L"OGLMaterial object";                     break;
    case siOGLTextureID:                      out_description = L"OGLTexture object";                      break;
    case siOGLLightID:                        out_description = L"OGLLight object";                        break;
    case siApplicationID:                     out_description = L"Application object";                     break;
    case siArgumentID:                        out_description = L"Argument object";                        break;
    case siCommandID:                         out_description = L"Command object";                         break;
    case siClipID:                            out_description = L"Clip object";                            break;
    case siShapeClipID:                       out_description = L"ShapeClip object";                       break;
    case siSubComponentID:                    out_description = L"SubComponent object";                    break;
    case siFacetID:                           out_description = L"Facet object";                           break;
    case siNurbsSurfaceID:                    out_description = L"NurbsSurfaceFace object";                break;
    case siPointID:                           out_description = L"Point object";                           break;
    case siControlPointID:                    out_description = L"ControlPoint object";                    break;
    case siNurbsCurveControlPointID:          out_description = L"NurbsCurveControlPoint object";          break;
    case siNurbsCurveListControlPointID:      out_description = L"NurbsCurveListControlPoint object";      break;
    case siNurbsSurfaceControlPointID:        out_description = L"NurbsSurfaceControlPoint object";        break;
    case siNurbsCurveID:                      out_description = L"NurbsCurve object";                      break;
    case siSampleID:                          out_description = L"Sample object";                          break;
    case siNurbsSampleID:                     out_description = L"NurbsSample object";                     break;
    case siPolygonNodeID:                     out_description = L"PolygonNode object";                     break;
    case siGeometryID:                        out_description = L"Geometry object";                        break;
    case siNurbsSurfaceMeshID:                out_description = L"NurbsSurfaceMesh object";                break;
    case siNurbsCurveListID:                  out_description = L"NurbsCurveList object";                  break;
    case siPolygonMeshID:                     out_description = L"PolygonMesh object";                     break;
    case siSegmentID:                         out_description = L"Segment object";                         break;
    case siConnectionPointID:                 out_description = L"ConnectionPoint object";                 break;
    case siConstructionHistoryID:             out_description = L"ConstructionHistory object";             break;
    case siDictionaryID:                      out_description = L"Dictionary object";                      break;
    case siEventInfoID:                       out_description = L"EventInfo object";                       break;
    case siFCurveID:                          out_description = L"FCurve object";                          break;
    case siNestedFCurveID:                    out_description = L"NestedFCurve object";                    break;
    case siFCurveKeyID:                       out_description = L"FCurveKey object";                       break;
    case siFileBrowserID:                     out_description = L"FileBrowser object";                     break;
    case siImageID:                           out_description = L"Image object";                           break;
    case siMappedItemID:                      out_description = L"MappedItem object";                      break;
    case siPortID:                            out_description = L"Port object";                            break;
    case siInputPortID:                       out_description = L"InputPort object";                       break;
    case siSelectionID:                       out_description = L"Selection object";                       break;
    case siStaticSourceID:                    out_description = L"StaticSource object";                    break;
    case siTriangleID:                        out_description = L"Triangle object";                        break;
    case siTriangleVertexID:                  out_description = L"TriangleVertex object";                  break;
    case siUpdateContextID:                   out_description = L"UpdateContext object";                   break;
    case siParameterID:                       out_description = L"Parameter object";                       break;
    case siCompoundParameterID:               out_description = L"CompoundParameter object";               break;
    case siProjectID:                         out_description = L"Project object";                         break;
    case siProjectItemID:                     out_description = L"ProjectItem object";                     break;
    case siActionSourceID:                    out_description = L"ActionSource object";                    break;
    case siExpressionID:                      out_description = L"Expression object";                      break;
    case siImageClipID:                       out_description = L"ImageClip object";                       break;
    case siSceneID:                           out_description = L"Scene object";                           break;
    case siShaderID:                          out_description = L"Shader object";                          break;
    case siOperatorID:                        out_description = L"Operator object";                        break;
    case siEnvelopeID:                        out_description = L"Envelope object";                        break;
    case siPrimitiveID:                       out_description = L"Primitive object";                       break;
    case siParticleCloudPrimitiveID:          out_description = L"ParticleCloudPrimitive object";          break;
    case siPropertyID:                        out_description = L"Property object";                        break;
    case siClusterPropertyID:                 out_description = L"ClusterProperty object";                 break;
    case siConstraintID:                      out_description = L"Constraint object";                      break;
    case siConstraintWithUpVectorID:          out_description = L"ConstraintWithUpVector object";          break;
    case siCustomPropertyID:                  out_description = L"CustomProperty object";                  break;
    case siJointID:                           out_description = L"Joint object";                           break;
    case siKinematicsID:                      out_description = L"Kinematics object";                      break;
    case siMaterialID:                        out_description = L"Material object";                        break;
    case siStaticKinematicStateID:            out_description = L"StaticKinematicState object";            break;
    case siSceneItemID:                       out_description = L"SceneItem object";                       break;
    case siClusterID:                         out_description = L"Cluster object";                         break;
    case siGroupID:                           out_description = L"Group object";                           break;
    case siLayerID:                           out_description = L"Layer object";                           break;
    case siPassID:                            out_description = L"Pass object";                            break;
    case siUserGroupID:                       out_description = L"UserGroup object";                       break;
    case siX3DObjectID:                       out_description = L"X3DObject object";                       break;
    case siParticleCloudID:                   out_description = L"ParticleCloud object";                   break;
    case siModelID:                           out_description = L"Model object";                           break;
    case siChainElementID:                    out_description = L"ChainElement object";                    break;
    case siChainRootID:                       out_description = L"ChainRoot object";                       break;
    case siChainBoneID:                       out_description = L"ChainBone object";                       break;
    case siChainEffectorID:                   out_description = L"ChainEffector object";                   break;
    case siDirectedID:                        out_description = L"Directed object";                        break;
    case siCameraID:                          out_description = L"Camera object";                          break;
    case siLightID:                           out_description = L"Light object";                           break;
    case siNullID:                            out_description = L"Null object";                            break;
    case siRigID:                             out_description = L"Rig object";                             break;
    case siCameraRigID:                       out_description = L"CameraRig object";                       break;
    case siLightRigID:                        out_description = L"LightRigobject";                         break;
    case siKinematicStateID:                  out_description = L"KinematicState object";                  break;
    case siTrackID:                           out_description = L"Track object";                           break;
    case siUserDataMapID:                     out_description = L"UserDataMap object";                     break;
    case siFxTreeID:                          out_description = L"FxTree object";                          break;
    case siFxOperatorID:                      out_description = L"FxOperator object";                      break;
    case siTriangleCollectionID:              out_description = L"TriangleCollection object";              break;
    case siTriangleVertexCollectionID:        out_description = L"TriangleVertexCollection object";        break;
    case siSampleCollectionID:                out_description = L"SampleCollection object";                break;
    case siPolygonNodeCollectionID:           out_description = L"PolygonNodeCollection object";           break;
    case siPointCollectionID:                 out_description = L"PointCollection object";                 break;
    case siFacetCollectionID:                 out_description = L"FacetCollection object";                 break;
    case siVertexID:                          out_description = L"Vertex object";                          break;
    case siVertexCollectionID:                out_description = L"VertexCollection object";                break;
    case siEdgeID:                            out_description = L"Edge object";                            break;
    case siEdgeCollectionID:                  out_description = L"EdgeCollection object";                  break;
    case siPolygonFaceID:                     out_description = L"PolygonFace object";                     break;
    case siPolygonFaceCollectionID:           out_description = L"PolygonFaceCollection object";           break;
    case siDataSourceID:                      out_description = L"DataSource object";                      break;
    case siAnimationSourceID:                 out_description = L"AnimationSource object";                 break;
    case siOutputPortID:                      out_description = L"OuptputPort object";                     break;
    case siProxyParameterID:                  out_description = L"Proxy Parameter object";                 break;
    case siDeviceCollectionID:                out_description = L"Device collection object";               break;
    case siDeviceID:                          out_description = L"Device object";                          break;
    case siChannelID:                         out_description = L"Channel object";                         break;
    case siKnotCollectionID:                  out_description = L"KnotCollection object";                  break;
    case siControlPointCollectionID:          out_description = L"ControlPointCollection object";          break;
    case siNurbsCurveCollectionID:            out_description = L"NurbsCurveCollection object";            break;
    case siNurbsSurfaceCollectionID:          out_description = L"NurbsSurfaceCollection object";          break;
    case siNurbsSampleCollectionID:           out_description = L"NurbsSampleCollection object";           break;
    case siTextureID:                         out_description = L"Texture object";                         break;
    case siUserDataBlobID:                    out_description = L"UserDataBlob object";                    break;
    case siParticleID:                        out_description = L"Particle object";                        break;
    case siAddonID:                           out_description = L"Addon object";                           break;
    case siPPGLayoutID:                       out_description = L"PPGLayout object";                       break;
    case siPPGItemID:                         out_description = L"PPGItem object";                         break;
    case siPreferencesID:                     out_description = L"Preferences object";                     break;
    case siParticleTypeID:                    out_description = L"ParticleType object";                    break;
    case siParticleAttributeID:               out_description = L"ParticleAttribute object";               break;
    case siGridDataID:                        out_description = L"GridData object";                        break;
    case siTextureLayerID:                    out_description = L"TextureLayer object";                    break;
    case siTextureLayerPortID:                out_description = L"TextureLayerPort object";                break;
    case siCustomOperatorID:                  out_description = L"CustomOperator object";                  break;
    case siPortGroupID:                       out_description = L"PortGroup object";                       break;
    case siDesktopID:                         out_description = L"Desktop object";                         break;
    case siLayoutID:                          out_description = L"Layout object";                          break;
    case siUIObjectID:                        out_description = L"UIObject object";                        break;
    case siUIPersistableID:                   out_description = L"UIPersistable object";                   break;
    case siViewID:                            out_description = L"View object";                            break;
    case siArrayParameterID:                  out_description = L"ArrayParameter object";                  break;
    case siViewContextID:                     out_description = L"X3DObject object";                       break;
    case siContextID:                         out_description = L"Context object";                         break;
    case siPPGEventContextID:                 out_description = L"PPGEventContext";                        break;
    case siClipEffectID:                      out_description = L"ClipEffect object";                      break;
    case siClipEffectItemID:                  out_description = L"ClipEffectItem object";                  break;
    case siShapeKeyID:                        out_description = L"ShapeKey object";                        break;
    case siSourceID:                          out_description = L"Source object";                          break;
    case siTimeControlID:                     out_description = L"TimeControl object";                     break;
    case siTransitionID:                      out_description = L"Transition object";                      break;
    case siAnimationSourceItemID:             out_description = L"AnimationSourceItem object";             break;
    case siClipContainerID:                   out_description = L"ClipContainer object";                   break;
    case siArgumentHandlerID:                 out_description = L"Argument Handler object";                break;
    case siMenuID:                            out_description = L"Menu object";                            break;
    case siMenuItemID:                        out_description = L"MenuItem object";                        break;
    case siPluginID:                          out_description = L"Plugin object";                          break;
    case siPluginItemID:                      out_description = L"PluginItem object";                      break;
    case siPluginRegistrarID:                 out_description = L"PluginRegistrar object";                 break;
    case siFilterID:                          out_description = L"Filter object";                          break;
    case siUIToolkitID:                       out_description = L"UIToolkit object";                       break;
    case siProgressBarID:                     out_description = L"ProgressBar object";                     break;
    case siParamDefID:                        out_description = L"ParamDef object";                        break;
    case siFactoryID:                         out_description = L"Factory object";                         break;
    case siCommandCollectionID:               out_description = L"Command object";                         break;
    case siArgumentCollectionID:              out_description = L"Argument Collection";                    break;
    case siGraphicSequencerContextID:         out_description = L"Graphic Sequencer Core";                 break;
    case siClipRelationID:                    out_description = L"ClipRelation object";                    break;
    case siMixerID:                           out_description = L"Mixer object";                           break;
    case siLibraryID:                         out_description = L"Library";                                break;
    case siSimulationEnvironmentID:           out_description = L"SimulationEnvironment object";           break;
    case siGridWidgetID:                      out_description = L"GridWidget object";                      break;
    case siGeometryAccessorID:                out_description = L"Geometry accessor object";               break;
    case siEnvelopeWeightID:                  out_description = L"EnvelopeWeight property object";         break;
    case siMeshBuilderID:                     out_description = L"MeshBuilder object";                     break;
    case siFileReferenceID:                   out_description = L"X3DObject object";                       break;
    case siClusterPropertyBuilderID:          out_description = L"ClusterPropertyBuilder object";          break;
    case siMaterialLibraryID:                 out_description = L"MaterialLibrary";                        break;
    case siHairPrimitiveID:                   out_description = L"HairPrimitive object";                   break;
    case siRenderHairAccessorID:              out_description = L"RenderHairAccessor object";              break;
    case siPointLocatorDataID:                out_description = L"PointLocatorData object";                break;
    case siCollectionItemID:                  out_description = L"CollectionItem object";                  break;
    case siOperatorContextID:                 out_description = L"Operator context object";                break;
    case siPointCloudID:                      out_description = L"PointCloud object";                      break;
    case siRigidBodyAccessorID:               out_description = L"RigidBodyAccessor object";               break;
    case siRigidConstraintAccessorID:         out_description = L"RigidConstraintAccessor object";         break;
    case siDeltaID:                           out_description = L"Delta object";                           break;
    case siActionDeltaID:                     out_description = L"ActionDelta object";                     break;
    case siActionDeltaItemID:                 out_description = L"ActionDeltaItem object";                 break;
    case siTimerEventID:                      out_description = L"TimerEvent object";                      break;
    case siPassContainerID:                   out_description = L"PassContainer object";                   break;
    case siRenderChannelID:                   out_description = L"RenderChannel object";                   break;
    case siSceneRenderPropertyID:             out_description = L"SceneRenderProperty object";             break;
    case siFramebufferID:                     out_description = L"Framebuffer object";                     break;
    case siRendererContextID:                 out_description = L"RendererContext object";                 break;
    case siRendererID:                        out_description = L"Renderer object";                        break;
    case siICENodeID:                         out_description = L"ICENode object";                         break;
    case siICECompoundNodeID:                 out_description = L"ICECompoundNode object";                 break;
    case siICENodePortID:                     out_description = L"ICENodePort object";                     break;
    case siICETreeID:                         out_description = L"ICETree object";                         break;
    case siICENodeContainerID:                out_description = L"ICENodeContainer object";                break;
    case siICENodeInputPortID:                out_description = L"ICENodeInputPort object";                break;
    case siICENodeOutputPortID:               out_description = L"ICENodeOutputPort object";               break;
    case siICEDataProviderNodeID:             out_description = L"ICEDataProviderNode object";             break;
    case siICEDataModifierNodeID:             out_description = L"ICEDataModifierNode object";             break;
    case siICENodeDefID:                      out_description = L"ICENodeDef object";                      break;
    case siICENodeContextID:                  out_description = L"ICENodeContext object";                  break;
    case siICEAttributeID:                    out_description = L"ICEAttribute object";                    break;
    case siPartitionID:                       out_description = L"Partition object";                       break;
    case siOverrideID:                        out_description = L"Override object";                        break;
    case siHardwareShaderContextID:           out_description = L"Hardware Shader Context Object";         break;
    case siValueMapID:                        out_description = L"ValueMap object";                        break;
    case siShaderParamDefID:                  out_description = L"ShaderParamDef object";                  break;
    case siShaderParamDefOptionsID:           out_description = L"ShaderParamDefOptions object";           break;
    case siShaderballOptionsID:               out_description = L"ShaderballOptions object";               break;
    case siShaderParamDefContainerID:         out_description = L"ShaderParamDefContainer object";         break;
    case siMetaShaderRendererDefID:           out_description = L"MetaShaderRendererDef object";           break;
    case siShaderDefID:                       out_description = L"ShaderDef object";                       break;
    case siShaderStructParamDefID:            out_description = L"ShaderStructParamDef object";            break;
    case siShaderArrayParamDefID:             out_description = L"ShaderArrayParamDef object";             break;
    case siShaderParameterID:                 out_description = L"ShaderParameter object";                 break;
    case siShaderArrayItemParameterID:        out_description = L"ShaderArrayItemParameter object";        break;
    case siShaderArrayParameterID:            out_description = L"ShaderArrayParameter object";            break;
    case siShaderCompoundParameterID:         out_description = L"ShaderCompoundParameter object";         break;
    case siRenderTreeNodeID:                  out_description = L"RenderTreeNode object";                  break;
    case siShaderBaseID:                      out_description = L"ShaderBase object";                      break;
    case siShaderContainerID:                 out_description = L"ShaderContainer object";                 break;
    case siShaderCompoundID:                  out_description = L"ShaderCompound object";                  break;
    case siShaderCommentID:                   out_description = L"ShaderComment object";                   break;
    case siShaderDefManagerID:                out_description = L"ShaderDefManager object";                break;
    case siHardwareSurfaceID:                 out_description = L"HardwareSurface object";                 break;
    case siGraphicDriverID:                   out_description = L"Graphic Driver object";                  break;
    case siHairGeometryID:                    out_description = L"HairGeometry object";                    break;
    case siPointCloudGeometryID:              out_description = L"PointCloudGeometry object";              break;
    case siSchematicNodeID:                   out_description = L"SchematicNode object";                   break;
    case siSchematicID:                       out_description = L"Schematic object";                       break;
    case siSchematicNodeCollectionID:         out_description = L"SchematicNodeCollection object";         break;
    case siUVPropertyID:                      out_description = L"UVProperty property object";             break;
    case siToolContextID:                     out_description = L"ToolContext object";                     break;
    case siPickBufferID:                      out_description = L"PickBuffer object";                      break;
    case siMemoCameraID:                      out_description = L"MemoCamera object";                      break;
    case siMemoCameraCollectionID:            out_description = L"MemoCameraCollection object";            break;
    case siAnnotationID:                      out_description = L"Annotation object";                      break;
    case siCustomPrimitiveID:                 out_description = L"CustomPrimitive object";                 break;
    case siCustomPrimitiveContextID:          out_description = L"CustomPrimitiveContext object";          break;
    default:                                  out_description = L"";                                       break;
  }

  return out_description;
}

bool dfgTools::FileExists(const char *filePath)
{
  if (!filePath)  return false;
  FILE *stream = fopen(filePath, "rb");
  if (stream)
  {
    fclose(stream);
    return true;
  }
  return false;
}

bool dfgTools::GetGeometryFromX3DObject(const XSI::X3DObject &in_x3DObj, double in_currFrame, bool in_useGlobalSRT, XSI::CDoubleArray &out_vertexPositions, XSI::CLongArray &out_polyVIndices, XSI::CLongArray &out_polyVCount, LONG &out_numNodes, bool &inout_useVertMotions, XSI::CFloatArray &out_vertMotions, bool noWarnMotions, bool &inout_useNodeNormals, bool in_useNormals, XSI::CFloatArray &out_nodeNormals, bool noWarnNormals, bool &inout_useNodeUVWs, XSI::CFloatArray &out_nodeUVWs, bool noWarnUVWs, bool &inout_useNodeColors, XSI::CFloatArray &out_nodeColors, bool noWarnColors, XSI::CString &in_nameMotions, XSI::CString &in_nameNormals, XSI::CString &in_nameUVWs, XSI::CString &in_nameColors, XSI::CString &errmsg, XSI::CString &wrnmsg)
{
  // init.
  errmsg = L"";
  wrnmsg = L"";

  // invalid input?
  if (!in_x3DObj.IsValid())
  { errmsg = L"Input Geometry not found.";
    return false; }
      
  // get the current global transformation.
  CTransformation glbTransCurr( CTransformation(in_x3DObj.GetKinematics().GetGlobal().GetTransform(in_currFrame)) );

  // get the global transformation of the next frame (if inout_useVertMotions != 0).
  CTransformation glbTransNext;
  if (inout_useVertMotions)   glbTransNext = CTransformation(in_x3DObj.GetKinematics().GetGlobal().GetTransform(in_currFrame + 1));
  else                        glbTransNext = glbTransCurr;

  // get the polygon mesh.
  PolygonMesh pmesh = in_x3DObj.GetActivePrimitive(in_currFrame).GetGeometry(in_currFrame, siConstructionModeSecondaryShape);
  if (!pmesh.IsValid())
  { errmsg = L"GetActivePrimitive( ).GetGeometry( ) failed.";
    return false; }

  // get the geometry accessor.
  CGeometryAccessor ga = pmesh.GetGeometryAccessor(siConstructionModeSecondaryShape);
  if (!ga.IsValid())
  { errmsg = L"GetGeometryAccessor() failed.";
    return false; }

  // get the vertex local positions.
  if (ga.GetVertexPositions(out_vertexPositions) != CStatus::OK)
  { errmsg = L"GetVertexPositions() failed.";
    return false; }
  LONG numVertFlat  = out_vertexPositions.GetCount();
  LONG numVert      = numVertFlat / 3;

  // get the polygons' vertex indices and count.
  if (ga.GetVertexIndices(out_polyVIndices) != CStatus::OK)
  { errmsg = L"GetVertexIndices() failed.";
    return false; }
  if (ga.GetPolygonVerticesCount(out_polyVCount) != CStatus::OK)
  { errmsg = L"GetPolygonVerticesCount() failed.";
    return false; }

  // get the polygon node indices.
  CLongArray nodeIndices(0);
  if (ga.GetNodeIndices(nodeIndices) != CStatus::OK)
  { errmsg = L"GetNodeIndices() failed.";
    return false; }

  // set out_numNodes and check whether the sum of out_polyVCount is equal out_numNodes.
  out_numNodes = nodeIndices.GetCount();
  {
    LONG polyVCountSum = 0;
    LONG *nc = (LONG *)out_polyVCount.GetArray();
    for (LONG i=0;i<out_polyVCount.GetCount();i++,nc++)
      polyVCountSum += *nc;
    if (polyVCountSum != out_numNodes)
    { errmsg = L"polyVCountSum != out_numNodes.";
      return false; }
  }

  // get the motion vectors.
  if (inout_useVertMotions)
  {
    // set tmpName and attempt to get ICE attribute.
    ICEAttribute attrib;
    CString tmpName;
    if (in_nameMotions != L"")
    {
      tmpName = in_nameMotions;
      attrib  = pmesh.GetICEAttributeFromName(tmpName);
    }
    else
    {
      CRefArray ra = pmesh.GetICEAttributes();
      bool found = false;
      tmpName = L"viMotion";
      for (LONG i=0;i<ra.GetCount();i++)
      {
        found = (ra.GetAsText().ReverseFindString(tmpName) != ULONG_MAX);
        if (found)  break;
      }
      if (!found)
      {
        tmpName = L"PointUserMotions";
        for (LONG i=0;i<ra.GetCount();i++)
        {
          found = (ra.GetAsText().ReverseFindString(tmpName) != ULONG_MAX);
          if (found)  break;
        }
      }
      attrib = pmesh.GetICEAttributeFromName(tmpName);
    }

    // check/get data and possibly switch to "transform".
    bool useTransform = false;
    if (!attrib.IsDefined())
    {
      if (noWarnMotions)
      {
        useTransform = true;
      }
      else
      {
        if (in_nameMotions != L"")
        {
          errmsg = L"the vertex motions \"" + tmpName + L"\" could not be found/accessed.";
          return false;
        }
        else
        {
          useTransform = true;
          wrnmsg = L"the vertex motions \"" + tmpName + L"\" could not be found/accessed, switching to \"transformations only\"";
        }
      }
    }

    // resize out_vertMotions.
    if (out_vertMotions.Resize(numVertFlat) != CStatus::OK)
    { errmsg = L"out_vertMotions.Resize() failed.";
      return false; }

    // use the ICE data.
    if (!useTransform)
    {
      if (attrib.GetDataType() != siICENodeDataVector3)
      { errmsg = L"the motions \"" + tmpName + L"\" have the wrong data type (only 3D vector is supported).";
        return false; }
      if (attrib.GetContextType() != siICENodeContextComponent0D)
      { errmsg = L"the motions \"" + tmpName + L"\" have the wrong context type (only \"per Point\" is supported).";
        return false; }
      if (attrib.GetStructureType() == siICENodeStructureSingle)      // single value per Vertex.
      {
        // get ICE data.
        CICEAttributeDataArrayVector3f tmpMtn;
        if (attrib.GetDataArray(tmpMtn) != CStatus::OK)
        { errmsg = L"failed to get motion ICE data.";
          return false; }

        // fill out_vertMotions from tmpMtn.
        float *vm = (float *)out_vertMotions.GetArray();
        for (LONG i=0;i<numVert;i++,vm+=3)
          tmpMtn[i].Get(vm[0], vm[1], vm[2]);
      }
      else if (attrib.GetStructureType() == siICENodeStructureArray)    // array per Vertex.
      {
        // get ICE data.
        CICEAttributeDataArray2DVector3f tmpMtn2D;
        if (attrib.GetDataArray2D(tmpMtn2D) != CStatus::OK)
        { errmsg = L"failed to get motion ICE data.";
          return false; }

        // fill out_vertMotions from tmpMtn.
        CICEAttributeDataArrayVector3f sub;
        float *vm = (float *)out_vertMotions.GetArray();
        for (LONG i=0;i<numVert;i++,vm+=3)
        {
          if (tmpMtn2D.GetSubArray(i, sub) == CStatus::OK)
            if (sub.GetCount())
            { sub[sub.GetCount() - 1].Get(vm[0], vm[1], vm[2]);
              continue; }
          vm[0] = 0;
          vm[1] = 0;
          vm[2] = 0;
        }
      }
      else
      { errmsg = L"the motions \"" + tmpName + L"\" have an unsupported structure type.";
        return false; }

      // consider global SRT?
      if (in_useGlobalSRT)
      {
        CTransformation tmpT(glbTransCurr);
        tmpT.SetTranslationFromValues(0, 0, 0);
        CVector3 v;
        for (LONG i=0;i<numVertFlat;i+=3)
        {
          v.Set(out_vertMotions[i + 0],
                out_vertMotions[i + 1],
                out_vertMotions[i + 2]);
          v.MulByTransformationInPlace(tmpT);
          out_vertMotions[i + 0] = (float)v.GetX();
          out_vertMotions[i + 1] = (float)v.GetY();
          out_vertMotions[i + 2] = (float)v.GetZ();
        }
      }
    }

    // use the transformations only (= manually calculate the motions).
    else
    {
      CVector3 vTransCurr = glbTransCurr.GetTranslation();
      CVector3 vCurr, vNext;
      for (LONG i=0;i<numVertFlat;i+=3)
      {
        vCurr.Set(out_vertexPositions[i + 0],
                  out_vertexPositions[i + 1],
                  out_vertexPositions[i + 2]);
        vNext = vCurr;
        vCurr.MulByTransformationInPlace(glbTransCurr);
        vNext.MulByTransformationInPlace(glbTransNext);
        vNext.SubInPlace(vCurr);
        if (in_useGlobalSRT)  vCurr = vNext;
        else
        {
          vNext.AddInPlace(vTransCurr);
          vCurr = MapWorldPositionToObjectSpace(glbTransCurr, vNext);
        }
        out_vertMotions[i + 0] = (float)vCurr.GetX();
        out_vertMotions[i + 1] = (float)vCurr.GetY();
        out_vertMotions[i + 2] = (float)vCurr.GetZ();
      }
    }
  }

  // now that we are done with the local vertex positions:
  // consider global SRT?
  if (in_useGlobalSRT)
  {
    CVector3 v;
    for (LONG i=0;i<numVertFlat;i+=3)
    {
      v.Set(out_vertexPositions[i + 0],
            out_vertexPositions[i + 1],
            out_vertexPositions[i + 2]);
      v.MulByTransformationInPlace(glbTransCurr);
      v.Get(out_vertexPositions[i + 0],
            out_vertexPositions[i + 1],
            out_vertexPositions[i + 2]);
    }
  }

  // get the polygon node normals.
  // note: GetNodeNormals() crashes with emPolygonizer normals, that's why the below code
  //     gets the values "on foot" instead of using GetNodeNormals() directly.
  if (inout_useNodeNormals)
  {
    // get array of references at user normals.
    CRefArray userNormalsRefs = ga.GetUserNormals();
    if (userNormalsRefs.GetCount() <= 0)
    {
      // there are no user normals.
      if (in_useNormals)
      {
        // we just call GetNodeNormals().
        CFloatArray tmpNorms(0);
        if (ga.GetNodeNormals(tmpNorms) != CStatus::OK)
        { errmsg = L"GetNodeNormals() failed.";
          return false; }

        // resize out_nodeNormals.
        if (out_nodeNormals.Resize(out_numNodes * 3L) != CStatus::OK)
        { errmsg = L"out_nodeNormals.Resize() failed.";
          return false; }

        // fill out_nodeNormals from tmpNorms.
        LONG  *ni = (LONG *)nodeIndices.GetArray();
        float *nn = (float *)out_nodeNormals.GetArray();
        for (LONG i=0;i<out_numNodes;i++,ni++,nn+=3)
        {
          nn[0] = tmpNorms[(*ni) * 3L + 0];
          nn[1] = tmpNorms[(*ni) * 3L + 1];
          nn[2] = tmpNorms[(*ni) * 3L + 2];
        }
      }
      else
        inout_useNodeNormals = false;
    }
    else
    {
      // there are user normals available... we simply take the first user normals in the ref array.
      ClusterProperty clusterProp(userNormalsRefs[0]);
      if (!clusterProp.IsValid())
      { errmsg = L"ClusterProperty clusterProp(userNormalsRefs[0]) failed.";
        return false; }

      // get the cluster property element array.
      CClusterPropertyElementArray clusterPropElements = clusterProp.GetElements();

      // resize out_nodeNormals.
      if (out_nodeNormals.Resize(out_numNodes * 3L) != CStatus::OK)
      { errmsg = L"out_nodeNormals.Resize() failed.";
        return false; }

      // fill out_nodeNormals "on foot".
      LONG  *ni = (LONG *)nodeIndices.GetArray();
      float *nn = (float *)out_nodeNormals.GetArray();
      for (LONG i=0;i<out_numNodes;i++,ni++,nn+=3)
      {
        CDoubleArray tmp = clusterPropElements.GetItem(*ni);
        nn[0] = (float)tmp[0];
        nn[1] = (float)tmp[1];
        nn[2] = (float)tmp[2];
      }
    }

    // consider global SRT for the normals?
    if (in_useGlobalSRT)
    {
      CTransformation tmpT(glbTransCurr);
      tmpT.SetTranslationFromValues(0, 0, 0);
      LONG numNormalsFlat = out_nodeNormals.GetCount();
      CVector3 v;
      for (LONG i=0;i<numNormalsFlat;i+=3)
      {
        v.Set(out_nodeNormals[i + 0],
            out_nodeNormals[i + 1],
            out_nodeNormals[i + 2]);
        v.MulByTransformationInPlace(tmpT);
        v.NormalizeInPlace();
        out_nodeNormals[i + 0] = (float)v.GetX();
        out_nodeNormals[i + 1] = (float)v.GetY();
        out_nodeNormals[i + 2] = (float)v.GetZ();
      }
    }
  }

  // get the node UVWs.
  if (inout_useNodeUVWs)
  {
    // get refs at UVs and look for the UVs specified by in_nameUVWs.
    CRefArray refsUVs = ga.GetUVs();
    LONG ruvIdx = -1;
    for (LONG i=0;i<refsUVs.GetCount();i++)
    {
      CStringArray r = refsUVs[i].GetAsText().Split(L".");
      if (    in_nameUVWs == L""
          || (r.GetCount() > 0 && r[r.GetCount() - 1] == in_nameUVWs)  )
      {
        ruvIdx = i;
        break;
      }
    }

    // did we find some UVs?
    if (ruvIdx >= 0)
    {
      ClusterProperty cProp(refsUVs[ruvIdx]);
      if (   cProp.IsValid()
          && cProp.GetPropertyType() == siClusterPropertyUVType
          && cProp.GetValueSize() == 3  )
      {
        // get UVWs.
        CFloatArray tmpUVWs(0);
        if (cProp.GetValues(tmpUVWs) != CStatus::OK)
        { errmsg = L"cProp.GetValues(tmpUVWs) failed.";
          return false; }

        // resize out_nodeUVWs.
        if (out_nodeUVWs.Resize(out_numNodes * 3L) != CStatus::OK)
        { errmsg = L"out_nodeUVWs.Resize() failed.";
          return false; }

        // fill out_nodeUVWs from tmpUVWs.
        LONG  *ni = (LONG *)nodeIndices.GetArray();
        float *nu = (float *)out_nodeUVWs.GetArray();
        for (LONG i=0;i<out_numNodes;i++,ni++,nu+=3)
        {
          nu[0] = tmpUVWs[(*ni) * 3L + 0];
          nu[1] = tmpUVWs[(*ni) * 3L + 1];
          nu[2] = tmpUVWs[(*ni) * 3L + 2];
        }
      }
      else
      { errmsg = cProp.GetFullName() + L" has an unsupported property type or value size.";
        return false; }
    }

    // we didn't find any UVs, so we check if there are any ICE UVs.
    else
    {
      // set tmpName and get attribute.
      ICEAttribute attrib;
      CString tmpName;
      if (in_nameUVWs != L"")
      {
        tmpName = in_nameUVWs;
        attrib = pmesh.GetICEAttributeFromName(tmpName);
      }
      else
      {
        tmpName = L"viUVW";
        attrib = pmesh.GetICEAttributeFromName(tmpName);
      }

      // check/get data.
      if (attrib.IsDefined() || !noWarnUVWs)
      {
        if (!attrib.IsDefined())
        { errmsg = L"the UVWs \"" + tmpName + L"\" could not be found/accessed.";
          return false; }
        if (attrib.GetDataType() != siICENodeDataVector3)
        { errmsg = L"the UVWs \"" + tmpName + L"\" have the wrong data type (only 3D vector is supported).";
          return false; }
        if (attrib.GetStructureType() != siICENodeStructureSingle)
        { errmsg = L"the UVWs \"" + tmpName + L"\" have the wrong structure type.";
          return false; }
        if    (attrib.GetContextType() == siICENodeContextComponent0D)    // per vertex.
        {
          // get ICE data.
          CICEAttributeDataArrayVector3f tmpUVWs;
          if (attrib.GetDataArray(tmpUVWs) != CStatus::OK)
          { errmsg = L"failed to get UVW ICE data.";
            return false; }
          // resize out_nodeUVWs.
          if (out_nodeUVWs.Resize(out_numNodes * 3L) != CStatus::OK)
          { errmsg = L"out_nodeUVWs.Resize() failed.";
            return false; }
          // fill out_nodeUVWs from tmpUVWs.
          LONG  *ni = (LONG *)out_polyVIndices.GetArray();
          float *nu = (float *)out_nodeUVWs.GetArray();
          for (LONG i=0;i<out_numNodes;i++,ni++,nu+=3)
            tmpUVWs[*ni].Get(nu[0], nu[1], nu[2]);
        }
      else if (attrib.GetContextType() == siICENodeContextComponent0D2D)    // per node.
      {
        // get ICE data.
        CICEAttributeDataArrayVector3f tmpUVWs;
        if (attrib.GetDataArray(tmpUVWs) != CStatus::OK)
        { errmsg = L"failed to get UVW ICE data.";
          return false; }
        // resize out_nodeUVWs.
        if (out_nodeUVWs.Resize(out_numNodes * 3L) != CStatus::OK)
        { errmsg = L"out_nodeUVWs.Resize() failed.";
          return false; }
        // fill out_nodeUVWs from tmpUVWs.
        LONG  *ni = (LONG *)nodeIndices.GetArray();
        float *nu = (float *)out_nodeUVWs.GetArray();
        for (LONG i=0;i<out_numNodes;i++,ni++,nu+=3)
          tmpUVWs[*ni].Get(nu[0], nu[1], nu[2]);
      }
        else
        { errmsg = L"the UVWs \"" + tmpName + L"\" have the wrong context type (only \"per Point\" and \"per Sample\" are supported).";
          return false; }
      }
    }
  }

  // get the node colors.
  if (inout_useNodeColors)
  {
    // get refs at vertex colors and look for the vertex colors specified by in_nameColors.
    CRefArray refsColors = ga.GetVertexColors();
    LONG rcolIdx = -1;
    for (LONG i=0;i<refsColors.GetCount();i++)
    {
      CStringArray r = refsColors[i].GetAsText().Split(L".");
      if (    in_nameColors == L""
          || (r.GetCount() > 0 && r[r.GetCount() - 1] == in_nameColors)  )
      {
        rcolIdx = i;
        break;
      }
    }

    // did we find some vertex colors?
    if (rcolIdx >= 0)
    {
      ClusterProperty cProp(refsColors[rcolIdx]);
      if (   cProp.IsValid()
          && cProp.GetPropertyType() == siClusterPropertyVertexColorType  
          && cProp.GetValueSize() == 4  )
      {
        // get colors.
        CFloatArray tmpColors(0);
        if (cProp.GetValues(tmpColors) != CStatus::OK)
        { errmsg = L"cProp.GetValues(tmpColors) failed.";
          return false; }

        // resize out_nodeColors.
        if (out_nodeColors.Resize(out_numNodes * 4L) != CStatus::OK)
        { errmsg = L"out_nodeColors.Resize() failed.";
          return false; }

        // fill out_nodeColors from tmpColors.
        LONG  *ni = (LONG *)nodeIndices.GetArray();
        float *nu = (float *)out_nodeColors.GetArray();
        for (LONG i=0;i<out_numNodes;i++,ni++,nu+=4)
        {
          nu[0] = tmpColors[(*ni) * 4L + 0];
          nu[1] = tmpColors[(*ni) * 4L + 1];
          nu[2] = tmpColors[(*ni) * 4L + 2];
          nu[3] = tmpColors[(*ni) * 4L + 3];
        }
      }
      else
      { errmsg = cProp.GetFullName() + L" has an unsupported property type or value size.";
        return false; }
    }

    // we didn't find any vertex colors, so we check if there are any ICE colors that we can use.
    else
    {
      // set tmpName and get attribute.
      ICEAttribute attrib;
      CString tmpName;
      if (in_nameColors != L"")
      {
        tmpName = in_nameColors;
        attrib = pmesh.GetICEAttributeFromName(tmpName);
      }
      else
      {
        tmpName = L"viColor";
        attrib = pmesh.GetICEAttributeFromName(tmpName);
        if (attrib.GetName() != tmpName)
        {
          tmpName = L"Color";
          attrib = pmesh.GetICEAttributeFromName(tmpName);
        }
      }

      // check/get data.
      if (attrib.IsDefined() || !noWarnColors)
      {
        if (!attrib.IsDefined())
        { errmsg = L"the colors \"" + tmpName + L"\" could not be found/accessed.";
          return false; }
        if (attrib.GetDataType() != siICENodeDataColor4)
        { errmsg = L"the colors \"" + tmpName + L"\" have the wrong data type (only color is supported).";
          return false; }
        if (attrib.GetStructureType() != siICENodeStructureSingle)
        { errmsg = L"the colors \"" + tmpName + L"\" have the wrong structure type.";
          return false; }
        if      (attrib.GetContextType() == siICENodeContextComponent0D)    // per vertex.
        {
          // get ICE data.
          CICEAttributeDataArrayColor4f tmpCols;
          if (attrib.GetDataArray(tmpCols) != CStatus::OK)
          { errmsg = L"failed to get color ICE data.";
            return false; }
          // resize out_nodeColors.
          if (out_nodeColors.Resize(out_numNodes * 4L) != CStatus::OK)
          { errmsg = L"out_nodeColors.Resize() failed.";
            return false; }
          // fill out_nodeColors from tmpCols.
          LONG  *ni = (LONG *)out_polyVIndices.GetArray();
          float *nc = (float *)out_nodeColors.GetArray();
          for (LONG i=0;i<out_numNodes;i++,ni++,nc+=4)
            tmpCols[*ni].GetAsRGBA(nc[0], nc[1], nc[2], nc[3]);
        }
        else if (attrib.GetContextType() == siICENodeContextComponent0D2D)    // per node.
        {
          // get ICE data.
          CICEAttributeDataArrayColor4f tmpCols;
          if (attrib.GetDataArray(tmpCols) != CStatus::OK)
          { errmsg = L"failed to get color ICE data.";
            return false; }
          // resize out_nodeColors.
          if (out_nodeColors.Resize(out_numNodes * 4L) != CStatus::OK)
          { errmsg = L"out_nodeColors.Resize() failed.";
            return false; }
          // fill out_nodeColors from tmpCols.
          LONG  *ni = (LONG *)nodeIndices.GetArray();
          float *nc = (float *)out_nodeColors.GetArray();
          for (LONG i=0;i<out_numNodes;i++,ni++,nc+=4)
            tmpCols[*ni].GetAsRGBA(nc[0], nc[1], nc[2], nc[3]);
        }
        else
        { errmsg = L"the colors \"" + tmpName + L"\" have the wrong context type (only \"per Point\" and \"per Sample\" are supported).";
          return false; }
      }
    }
  }

  // done.
  return true;
}

bool dfgTools::GetGeometryFromX3DObject(const XSI::X3DObject &in_x3DObj, double in_currFrame, _polymesh &out_polymesh, XSI::CString &out_errmsg, XSI::CString &out_wrnmsg)
{
  // init.
  out_polymesh.clear();

  // get geo as flat arrays.
  CDoubleArray  vertexPositions  (0);
  CLongArray    polyVIndices     (0);
  CLongArray    polyVCount       (0);
  LONG          numNodes        = 0;
  bool          useVertMotions  = false;
  bool          useNodeNormals  = true;
  bool          useNodeUVWs     = false;
  bool          useNodeColors   = false;
  CFloatArray   vertMotions      (0);
  CFloatArray   nodeNormals      (0);
  CFloatArray   nodeUVWs         (0);
  CFloatArray   nodeColors       (0);
  if (!GetGeometryFromX3DObject(in_x3DObj,
                                in_currFrame,
                                false,
                                vertexPositions,
                                polyVIndices,
                                polyVCount,
                                numNodes,
                                useVertMotions,       vertMotions, true,
                                useNodeNormals, true, nodeNormals, true,
                                useNodeUVWs,          nodeUVWs,    true,
                                useNodeColors,        nodeColors,  true,
                                CString(),
                                CString(),
                                CString(),
                                CString(),
                                out_errmsg,
                                out_wrnmsg  ) )
  {
    return false;
  }

  if (!useVertMotions)  vertMotions .Clear();
  if (!useNodeNormals)  nodeNormals .Clear();
  if (!useNodeUVWs)     nodeUVWs    .Clear();
  if (!useNodeColors)   nodeColors  .Clear();

  // set out_polymesh from arrays.
  int ret = out_polymesh.SetFromFlatArrays(                      vertexPositions.GetArray(),  vertexPositions.GetCount(),
                                                                 nodeNormals    .GetArray(),  nodeNormals    .GetCount(),
                                           (const unsigned int *)polyVCount     .GetArray(),  polyVCount     .GetCount(),
                                           (const unsigned int *)polyVIndices   .GetArray(),  polyVIndices   .GetCount()
                                          );
  if (ret)
  {
    out_polymesh.clear();
    switch (ret)
    {
      case -1:
        out_errmsg = L"out_polymesh.SetFromFlatArrays() returned " + CString((LONG)ret) + L" (\"pointer is NULL\")";
        return false;
      case -2:
        out_errmsg = L"out_polymesh.SetFromFlatArrays() returned " + CString((LONG)ret) + L" (\"illegal array size\")";
        return false;
      case -3:
        out_errmsg = L"out_polymesh.SetFromFlatArrays() returned " + CString((LONG)ret) + L" (\"memory error\")";
        return false;
      default:
        out_errmsg = L"out_polymesh.SetFromFlatArrays() returned " + CString((LONG)ret) + L" (\"unknown error\")";
        return false;
    }
    return false;
  }

  // done.
  return true;
}


