
#include "FabricSpliceBaseInterface.h"
#include "plugin.h"
#include "operators.h"
#include "commands.h"
#include "dialogs.h"

#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <xsi_application.h>
#include <xsi_factory.h>
#include <xsi_parameter.h>
#include <xsi_customoperator.h>
#include <xsi_port.h>
#include <xsi_portgroup.h>
#include <xsi_outputport.h>
#include <xsi_vector3.h>
#include <xsi_matrix4f.h>
#include <xsi_matrix4.h>
#include <xsi_transformation.h>
#include <xsi_kinematicstate.h>
#include <xsi_primitive.h>
#include <xsi_geometry.h>
#include <xsi_polygonmesh.h>
#include <xsi_nurbscurve.h>
#include <xsi_nurbscurvelist.h>
#include <xsi_nurbsdata.h>
#include <xsi_knot.h>
#include <xsi_controlpoint.h>
#include <xsi_polygonface.h>
#include <xsi_polygonnode.h>
#include <xsi_vertex.h>
#include <xsi_geometryaccessor.h>
#include <xsi_griddata.h>
#include <xsi_fcurve.h>
#include <xsi_expression.h>
#include <xsi_plugin.h>
#include <xsi_color4f.h>
#include <xsi_x3dobject.h>
#include <xsi_comapihandler.h>

using namespace XSI;

std::vector<FabricSpliceBaseInterface*> FabricSpliceBaseInterface::_instances;
FabricSpliceBaseInterface * FabricSpliceBaseInterface::_currentInstance = NULL;

FabricSpliceBaseInterface::FabricSpliceBaseInterface(){
  XSISPLICE_CATCH_BEGIN();

  xsiInitializeSplice();
  _persist = false;
  _usedInICENode = false;
  _spliceGraph = FabricSplice::DGGraph("softimageGraph");
  _spliceGraph.constructDGNode("DGNode");
  _spliceGraph.setUserPointer(this);
  _objectID = UINT_MAX;
  _instances.push_back(this);
  _nbOutputPorts = 0;

  FabricSplice::setDCCOperatorSourceCodeCallback(&getSourceCodeForOperator);

  XSISPLICE_CATCH_END_VOID();
}

FabricSpliceBaseInterface::~FabricSpliceBaseInterface(){
  for(size_t i=0;i<_instances.size();i++){
    if(_instances[i] == this){
      std::vector<FabricSpliceBaseInterface*>::iterator iter = _instances.begin() + i;
      _instances.erase(iter);
      break;
    }
  }
}

unsigned int FabricSpliceBaseInterface::getObjectID() const
{
  return _objectID;
}

void FabricSpliceBaseInterface::setObjectID(unsigned int objectID)
{
  _objectID = objectID;
}

std::vector<FabricSpliceBaseInterface*> FabricSpliceBaseInterface::getInstances(){
  return _instances;
}

FabricSpliceBaseInterface * FabricSpliceBaseInterface::getInstanceByObjectID(unsigned int objectID) {
  for(size_t i=0;i<_instances.size();i++)
  {
    if(objectID == _instances[i]->getObjectID())
    {
      return _instances[i];
    }
  }
  return NULL;
}

CStatus FabricSpliceBaseInterface::updateXSIOperator()
{
  FabricSplice::Logging::AutoTimer globalTimer("XSI::updateXSIOperator");
  std::string localTimerName = (std::string("XSI::")+_spliceGraph.getName()+"::updateXSIOperator()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName);

  _currentInstance = this;

  setNeedsDeletion(false);

  // delete the previous one
  CRef oldRef = Application().GetObjectFromID(_objectID);
  CustomOperator oldOp(oldRef);

  // persist some generic settings of the CustomOperator
  CValue alwaysEvaluate = oldOp.GetParameterValue("alwaysevaluate");
  CValue mute = oldOp.GetParameterValue("mute");
  CValue debug = oldOp.GetParameterValue("debug");
  CValue alwaysConvertMeshes = oldOp.GetParameterValue("alwaysConvertMeshes");

  // create the operator
  CustomOperator op = Application().GetFactory().CreateObject(L"SpliceOp");
  setObjectID(op.GetObjectID());

  for(std::map<std::string, portInfo>::iterator it = _ports.begin(); it != _ports.end(); it++)
  {
    CString portName = it->first.c_str();
    FabricSplice::Port_Mode portMode = it->second.portMode;
    it->second.portIndices.Clear();
    CString targets = it->second.targets;
    CRefArray targetRefs = getCRefArrayFromCString(targets);

    PortGroup group(op.AddPortGroup(portName, targetRefs.GetCount(), targetRefs.GetCount()));
    for(LONG i=0;i<targetRefs.GetCount();i++)
    {
      if(portMode == FabricSplice::Port_Mode_IN)
      {
        CString indexStr(i);
        op.AddInputPort(targetRefs[i], portName+indexStr, group.GetIndex());
      }
    }
    for(LONG i=0;i<targetRefs.GetCount();i++)
    {
      // for IO ports, softimage changes the order of the
      // ports and the port indices.
      // since we need the port indices for figuring out the
      // output array index for array ports,
      // we'll have to rebuild all of it. the port layout
      // changes are as follows:
      //
      // First port added:
      // Inmatrices0     0
      // Outmatrices0    1
      //
      // Second port added
      // Inmatrices0     0
      // Inmatrices1     1
      // Outmatrices0    2
      // Outmatrices1    3
      //
      // So the output port indices changes for every added
      // IO port. Thus we rebuild the port indices completely
      // from the provided ports in the group.
      if(portMode == FabricSplice::Port_Mode_IO)
      {
        CString indexStr(i);
        op.AddIOPort(targetRefs[i], portName+indexStr, group.GetIndex());
        it->second.realPortName = "In"+portName;

        it->second.portIndices.Clear();
        CRefArray groupPorts = group.GetPorts();
        for(ULONG j=0;j<groupPorts.GetCount();j++)
        {
          Port port(groupPorts[j]);
          if(port.GetPortType() == siPortInput)
            continue;
          it->second.portIndices.Add(port.GetIndex());
        }
      }
    }
    for(LONG i=0;i<targetRefs.GetCount();i++)
    {
      if(portMode == FabricSplice::Port_Mode_OUT)
      {
        CString indexStr(i);
        Port port(op.AddOutputPort(targetRefs[i], portName+indexStr, group.GetIndex()));
        LONG portIndex = port.GetIndex();
        it->second.portIndices.Add(portIndex);
      }
    }

    if(portMode == FabricSplice::Port_Mode_OUT)
    {
      FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.GetAsciiString());
      if(port.isArray())
      {
        FabricCore::RTVal value = port.getRTVal();
        if(value.getArraySize() != targetRefs.GetCount())
        {
          value.setArraySize(targetRefs.GetCount());
          port.setRTVal(value);
        }
      }
    }
  }

  if(op.Connect().Succeeded())
  {
    for(std::map<std::string, parameterInfo>::iterator it = _parameters.begin(); it != _parameters.end(); it++)
    {
      if(it->second.defaultValue == CValue())
        continue;
      CValue value = it->second.defaultValue;
      op.PutParameterValue(it->first.c_str(), value);
    }

    // restore alwaysEvaluate
    op.PutParameterValue("alwaysevaluate", alwaysEvaluate);
    op.PutParameterValue("mute", mute);
    op.PutParameterValue("debug", debug);
    op.PutParameterValue("alwaysConvertMeshes", alwaysConvertMeshes);
  }

  _currentInstance = NULL;

  if(oldRef.IsValid())
  {
    // copy all of the values over
    CustomOperator oldOp(oldRef);

    for(std::map<std::string, parameterInfo>::iterator it = _parameters.begin(); it != _parameters.end(); it++)
    {
      Parameter oldParam = oldOp.GetParameter(it->first.c_str());
      if(!oldParam.IsValid())
        continue;
      Parameter param = op.GetParameter(it->first.c_str());
      if(!param.IsValid())
        continue;
      CRef source = oldParam.GetSource();
      FCurve oldFcurve(source);
      Expression oldExpression(source);
      if(oldFcurve.IsValid())
      {
        FCurve fcurve;
        param.AddFCurve(siDefaultFCurve, fcurve);
        fcurve.Set(oldFcurve);
      }
      else if(oldExpression.IsValid())
      {
        // todo: expression copying doesn't work...
        // CString definition = oldExpression.GetParameterValue(L"definition");
        // Expression expression = param.AddExpression(definition);
        // if(expression.IsValid())
        //   expression.PutParameterValue(L"definition", definition);
        param.PutValue(oldParam.GetValue());
      }
      else
        param.PutValue(oldParam.GetValue());
    }

    CValue returnVal;
    CValueArray args(1);
    args[0] = oldRef.GetAsText();
    Application().ExecuteCommand("DeleteObj", args, returnVal);
  }

  op = CustomOperator();

  setNeedsDeletion(true);

  return CStatus::OK;
}

void FabricSpliceBaseInterface::forceEvaluate()
{
  CRef ref = Application().GetObjectFromID(_objectID);
  CustomOperator op(ref);
  bool alwaysEvaluate = op.GetParameterValue("alwaysevaluate");
  if(!alwaysEvaluate)
  {
    op.PutParameterValue("alwaysevaluate", CValue(true));
    _spliceGraph.requireEvaluate();

    CRefArray ports = op.GetOutputPorts();
    for(LONG i=0;i<ports.GetCount();i++)
    {
      OutputPort port(ports[i]);
      CRef target = port.GetTarget();
      KinematicState kineState(target);
      Primitive primitive(target);
      Parameter parameter(target);

      if(kineState.IsValid())
      {
        kineState.GetTransform();
        break;
      }
      else if(primitive.IsValid())
      {
        primitive.GetGeometry().GetPoints();
        break;
      }
      else if(parameter.IsValid())
      {
        parameter.GetValue();
        break;
      }
    }

    op.PutParameterValue("alwaysevaluate", CValue(false));
  }
}

CStatus FabricSpliceBaseInterface::constructXSIParameters(CustomOperator & op, Factory & factory)
{
  if(_currentInstance == NULL)
    return CStatus::Unexpected;

  for(std::map<std::string, parameterInfo>::iterator it = _currentInstance->_parameters.begin(); it != _currentInstance->_parameters.end(); it++)
  {
    CString portName = it->first.c_str();
    FabricSplice::DGPort port = _currentInstance->_spliceGraph.getDGPort(portName.GetAsciiString());

    double minValue = -1000000.0;
    double maxValue = 1000000.0;
    double minSoftValue = 0.0;
    double maxSoftValue = 1.0;

    float uiMin = port.getScalarOption("uiMin");
    float uiMax = port.getScalarOption("uiMax");
    if(uiMin < uiMax)
    {
      minValue = uiMin;
      maxValue = uiMax;
      float uiSoftMin = port.getScalarOption("uiSoftMin");
      float uiSoftMax = port.getScalarOption("uiSoftMax");
      if(uiSoftMin < uiSoftMax)
      {
        minSoftValue = uiSoftMin;
        maxSoftValue = uiSoftMax;
      }
      else
      {
        minSoftValue = uiMin;
        maxSoftValue = uiMax;
      }
    }

    it->second.paramNames.Clear();
    it->second.paramValues.Clear();
    it->second.paramNames.Add(portName);
    it->second.paramValues.Add(0.0);
    CString dataType = it->second.dataType;
    CValue::DataType xsiDataType = CValue::siEmpty;
    if(dataType == "Boolean")
      xsiDataType = CValue::siBool;
    else if(dataType == "Integer")
    {
      xsiDataType = CValue::siInt4;
    }
    else if(dataType == "Scalar")
    {
      xsiDataType = CValue::siDouble;
    }
    else if(dataType == "String")
    {
      xsiDataType = CValue::siString;
      it->second.paramValues[0] = L"";
    }
    else if(dataType == "Path")
    {
      xsiDataType = CValue::siString;
      it->second.paramValues[0] = L"";
    }
    else if(dataType == "Vec3")
    {
      xsiDataType = CValue::siVector3;
    }
    else if(dataType == "Color")
    {
      xsiDataType = CValue::siColor4f;
    }

    if(xsiDataType == CValue::siEmpty)
    {
      xsiLogErrorFunc("Parameter dataType '"+dataType+"' not supported.");
      continue;
    }

    for(LONG i=0;i<it->second.paramNames.GetCount();i++)
    {
      CRef paramDef = factory.CreateParamDef(it->second.paramNames[i], xsiDataType, siPersistable | siAnimatable, L"", L"", it->second.paramValues[i % it->second.paramValues.GetCount()], minValue, maxValue, minSoftValue, maxSoftValue);
      Parameter param;
      op.AddParameter(paramDef, param);
    }
  }
  return CStatus::OK;
}

CValueArray FabricSpliceBaseInterface::getSpliceParamTypeCombo()
{
  CValueArray combo;
  combo.Add(L"Custom"); combo.Add(L"Custom");
  combo.Add(L"Boolean"); combo.Add(L"Boolean");
  combo.Add(L"Integer"); combo.Add(L"Integer");
  combo.Add(L"Scalar"); combo.Add(L"Scalar");
  combo.Add(L"String"); combo.Add(L"String");
  // combo.Add(L"Color"); combo.Add(L"Color");
  // combo.Add(L"Vec3"); combo.Add(L"Vec3");
  return combo;
}

CValueArray FabricSpliceBaseInterface::getSpliceXSIPortTypeCombo()
{
  CValueArray combo;
  combo.Add(L"Custom"); combo.Add(L"Custom");
  combo.Add(L"Boolean"); combo.Add(L"Boolean");
  combo.Add(L"Integer"); combo.Add(L"Integer");
  combo.Add(L"Scalar"); combo.Add(L"Scalar");
  combo.Add(L"String"); combo.Add(L"String");
  combo.Add(L"Mat44"); combo.Add(L"Mat44");
  combo.Add(L"Xfo"); combo.Add(L"Xfo");
  combo.Add(L"EnvelopeWeight"); combo.Add(L"Float64[]");
  combo.Add(L"WeightMap"); combo.Add(L"Float64[]");
  combo.Add(L"ShapeProperty"); combo.Add(L"Vec3[]");
  combo.Add(L"Lines"); combo.Add(L"Lines");
  combo.Add(L"PolygonMesh"); combo.Add(L"PolygonMesh");
  return combo;
}

CValueArray FabricSpliceBaseInterface::getSpliceInternalPortTypeCombo()
{
  CValueArray combo;
  combo.Add(L"Custom"); combo.Add(L"Custom");
  combo.Add(L"Boolean"); combo.Add(L"Boolean");
  combo.Add(L"Color"); combo.Add(L"Color");
  combo.Add(L"Integer"); combo.Add(L"Integer");
  combo.Add(L"Scalar"); combo.Add(L"Scalar");
  combo.Add(L"String"); combo.Add(L"String");
  combo.Add(L"Vec2"); combo.Add(L"Vec2");
  combo.Add(L"Vec3"); combo.Add(L"Vec3");
  combo.Add(L"Vec4"); combo.Add(L"Vec4");
  combo.Add(L"Quat"); combo.Add(L"Quat");
  combo.Add(L"Euler"); combo.Add(L"Euler");
  combo.Add(L"Xfo"); combo.Add(L"Xfo");
  combo.Add(L"Mat44"); combo.Add(L"Mat44");
  combo.Add(L"Points"); combo.Add(L"Points");
  combo.Add(L"Lines"); combo.Add(L"Lines");
  combo.Add(L"PolygonMesh"); combo.Add(L"PolygonMesh");
  combo.Add(L"GeometryLocation"); combo.Add(L"GeometryLocation");
  combo.Add(L"KeyframeTrack"); combo.Add(L"KeyframeTrack");
  return combo;
}

CStatus FabricSpliceBaseInterface::addXSIParameter(const CString &portName, const CString &dataType, const CString &portModeStr, const XSI::CString & dgNode, const FabricCore::Variant & defaultValue, const XSI::CString & extStr)
{
  FabricSplice::Port_Mode portMode = FabricSplice::Port_Mode_IN;
  if(portModeStr.IsEqualNoCase("out"))
    portMode = FabricSplice::Port_Mode_OUT;
  else if(portModeStr.IsEqualNoCase("io"))
    portMode = FabricSplice::Port_Mode_IO;
  if(!addSplicePort(portName, dataType, portMode, dgNode, true, defaultValue, extStr).Succeeded())
    return CStatus::Unexpected;

  parameterInfo info;
  info.dataType = dataType;
  _parameters.insert(std::pair<std::string, parameterInfo>(portName.GetAsciiString(), info));

  _spliceGraph.getDGPort(portName.GetAsciiString()).setOption("SoftimagePortType", FabricCore::Variant::CreateSInt32(SoftimagePortType_Parameter));

  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::addXSIPort(const CRefArray & targets, const CString &portName, const CString &dataType, const FabricSplice::Port_Mode &portMode, const XSI::CString & dgNode, bool validateDataType)
{
  if(validateDataType &&
     dataType != "Boolean" &&
     dataType != "Integer" &&
     dataType != "Scalar" &&
     dataType != "String" &&
     dataType != "Boolean[]" &&
     dataType != "Integer[]" &&
     dataType != "Scalar[]" &&
     dataType != "String[]" &&
     dataType != "Mat44" &&
     dataType != "Mat44[]" &&
     dataType != "Xfo" &&
     dataType != "Xfo[]" &&
     dataType != "PolygonMesh" &&
     dataType != "PolygonMesh[]" &&
     dataType != "Float64[]" &&
     dataType != "Vec3[]" &&
     dataType != "Lines" &&
     dataType != "Lines[]")
  {
    xsiLogErrorFunc("Operator port dataType '"+dataType+"' not supported.");
    return CStatus::Unexpected;
  }

  if(!addSplicePort(portName, dataType, portMode, dgNode).Succeeded())
    return CStatus::Unexpected;

  portInfo info;
  info.realPortName = portName;
  info.dataType = dataType;
  info.isArray = dataType.GetSubString(dataType.Length() - 2, 2) == L"[]";
  info.portMode = portMode;
  info.targets = targets.GetAsText();
  _ports.insert(std::pair<std::string, portInfo>(portName.GetAsciiString(), info));

  if(portMode != FabricSplice::Port_Mode_IN)
    _nbOutputPorts += targets.GetCount();

  _spliceGraph.getDGPort(portName.GetAsciiString()).setOption("SoftimagePortType", FabricCore::Variant::CreateSInt32(SoftimagePortType_Port));

  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::addXSIICEPort(const CRefArray & targets, const CString &portName, const CString &dataType, const CString &iceAttrName, const XSI::CString & dgNode)
{
  if(!addXSIPort(targets, portName, dataType, FabricSplice::Port_Mode_IN, dgNode, false).Succeeded())
    return CStatus::Unexpected;
  _spliceGraph.getDGPort(portName.GetAsciiString()).setOption("ICEAttribute", FabricCore::Variant::CreateString(iceAttrName.GetAsciiString()));
  _spliceGraph.getDGPort(portName.GetAsciiString()).setOption("SoftimagePortType", FabricCore::Variant::CreateSInt32(SoftimagePortType_ICE));
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::addSplicePort(const CString &portName, const CString &dataType, const FabricSplice::Port_Mode &portMode, const XSI::CString & dgNode, bool autoInitObjects, const FabricCore::Variant & defaultValue, const XSI::CString & extStr)
{
  if(_parameters.find(portName.GetAsciiString()) != _parameters.end())
  {
    xsiLogErrorFunc("Port '"+portName+"' already exists!");
    return CStatus::Unexpected;
  }

  std::map<std::string, portInfo>::iterator it = _ports.find(portName.GetAsciiString());
  if(it != _ports.end())
  {
    if(it->second.portMode == portMode || it->second.portMode == FabricSplice::Port_Mode_IO)
    {
      xsiLogErrorFunc("Port '"+portName+"' already exists!");
      return CStatus::Unexpected;
    }

    it->second.portMode = FabricSplice::Port_Mode_IO;
    _spliceGraph.getDGPort(portName.GetAsciiString()).setMode(it->second.portMode);
    return CStatus::OK;
  }

  XSISPLICE_CATCH_BEGIN()

  _spliceGraph.addDGNodeMember(portName.GetAsciiString(), dataType.GetAsciiString(), defaultValue, dgNode.GetAsciiString(), extStr.GetAsciiString());
  // [andrew 20140414] GetAsciiString()'s previous results are invalidated on each call so need
  // to only call it once per parameter
  const char *portNameAscii = portName.GetAsciiString();
  _spliceGraph.addDGPort(portNameAscii, portNameAscii, portMode, dgNode.GetAsciiString());

  if(portMode != FabricSplice::Port_Mode_IO)
  {
    FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.GetAsciiString());
  }
  XSISPLICE_CATCH_END_CSTATUS()

  _spliceGraph.getDGPort(portName.GetAsciiString()).setOption("SoftimagePortType", FabricCore::Variant::CreateSInt32(SoftimagePortType_Internal));

  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::removeSplicePort(const CString &portName)
{
  XSISPLICE_CATCH_BEGIN()

  FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.GetAsciiString());
  if(!port.isValid())
    return CStatus::OK;

  std::map<std::string, parameterInfo>::iterator parameterIt = _parameters.find(portName.GetAsciiString());
  if(parameterIt != _parameters.end())
    _parameters.erase(parameterIt);
  std::map<std::string, portInfo>::iterator portIt = _ports.find(portName.GetAsciiString());
  if(portIt != _ports.end())
  {
    _nbOutputPorts -= portIt->second.portIndices.GetCount();
    _ports.erase(portIt);
  }

  _spliceGraph.removeDGNodeMember(portName.GetAsciiString(), port.getDGNodeName());

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::rerouteXSIPort(const CString &portName, FabricCore::Variant & scriptArgs)
{
  XSISPLICE_CATCH_BEGIN()

  std::map<std::string, portInfo>::iterator portIt = _ports.find(portName.GetAsciiString());
  if(portIt == _ports.end())
  {
    xsiLogErrorFunc(("The port '"+portName+"' is not connected to a XSI port.").GetAsciiString());
    return CStatus::Unexpected;
  }

  FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.GetAsciiString());
  if(!port.isValid())
  {
    xsiLogErrorFunc(("The port '"+portName+"' does not exist.").GetAsciiString());
    return CStatus::Unexpected;
  }
  bool isICEAttribute = port.getOption("ICEAttribute").isString();

  CRefArray targetRefs;
  CString targetStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "targets", "", true).c_str();
  if(!targetStr.IsEmpty())
  {
    targetRefs = getCRefArrayFromCString(targetStr);
    if(targetRefs.GetCount() == 0)
    {
      xsiLogErrorFunc(("The targets '"+targetStr+"' aren't valid.").GetAsciiString());
      return CStatus::Unexpected;
    }
  }
  if(targetRefs.GetCount() == 0){
    xsiLogErrorFunc("The targets argument must be an array of scene objects.");
    return CStatus::Unexpected;
  }

  if(isICEAttribute)
  {
    CString iceAttributeName = port.getOption("ICEAttribute").getStringData();
    Primitive prim(targetRefs[0]);
    Geometry geo(prim.GetGeometry());
    if(!geo.GetICEAttributeFromName(iceAttributeName).IsValid())
    {
      xsiLogErrorFunc(("The target '"+prim.GetFullName()+"' doesn't contain an ICE attribute '"+iceAttributeName+"'.").GetAsciiString());
      return CStatus::Unexpected;
    }
  }

  portIt->second.targets = targetRefs.GetAsText();

  if(!updateXSIOperator().Succeeded())
    return CStatus::Unexpected;

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CString FabricSpliceBaseInterface::getXSIPortTargets(const CString & portName)
{
  std::map<std::string, portInfo>::iterator portIt = _ports.find(portName.GetAsciiString());
  if(portIt == _ports.end())
    return CString();
  return portIt->second.targets;
}

void FabricSpliceBaseInterface::setXSIPortTargets(const CString &portName, const CString &targets)
{
  std::map<std::string, portInfo>::iterator portIt = _ports.find(portName.GetAsciiString());
  if(portIt != _ports.end())
    portIt->second.targets = targets;
}

CString FabricSpliceBaseInterface::getParameterString()
{
  return _spliceGraph.generateKLOperatorParameterList().getStringData();
}

bool convertBasicInputParameter(const CString & dataType, const CValue & value, FabricCore::RTVal & rtVal)
{
  if(dataType == "Boolean")
    rtVal = FabricSplice::constructBooleanRTVal((bool)value);
  else if(dataType == "Integer")
    rtVal = FabricSplice::constructSInt32RTVal((LONG)value);
  else if(dataType == "Scalar")
    rtVal = FabricSplice::constructFloat32RTVal((float)value);
  else if(dataType == "String")
    rtVal = FabricSplice::constructStringRTVal(((CString)value).GetAsciiString());
  else if(dataType == "Vec3")
  {
    MATH::CVector3 color = value;
    rtVal = FabricSplice::constructRTVal("Vec3");
    rtVal.setMember("x", FabricSplice::constructFloat32RTVal(color.GetX()));
    rtVal.setMember("y", FabricSplice::constructFloat32RTVal(color.GetY()));
    rtVal.setMember("z", FabricSplice::constructFloat32RTVal(color.GetZ()));
  }
  else if(dataType == "Color")
  {
    MATH::CColor4f color = value;
    rtVal = FabricSplice::constructRTVal("Color");
    rtVal.setMember("r", FabricSplice::constructFloat32RTVal(color.GetR()));
    rtVal.setMember("g", FabricSplice::constructFloat32RTVal(color.GetG()));
    rtVal.setMember("b", FabricSplice::constructFloat32RTVal(color.GetB()));
    rtVal.setMember("a", FabricSplice::constructFloat32RTVal(color.GetA()));
  }
  else
    return false;
  return true;
}

bool convertBasicOutputParameter(const CString & dataType, CValue & value, FabricCore::RTVal & rtVal)
{
  if(dataType == "Boolean")
    value = rtVal.getBoolean();
  else if(dataType == "Integer")
    value = getFloat64FromRTVal(rtVal);
  else if(dataType == "Scalar")
    value = getFloat64FromRTVal(rtVal);
  else if(dataType == "String")
    value = CString(rtVal.getStringCString());
  else if(dataType == "Color")
  {
    MATH::CColor4f color;
    color.PutR(getFloat64FromRTVal(rtVal.maybeGetMember("r")));
    color.PutG(getFloat64FromRTVal(rtVal.maybeGetMember("g")));
    color.PutB(getFloat64FromRTVal(rtVal.maybeGetMember("b")));
    color.PutA(getFloat64FromRTVal(rtVal.maybeGetMember("a")));
    value = color;
  }
  else if(dataType == "Vec3")
  {
    MATH::CVector3 vec3;
    vec3.PutX(getFloat64FromRTVal(rtVal.maybeGetMember("x")));
    vec3.PutY(getFloat64FromRTVal(rtVal.maybeGetMember("y")));
    vec3.PutZ(getFloat64FromRTVal(rtVal.maybeGetMember("z")));
    value = vec3;
  }
  else
    return false;
  return true;
}


void FabricSpliceBaseInterface::addDirtyInput(std::string portName, FabricCore::RTVal evalContext, int index){
  if(index == -1)
  {
    FabricCore::RTVal input = FabricSplice::constructStringRTVal(portName.c_str());
    evalContext.callMethod("", "_addDirtyInput", 1, &input);
  }
  else{
    FabricCore::RTVal args[2] = {
      FabricSplice::constructStringRTVal(portName.c_str()),
      FabricSplice::constructSInt32RTVal(index)
    };
    evalContext.callMethod("", "_addDirtyInput", 2, &args[0]);
  }
}

bool FabricSpliceBaseInterface::checkIfValueChangedAndDirtyInput(CValue value, std::vector<XSI::CValue> &cachedValues, bool alwaysEvaluate, std::string portName, FabricCore::RTVal evalContext, int index){
  if(index == -1){
    cachedValues.resize(1);
    index = 0;
  }
  else if(cachedValues.size() <= index)
    cachedValues.resize(index+1);

  bool result = false;
  if(cachedValues[index] != value || alwaysEvaluate) {
    addDirtyInput(portName, evalContext, index);
    cachedValues[index] = value;
    result = true;
  }
  return result;
}

bool FabricSpliceBaseInterface::checkEvalIDCache(LONG evalID, int &evalIDCacheIndex, bool alwaysEvaluate){
  if(evalIDsCache.size() <= evalIDCacheIndex)
    evalIDsCache.resize(evalIDCacheIndex+1);
  bool result = evalIDsCache[evalIDCacheIndex] == evalID && !alwaysEvaluate;
  evalIDsCache[evalIDCacheIndex] = evalID;
  evalIDCacheIndex++;
  return result;
}

bool FabricSpliceBaseInterface::transferInputPorts(XSI::CRef opRef, OperatorContext & context)
{
  FabricSplice::Logging::AutoTimer globalTimer("XSI::transferInputPorts");
  std::string localTimerName = (std::string("XSI::")+_spliceGraph.getName()+"::transferInputPorts()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName);

  bool result = false;

  // If 'AlwaysEvaluate' is on, then we will evaluate even if no changes have occured.
  // this can be usefull in debugging, or when an operator simply must evaluate even if none of its inputs are dirty.
  CustomOperator op(opRef);
  bool alwaysEvaluate = bool(op.GetParameterValue("AlwaysEvaluate"));
  if(alwaysEvaluate)
  {
    result = true;
  }
  else
  {
    // for output mat44 array ports, we should skip the transfer if another
    // element of that array has already performed conversion.
    // transferOutputPort will reset the counter once all outputs
    // have been transfered.
    OutputPort xsiPort(context.GetOutputPort());
    std::string outPortName = xsiPort.GetGroupName().GetAsciiString();

    std::map<std::string, portInfo>::iterator it = _ports.find(outPortName);
    if(it != _ports.end())
    {
      if(it->second.dataType == "Mat44[]")
      {
        if(it->second.outPortElementsProcessed > 0)
          return false;
      }
    }
  }

  FabricCore::RTVal evalContext = _spliceGraph.getEvalContext();
  evalContext.callMethod("", "_clear", 0, 0);

  // If the splice op has only output ports, then we should force an evaluation.
  // otherwize w must always provide one input param. (simple testing scenarios might not include input params).
  bool nodeHasInputs = false;

  OutputPort xsiPort(context.GetOutputPort());
  std::string outPortName = xsiPort.GetGroupName().GetAsciiString();

  // make sure that the output array is already of the right size.
  // we need to resize the outputs prior to performing the operators.
  if(outPortName.length() > 0)
  {
    std::map<std::string, portInfo>::iterator it = _ports.find(outPortName);
    if(it != _ports.end())
    {
      unsigned int arraySize = it->second.portIndices.GetCount();

      try
      {
        FabricCore::RTVal rtVal;
        FabricSplice::DGPort port = _spliceGraph.getDGPort(outPortName.c_str());
        FabricCore::RTVal arrayVal = port.getRTVal();
        if(arrayVal.isValid() && arrayVal.isArray())
        {
          if(arrayVal.getArraySize() != arraySize)
          {
            FabricCore::RTVal countVal = FabricSplice::constructUInt32RTVal(arraySize);
            arrayVal.callMethod("", "resize", 1, &countVal);
            port.setRTVal(arrayVal);
          }
        }
      }
      catch(FabricSplice::Exception e)
      {
        xsiLogErrorFunc("Error resizing output array:" + CString(outPortName.c_str()) + ": " + CString(e.what()));
      }
      catch(FabricCore::Exception e)
      {
        xsiLogErrorFunc("Error resizing output array:" + CString(outPortName.c_str()) + ": " + CString(e.getDesc_cstr()));
      }
    }
  }

  // setting to determine if we need to always convert meshes
  bool alwaysConvertMeshes = op.GetParameterValue("alwaysConvertMeshes");

  // Simple values are cached in the CValues cache member. we don't know how many cache values we will require
  // because this depends on the port type. We simply grow the array as we need it, and never shrink it. Every
  // time we store a cache value, we should increment this value.
  int valueCacheIndex = 0;
  // Complex data such as geometries provides an 'EvaluatoinID' which is similar to the version number we have in
  // KL Geometry objects. We can simply cache the evaluation id and compare the current value iwth cached values.
  // We als don't know how many evaluation ids we will need, so we simply grow the array as needed.
  int evalIDCacheIndex = 0;
  {
    FabricSplice::Logging::AutoTimer globalTimer("XSI::transferInputParameters");
    std::string localTimerName = (std::string("XSI::")+_spliceGraph.getName()+"::transferInputParameters()").c_str();
    FabricSplice::Logging::AutoTimer localTimer(localTimerName);

    if(valuesCache.size() < _parameters.size())
      valuesCache.resize(_parameters.size());

    // First transfer all the basic parameters.
    for(std::map<std::string, parameterInfo>::iterator it = _parameters.begin(); it != _parameters.end(); it++)
    {
      std::string portName = it->first;
      try
      {
        CValue value = context.GetParameterValue(portName.c_str());
        if(checkIfValueChangedAndDirtyInput(value, valuesCache[valueCacheIndex], alwaysEvaluate, portName, evalContext, -1))
        {
          FabricCore::RTVal rtVal;
          if(!convertBasicInputParameter(it->second.dataType, value, rtVal))
            continue;
          FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.c_str());
          port.setRTVal(rtVal);
          result = true;
        }
        valueCacheIndex++;
        nodeHasInputs = true;
      }
      catch(FabricSplice::Exception e)
      {
        xsiLogErrorFunc("Error accessing Parameter:" + CString(portName.c_str()) + ": " + CString(e.what()));
      }
      catch(FabricCore::Exception e)
      {
        xsiLogErrorFunc("Error accessing Parameter:" + CString(portName.c_str()) + ": " + CString(e.getDesc_cstr()));
      }
    }
  }

  for(std::map<std::string, portInfo>::iterator it = _ports.begin(); it != _ports.end(); it++)
  {
    if(it->second.portMode == FabricSplice::Port_Mode_OUT)
      continue;
    nodeHasInputs = true;
    std::string portName = it->first;

    try
    {
      FabricSplice::DGPort splicePort = _spliceGraph.getDGPort(portName.c_str());

      FabricCore::Variant iceAttrName = splicePort.getOption("ICEAttribute");
      if(iceAttrName.isString())
      {
        Primitive prim((CRef)context.GetInputValue(it->second.realPortName+CString(CValue(CValue(0)))));

        // Now check if the input geometry has changed scince our previous evaluation.
        if(!alwaysConvertMeshes)
        {
          LONG evalID = ProjectItem(prim).GetEvaluationID();
          if(checkEvalIDCache( evalID, evalIDCacheIndex, alwaysEvaluate))
            continue;
        }

        Geometry xsiGeo = prim.GetGeometry();
        CString iceAttrStr = iceAttrName.getStringData();
        ICEAttribute iceAttr = xsiGeo.GetICEAttributeFromName(iceAttrStr);
        if(iceAttr.IsValid()){
          convertInputICEAttribute(splicePort, it->second.dataType, iceAttr, xsiGeo);
          addDirtyInput(portName, evalContext, -1);
          result = true;
        }
      }
      else if(it->second.dataType == "Boolean" ||
         it->second.dataType == "Integer" ||
         it->second.dataType == "Scalar" ||
         it->second.dataType == "String")
      {
        CValue value = context.GetInputValue(portName.c_str()+CString(CValue(0)));
        if(valuesCache.size() <= valueCacheIndex)
          valuesCache.resize(valueCacheIndex+1);
        if(checkIfValueChangedAndDirtyInput(value, valuesCache[valueCacheIndex], alwaysEvaluate, portName, evalContext, -1))
        {
          FabricCore::RTVal rtVal;
          if(convertBasicInputParameter(it->second.dataType, value, rtVal))
            splicePort.setRTVal(rtVal);
          result = true;
        }
        valueCacheIndex++;
      }
      else if(it->second.dataType == "Boolean[]" ||
         it->second.dataType == "Integer[]" ||
         it->second.dataType == "Scalar[]" ||
         it->second.dataType == "String[]")
      {
        if(valuesCache.size() <= valueCacheIndex)
          valuesCache.resize(valueCacheIndex+1);

        CString singleDataType = it->second.dataType.GetSubString(0, it->second.dataType.Length()-2);
        FabricCore::RTVal arrayVal = splicePort.getRTVal();
        uint32_t arraySize = splicePort.getArrayCount();
        for(int i=0; ; i++)
        {
          CValue value = context.GetInputValue(portName.c_str()+CString(i));
          if(value.IsEmpty())
            break;
          if(i >= arraySize){
            valuesCache[valueCacheIndex].resize(i+1);
          }
          if(checkIfValueChangedAndDirtyInput(value, valuesCache[valueCacheIndex], alwaysEvaluate, portName, evalContext, i))
          {
            FabricCore::RTVal rtVal;
            if(convertBasicInputParameter(singleDataType, value, rtVal)){
              if(i >= arraySize){
                arrayVal.callMethod("", "push", 1, &rtVal);
                result = true;
                arraySize++;
              }
              else{
                arrayVal.callMethod("", "setArrayElement", 1, &rtVal);
              }
            }
            result = true;
          }
          valueCacheIndex++;
        }
        splicePort.setRTVal(arrayVal);
      }
      else if(it->second.dataType == "Float64[]")
      {
        ClusterProperty clsProp((CRef)context.GetInputValue(it->second.realPortName+CString(CValue(0))));
        if(!clsProp.IsValid())
          continue;
        /*if(!alwaysConvertMeshes)
        {
          LONG evalID = ProjectItem(clsProp).GetEvaluationID();
          if(checkEvalIDCache( evalID, evalIDCacheIndex, alwaysEvaluate))
            continue;
        }*/

        CClusterPropertyElementArray clsPropElem(clsProp.GetElements());
        splicePort.setArrayData(&clsPropElem.GetArray()[0], sizeof(double) * clsProp.GetValueSize() *  clsPropElem.GetCount());
      }
      else if(it->second.dataType == "Vec3[]")
      {
        ClusterProperty clsProp((CRef)context.GetInputValue(it->second.realPortName+CString(CValue(0))));
        if(!clsProp.IsValid())
          continue;
        /*if(!alwaysConvertMeshes)
        {
          LONG evalID = ProjectItem(clsProp).GetEvaluationID();
          if(checkEvalIDCache( evalID, evalIDCacheIndex, alwaysEvaluate))
            continue;
        }*/

        CClusterPropertyElementArray clsPropElem(clsProp.GetElements());
        CFloatArray values;
        clsProp.GetValues(values);
        splicePort.setArrayData(&values[0], sizeof(float) * clsProp.GetValueSize() *  clsPropElem.GetCount());
      }
      else if(it->second.dataType == "Mat44")
      {
        LONG i = 0;
        KinematicState kine((CRef)context.GetInputValue(it->second.realPortName+CString(i)));
        MATH::CMatrix4 matrix = kine.GetTransform().GetMatrix4();
        FabricCore::RTVal rtVal;
        getRTValFromCMatrix4(matrix, rtVal);

        FabricCore::RTVal currVal = splicePort.getRTVal();
        if(!currVal.callMethod("Boolean", "almostEqual", 1, &rtVal).getBoolean()){
          splicePort.setRTVal(rtVal);
          addDirtyInput(portName, evalContext, -1);
          result = true;
        }
      }
      else if(it->second.dataType == "Mat44[]")
      {
        FabricCore::RTVal arrayVal = splicePort.getRTVal();
        for(int i=0; ; i++)
        {
          KinematicState kine((CRef)context.GetInputValue(it->second.realPortName+CString(i)));
          if(!kine.IsValid())
            break;
          MATH::CMatrix4 matrix = kine.GetTransform().GetMatrix4();
          FabricCore::RTVal rtVal;
          getRTValFromCMatrix4(matrix, rtVal);


          if(arrayVal.getArraySize() <= i)
          {
            arrayVal.callMethod("", "push", 1, &rtVal);
            addDirtyInput(portName, evalContext, i);
            result = true;
          }
          else
          {
            FabricCore::RTVal currVal = arrayVal.getArrayElement(i);
            if(!currVal.callMethod("Boolean", "almostEqual", 1, &rtVal).getBoolean()){
              arrayVal.setArrayElement(i, rtVal);
              addDirtyInput(portName, evalContext, i);
              result = true;
            }
          }
        }
        splicePort.setRTVal(arrayVal);
      }
	  ////////
      else if(it->second.dataType == "Xfo")
      {
        LONG i = 0;
        KinematicState kine((CRef)context.GetInputValue(it->second.realPortName+CString(i)));
        MATH::CTransformation transform = kine.GetTransform();
        FabricCore::RTVal rtVal;
        getRTValFromCTransformation(transform, rtVal);

        FabricCore::RTVal currVal = splicePort.getRTVal();
        if(!currVal.callMethod("Boolean", "almostEqual", 1, &rtVal).getBoolean()){
          splicePort.setRTVal(rtVal);
          addDirtyInput(portName, evalContext, -1);
          result = true;
        }
      }
      else if(it->second.dataType == "Xfo[]")
      {
        FabricCore::RTVal arrayVal = splicePort.getRTVal();
        for(int i=0; ; i++)
        {
          KinematicState kine((CRef)context.GetInputValue(it->second.realPortName+CString(i)));
          if(!kine.IsValid())
            break;
          MATH::CTransformation transform = kine.GetTransform();
          FabricCore::RTVal rtVal;
          getRTValFromCTransformation(transform, rtVal);


          if(arrayVal.getArraySize() <= i)
          {
            arrayVal.callMethod("", "push", 1, &rtVal);
            addDirtyInput(portName, evalContext, i);
            result = true;
          }
          else
          {
            FabricCore::RTVal currVal = arrayVal.getArrayElement(i);
            if(!currVal.callMethod("Boolean", "almostEqual", 1, &rtVal).getBoolean()){
              arrayVal.setArrayElement(i, rtVal);
              addDirtyInput(portName, evalContext, i);
              result = true;
            }
          }
        }
        splicePort.setRTVal(arrayVal);
      }
	  ////////
      else if(it->second.dataType == "PolygonMesh")
      {
        Primitive prim;
        if(it->second.portMode == FabricSplice::Port_Mode_IO && portName == outPortName)
          prim = context.GetOutputTarget();
        else
          prim = (CRef)context.GetInputValue(it->second.realPortName+CString(CValue(0)));
        if(!prim.IsValid())
          break;

        // Now check if the input geometry has changed scince our previous evaluation.
        if(!alwaysConvertMeshes)
        {
          LONG evalID = ProjectItem(prim).GetEvaluationID();
          if(checkEvalIDCache( evalID, evalIDCacheIndex, alwaysEvaluate))
            continue;
        }

        PolygonMesh mesh = PolygonMesh(prim.GetGeometry().GetRef());

        FabricCore::RTVal rtVal = splicePort.getRTVal();
        convertInputPolygonMesh(mesh, rtVal);
        splicePort.setRTVal(rtVal);

        addDirtyInput(portName, evalContext, -1);
        result = true;
      }
      else if(it->second.dataType == "PolygonMesh[]")
      {
        FabricCore::RTVal arrayVal = splicePort.getRTVal();
        for(int i=0; ; i++)
        {
          Primitive prim((CRef)context.GetInputValue(it->second.realPortName+CString(i)));
          if(!prim.IsValid())
            break;

          // Now check if the input geometry has changed scince our previous evaluation.
          if(!alwaysConvertMeshes)
          {
            LONG evalID = ProjectItem(prim).GetEvaluationID();
            if(checkEvalIDCache( evalID, evalIDCacheIndex, alwaysEvaluate))
              continue;
          }

          PolygonMesh mesh = PolygonMesh(prim.GetGeometry().GetRef());
          if(arrayVal.getArraySize() <= i)
          {
            FabricCore::RTVal rtVal;
            convertInputPolygonMesh(mesh, rtVal);
            arrayVal.callMethod("", "push", 1, &rtVal);
          }
          else
          {
            FabricCore::RTVal rtVal = arrayVal.getArrayElement(i);
            convertInputPolygonMesh( mesh, rtVal);
            arrayVal.setArrayElement(i, rtVal);
          }
          addDirtyInput(portName, evalContext, i);
          result = true;
        }
        splicePort.setRTVal(arrayVal);
      }
      else if(it->second.dataType == "Lines")
      {
        Primitive prim;
        if(it->second.portMode == FabricSplice::Port_Mode_IO && portName == outPortName)
          prim = context.GetOutputTarget();
        else
          prim = (CRef)context.GetInputValue(portName.c_str()+CString(CValue(0)));

        // Now check if the input geometry has changed scince our previous evaluation.
        if(!alwaysConvertMeshes)
        {
          LONG evalID = ProjectItem(prim).GetEvaluationID();
          if(checkEvalIDCache( evalID, evalIDCacheIndex, alwaysEvaluate))
            continue;
        }

        NurbsCurveList curveList = prim.GetGeometry().GetRef();
        FabricCore::RTVal rtVal = splicePort.getRTVal();
        convertInputLines( curveList, rtVal);
        splicePort.setRTVal(rtVal);
        addDirtyInput(portName, evalContext, -1);
        result = true;
      }
      else if(it->second.dataType == "Lines[]")
      {
        FabricCore::RTVal arrayVal = splicePort.getRTVal();
        for(int i=0; ; i++)
        {
          Primitive prim((CRef)context.GetInputValue(portName.c_str()+CString(i)));
          if(!prim.IsValid())
            break;

          // Now check if the input geometry has changed scince our previous evaluation.
          if(!alwaysConvertMeshes)
          {
            LONG evalID = ProjectItem(prim).GetEvaluationID();
            if(checkEvalIDCache( evalID, evalIDCacheIndex, alwaysEvaluate))
              continue;
          }

          NurbsCurveList curveList = prim.GetGeometry().GetRef();
          if(arrayVal.getArraySize() <= i)
          {
            FabricCore::RTVal rtVal;
            convertInputLines( curveList, rtVal);
            arrayVal.callMethod("", "push", 1, &rtVal);
          }
          else
          {
            FabricCore::RTVal rtVal = arrayVal.getArrayElement(i);
            convertInputLines( curveList, rtVal);
            arrayVal.setArrayElement(i, rtVal);
          }
          addDirtyInput(portName, evalContext, i);
          result = true;
        }
        splicePort.setRTVal(arrayVal);
      }
      else
      {
        xsiLogErrorFunc("Skipping input port of type "+it->second.dataType);
      }

    }
    catch(FabricSplice::Exception e)
    {
      xsiLogErrorFunc("Error accessing Port:" + CString(portName.c_str()) + ": " + CString(e.what()));
    }
    catch(FabricCore::Exception e)
    {
      xsiLogErrorFunc("Error accessing Port:" + CString(portName.c_str()) + ": " + CString(e.getDesc_cstr()));
    }
  }

  // see declaration of nodeHasInputs
  if(!nodeHasInputs)
    result = true;

  return result;
}

CStatus FabricSpliceBaseInterface::transferOutputPort(OperatorContext & context)
{
  FabricSplice::Logging::AutoTimer globalTimer("XSI::transferOutputPort");
  std::string localTimerName = (std::string("XSI::")+_spliceGraph.getName()+"::transferOutputPort()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName);

  try
  {
    OutputPort xsiPort(context.GetOutputPort());
    std::string outPortName = xsiPort.GetGroupName().GetAsciiString();

    std::map<std::string, portInfo>::iterator it = _ports.find(outPortName);
    if(it == _ports.end())
      return CStatus::Unexpected;
    if(it->second.portMode == FabricSplice::Port_Mode_IN)
      return CStatus::Unexpected;

    FabricSplice::DGPort splicePort = _spliceGraph.getDGPort(it->first.c_str());

    if(it->second.dataType == "Boolean" ||
       it->second.dataType == "Integer" ||
       it->second.dataType == "Scalar" ||
       it->second.dataType == "String" ||
       it->second.dataType == "Color" ||
       it->second.dataType == "Vec3")
    {
      CValue value;
      FabricCore::RTVal rtVal = splicePort.getRTVal();
      if(convertBasicOutputParameter(it->second.dataType, value, rtVal))
        xsiPort.PutValue(value);
    }
    else if(it->second.dataType == "Boolean[]" ||
       it->second.dataType == "Integer[]" ||
       it->second.dataType == "Scalar[]" ||
       it->second.dataType == "String[]")
    {
      CString singleDataType = it->second.dataType.GetSubString(0, it->second.dataType.Length()-2);
      FabricCore::RTVal rtVal = splicePort.getRTVal();
      uint32_t arraySize = splicePort.getArrayCount();
      uint32_t portIndex = xsiPort.GetIndex();
      uint32_t arrayIndex = UINT_MAX;
      for(LONG i=0;i<it->second.portIndices.GetCount();i++)
      {
        if(it->second.portIndices[i] == portIndex)
        {
          arrayIndex = i;
          break;
        }
      }
      if(arrayIndex < arraySize)
      {
        CValue value;
        FabricCore::RTVal rtValElement = rtVal.getArrayElement(arrayIndex);
        if(convertBasicOutputParameter(singleDataType, value, rtValElement))
          xsiPort.PutValue(value);
      }
    }
    else if(it->second.dataType == "Float64[]")
    {
      Application().LogMessage("Float64[]");
      FabricCore::RTVal rtVal = splicePort.getRTVal();

      ClusterProperty clsProp((CRef)context.GetOutputTarget());
      CClusterPropertyElementArray clsPropElem(clsProp.GetElements());

      std::vector<double> spliceValues;
      spliceValues.resize(clsProp.GetValueSize() * clsPropElem.GetCount());
      splicePort.getArrayData(&spliceValues[0], sizeof(double) * clsProp.GetValueSize() * clsPropElem.GetCount());
      CDoubleArray XsiValues;
      XsiValues.Attach(&spliceValues[0], clsProp.GetValueSize() * clsPropElem.GetCount());
      clsPropElem.PutArray(XsiValues);
    }
    else if(it->second.dataType == "Vec3[]")
    {
      //We do not write data for shape properties (for now??)

      /*FabricCore::RTVal rtVal = splicePort.getRTVal();

      ClusterProperty clsProp((CRef)context.GetOutputTarget());
      CClusterPropertyElementArray clsPropElem(clsProp.GetElements());

      std::vector<float> spliceValues;
      spliceValues.resize(clsProp.GetValueSize() * clsPropElem.GetCount());
      splicePort.getArrayData(&spliceValues[0], sizeof(float) * clsProp.GetValueSize() * clsPropElem.GetCount());
      //unfortunatly we need to cast back to doubles manually
      CDoubleArray XsiValues(spliceValues.size());
      for (int i = 0; i < XsiValues.GetCount(); ++i)
      {
        XsiValues[i] = double(spliceValues[i]);
      }
      Application().LogMessage(XsiValues.GetAsText());
      //XsiValues.Attach(&spliceValues[0], clsProp.GetValueSize() * clsPropElem.GetCount());
      clsPropElem.PutArray(XsiValues);*/
    }
    else if(it->second.dataType == "Mat44")
    {
      FabricCore::RTVal rtVal = splicePort.getRTVal();
      MATH::CMatrix4 matrix;
      getCMatrix4FromRTVal(rtVal, matrix);

      MATH::CTransformation transform;
      transform.SetMatrix4(matrix);
      KinematicState kine(context.GetOutputTarget());
      kine.PutTransform(transform);
    }
    else if(it->second.dataType == "Mat44[]")
    {
      FabricCore::RTVal rtVal = splicePort.getRTVal();
      uint32_t arraySize = splicePort.getArrayCount();
      uint32_t portIndex = xsiPort.GetIndex();
      uint32_t arrayIndex = UINT_MAX;

      // increment the counter for the processed elements
      // only at count 0 we will perform transfer input
      it->second.outPortElementsProcessed++;
      if(it->second.outPortElementsProcessed == arraySize)
        it->second.outPortElementsProcessed = 0;

      for(LONG i=0;i<it->second.portIndices.GetCount();i++)
      {
        if(it->second.portIndices[i] == portIndex)
        {
          arrayIndex = i;
          break;
        }
      }
      if(arrayIndex < arraySize)
      {
        // todo: maybe we should be caching this....
        MATH::CMatrix4 matrix;
        getCMatrix4FromRTVal(rtVal.getArrayElement(arrayIndex), matrix);

        MATH::CTransformation transform;
        transform.SetMatrix4(matrix);
        KinematicState kine(context.GetOutputTarget());
        kine.PutTransform(transform);
      }
    }
    else if(it->second.dataType == "Xfo")
    {
      FabricCore::RTVal rtVal = splicePort.getRTVal();
      MATH::CTransformation transform;
      getCTransformationFromRTVal(rtVal, transform);

      KinematicState kine(context.GetOutputTarget());
      kine.PutTransform(transform);
    }
    else if(it->second.dataType == "Xfo[]")
    {
      FabricCore::RTVal rtVal = splicePort.getRTVal();
      uint32_t arraySize = splicePort.getArrayCount();
      uint32_t portIndex = xsiPort.GetIndex();
      uint32_t arrayIndex = UINT_MAX;

      // increment the counter for the processed elements
      // only at count 0 we will perform transfer input
      it->second.outPortElementsProcessed++;
      if(it->second.outPortElementsProcessed == arraySize)
        it->second.outPortElementsProcessed = 0;

      for(LONG i=0;i<it->second.portIndices.GetCount();i++)
      {
        if(it->second.portIndices[i] == portIndex)
        {
          arrayIndex = i;
          break;
        }
      }
      if(arrayIndex < arraySize)
      {
        // todo: maybe we should be caching this....
        MATH::CMatrix4 matrix;
        getCMatrix4FromRTVal(rtVal.getArrayElement(arrayIndex), matrix);

        MATH::CTransformation transform;
        getCTransformationFromRTVal(rtVal.getArrayElement(arrayIndex), transform);

        KinematicState kine(context.GetOutputTarget());
        kine.PutTransform(transform);
      }
    }
    else if(it->second.dataType == "PolygonMesh" || it->second.dataType == "PolygonMesh[]")
    {
      bool isArray = it->second.dataType == "PolygonMesh[]";
      FabricCore::RTVal rtVal;
      if(isArray)
      {
        FabricCore::RTVal arrayVal = splicePort.getRTVal();
        uint32_t portIndex = xsiPort.GetIndex();
        uint32_t arrayIndex = UINT_MAX;
        for(LONG i=0;i<it->second.portIndices.GetCount();i++)
        {
          if(it->second.portIndices[i] == portIndex)
          {
            arrayIndex = i;
            break;
          }
        }
        if(arrayIndex < arrayVal.getArraySize())
        {
          rtVal = arrayVal.getArrayElement(arrayIndex);
        }
        else
        {
          xsiLogErrorFunc("Error writing to OutPort:" + CString(outPortName.c_str()) + ": The KL array size does not match the number of target connections.");
          return CStatus::Unexpected;
        }
      }
      else
      {
        rtVal = splicePort.getRTVal();
      }

      if(!rtVal.isValid() || rtVal.isNullObject())
      {
        xsiLogErrorFunc("Error writing to OutPort:" + CString(outPortName.c_str()) + ": PolygonMesh is not valid. Please construct the PolygonMesh in your KL operator.");
        return CStatus::Unexpected;
      }

      Primitive prim(context.GetOutputTarget());
      PolygonMesh mesh(prim.GetGeometry());
      convertOutputPolygonMesh( mesh, rtVal);
    }
    else if(it->second.dataType == "Lines" || it->second.dataType == "Lines[]")
    {
      bool isArray = it->second.dataType == "Lines[]";

      FabricCore::RTVal rtVal;
      if(isArray)
      {
        FabricCore::RTVal arrayVal = splicePort.getRTVal();
        uint32_t portIndex = xsiPort.GetIndex();
        uint32_t arrayIndex = UINT_MAX;
        for(LONG i=0;i<it->second.portIndices.GetCount();i++)
        {
          if(it->second.portIndices[i] == portIndex)
          {
            arrayIndex = i;
            break;
          }
        }
        if(arrayIndex <  arrayVal.getArraySize())
        {
          rtVal = arrayVal.getArrayElement(arrayIndex);
        }
        else
        {
          xsiLogErrorFunc("Error writing to OutPort:" + CString(outPortName.c_str()) + ": The KL array size does not match the number of target connections.");
          return CStatus::Unexpected;
        }
      }
      else
      {
        rtVal = splicePort.getRTVal();
      }

      if(!rtVal.isValid() || rtVal.isNullObject())
      {
        xsiLogErrorFunc("Error writing to OutPort:" + CString(outPortName.c_str()) + ": Lines is not valid. Please construct the Lines in your KL operator.");
        return CStatus::Unexpected;
      }

      Primitive prim(context.GetOutputTarget());
      NurbsCurveList curveList(prim.GetGeometry());
      convertOutputLines( curveList, rtVal);
    }
    else
    {
      xsiLogErrorFunc("Skipping output port of type "+it->second.dataType);
    }

  }
  catch(FabricSplice::Exception e)
  {
    xsiLogErrorFunc(e.what());
  }
  catch(FabricCore::Exception e)
  {
    xsiLogErrorFunc(e.getDesc_cstr());
  }
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::evaluate()
{
  CRef ofRef = Application().GetObjectFromID((LONG)getObjectID());

  FabricSplice::Logging::AutoTimer globalTimer("XSI::evaluate");
  std::string localTimerName = (std::string("XSI::")+_spliceGraph.getName()+"::evaluate()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName);

  // setup the context
  FabricCore::RTVal context = _spliceGraph.getEvalContext();
  context.setMember("host", FabricSplice::constructStringRTVal("Softimage"));
  context.setMember("graph", FabricSplice::constructStringRTVal(ofRef.GetAsText().GetAsciiString()));
  context.setMember("time", FabricSplice::constructFloat32RTVal(CTime().GetTime( CTime::Seconds )));
  context.setMember("currentFilePath", FabricSplice::constructStringRTVal(xsiGetLastLoadedScene().GetAsciiString()));

  _spliceGraph.evaluate();
  return CStatus::OK;
}

FabricSplice::DGGraph FabricSpliceBaseInterface::getSpliceGraph()
{
  return _spliceGraph;
}

CStatus FabricSpliceBaseInterface::addKLOperator(const CString &operatorName, const CString &operatorCode, const CString &operatorEntry, const XSI::CString & dgNode, const FabricCore::Variant & portMap)
{
  FabricSplice::Logging::AutoTimer globalTimer("XSI::addKLOperator");
  std::string localTimerName = (std::string("XSI::")+_spliceGraph.getName()+"::addKLOperator()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName);
  XSISPLICE_CATCH_BEGIN()

  _spliceGraph.constructKLOperator(operatorName.GetAsciiString(), operatorCode.GetAsciiString(), operatorEntry.GetAsciiString(), dgNode.GetAsciiString(), portMap);
  forceEvaluate();

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

bool FabricSpliceBaseInterface::hasKLOperator(const XSI::CString &operatorName, const XSI::CString & dgNode)
{
  FabricSplice::Logging::AutoTimer globalTimer("XSI::hasKLOperator");
  std::string localTimerName = (std::string("XSI::")+_spliceGraph.getName()+"::hasKLOperator()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName);
  XSISPLICE_CATCH_BEGIN()

  return _spliceGraph.hasKLOperator(operatorName.GetAsciiString(), dgNode.GetAsciiString());

  XSISPLICE_CATCH_END()
  return false;
}

CString FabricSpliceBaseInterface::getKLOperatorCode(const CString &operatorName)
{
  XSISPLICE_CATCH_BEGIN()

  return _spliceGraph.getKLOperatorSourceCode(operatorName.GetAsciiString());

  XSISPLICE_CATCH_END()
  return L"";
}

CStatus FabricSpliceBaseInterface::setKLOperatorCode(const CString &operatorName, const CString &operatorCode, const CString &operatorEntry)
{
  XSISPLICE_CATCH_BEGIN()

  _spliceGraph.setKLOperatorSourceCode(operatorName.GetAsciiString(), operatorCode.GetAsciiString(), operatorEntry.GetAsciiString());
  forceEvaluate();

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::setKLOperatorFile(const CString &operatorName, const CString &filename, const CString &entry)
{
  XSISPLICE_CATCH_BEGIN()

  _spliceGraph.setKLOperatorFilePath(operatorName.GetAsciiString(), filename.GetAsciiString(), entry.GetAsciiString());
  forceEvaluate();

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::setKLOperatorEntry(const CString &operatorName, const CString &operatorEntry)
{
  XSISPLICE_CATCH_BEGIN()

  _spliceGraph.setKLOperatorEntry(operatorName.GetAsciiString(), operatorEntry.GetAsciiString());
  forceEvaluate();

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::setKLOperatorIndex(const CString &operatorName, unsigned int operatorIndex)
{
  XSISPLICE_CATCH_BEGIN()

  _spliceGraph.setKLOperatorIndex(operatorName.GetAsciiString(), operatorIndex);

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::removeKLOperator(const CString &operatorName, const XSI::CString & dgNode)
{
  XSISPLICE_CATCH_BEGIN()

  _spliceGraph.removeKLOperator(operatorName.GetAsciiString(), dgNode.GetAsciiString());
  forceEvaluate();

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::storePersistenceData(CString file)
{
  FabricSplice::Logging::AutoTimer globalTimer("XSI::storePersistenceData");
  std::string localTimerName = (std::string("XSI::")+_spliceGraph.getName()+"::storePersistenceData()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName);

  CRef ref = Application().GetObjectFromID(_objectID);
  CustomOperator op(ref);
  if(op.IsValid())
  {
    XSISPLICE_CATCH_BEGIN()

    FabricSplice::PersistenceInfo info;
    info.hostAppName = FabricCore::Variant::CreateString("Softimage");
    info.hostAppVersion = FabricCore::Variant::CreateString(Application().GetVersion().GetAsciiString());
    info.filePath = FabricCore::Variant::CreateString(file.GetAsciiString());

    FabricCore::Variant dictData = _spliceGraph.getPersistenceDataDict(&info);

    op.PutParameterValue("persistenceData", CString(dictData.getJSONEncoding().getStringData()));

    XSISPLICE_CATCH_END_CSTATUS()
  }

  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::restoreFromPersistenceData(CString file)
{
  FabricSplice::Logging::AutoTimer globalTimer("XSI::restoreFromPersistenceData");
  std::string localTimerName = (std::string("XSI::")+_spliceGraph.getName()+"::restoreFromPersistenceData()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName);

  CRef ref = Application().GetObjectFromID(_objectID);
  CustomOperator op(ref);
  if(op.IsValid())
  {
    XSISPLICE_CATCH_BEGIN()

    FabricSplice::PersistenceInfo info;
    info.hostAppName = FabricCore::Variant::CreateString("Softimage");
    info.hostAppVersion = FabricCore::Variant::CreateString(Application().GetVersion().GetAsciiString());
    info.filePath = FabricCore::Variant::CreateString(file.GetAsciiString());

    CString data = op.GetParameterValue("persistenceData");
    FabricCore::Variant dictData = FabricCore::Variant::CreateFromJSON(data.GetAsciiString());
    bool dataRestored = _spliceGraph.setFromPersistenceDataDict(dictData, &info);
    if(dataRestored){
      //
    }

    XSISPLICE_CATCH_END_CSTATUS()
  }

  _parameters.clear();
  _ports.clear();
  _nbOutputPorts = 0;

  CParameterRefArray params = op.GetParameters();
  for(LONG i=0;i<params.GetCount();i++)
  {
    Parameter param(params[i]);
    CString portName = param.GetName();
    FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.GetAsciiString());
    if(!port.isValid())
      continue;
    parameterInfo info;
    info.dataType = port.getDataType();
    _parameters.insert(std::pair<std::string, parameterInfo>(portName.GetAsciiString(), info));
  }

  for(unsigned int i=0;i<_spliceGraph.getDGPortCount();i++)
  {
    FabricSplice::DGPort port = _spliceGraph.getDGPort(i);
    if(!port.isValid())
      continue;

    parameterInfo info;
    info.dataType = port.getDataType();

    if(info.paramNames.GetCount() == 0)
      continue;
    _parameters.insert(std::pair<std::string, parameterInfo>(port.getName(), info));
  }

  CRefArray portGroups = op.GetPortGroups();
  for(LONG i=0;i<portGroups.GetCount();i++)
  {
    PortGroup group(portGroups[i]);
    CString portName = group.GetName();
    FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.GetAsciiString());
    if(!port.isValid())
      continue;

    portInfo info;
    info.outPortElementsProcessed = 0;
    info.realPortName = portName;
    info.dataType = port.getDataType();
    info.isArray = port.isArray();
    if(info.isArray)
      info.dataType += L"[]";
    info.portMode = port.getMode();
    std::map<std::string, portInfo>::iterator it = _ports.find(portName.GetAsciiString());
    if(it != _ports.end())
      continue;

    CRefArray groupPorts = group.GetPorts();
    CRefArray targets;

    for(LONG j=0;j<groupPorts.GetCount();j++)
    {
      Port groupPort(groupPorts[j]);
      CString groupPortName = groupPort.GetName();
      if(groupPortName == "In"+portName+"0")
        info.realPortName = "In"+portName;
      Port xsiPort;
      CRefArray xsiPorts;
      if(groupPort.GetPortType() == siPortInput)
        xsiPorts = op.GetInputPorts();
      else
        xsiPorts = op.GetOutputPorts();
      for(LONG k=0;k<xsiPorts.GetCount();k++)
      {
        if(Port(xsiPorts[k]).GetName() == groupPortName)
        {
          xsiPort = xsiPorts[k];
          break;
        }
      }

      if(port.getMode() == FabricSplice::Port_Mode_IO)
      {
        if(groupPort.GetPortType() == siPortInput)
          continue;
      }
      if(groupPort.GetPortType() == siPortOutput)
        info.portIndices.Add(groupPort.GetIndex());
      targets.Add(xsiPort.GetTarget());
    }

    info.targets = targets.GetAsText();
    _ports.insert(std::pair<std::string, portInfo>(portName.GetAsciiString(), info));

    if(port.getMode() != FabricSplice::Port_Mode_IN)
      _nbOutputPorts += info.portIndices.GetCount();
  }

  xsiUpdateOp(getObjectID());

  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::saveToFile(CString fileName)
{
  XSISPLICE_CATCH_BEGIN()

  FabricSplice::PersistenceInfo info;
  info.hostAppName = FabricCore::Variant::CreateString("Softimage");
  info.hostAppVersion = FabricCore::Variant::CreateString(Application().GetVersion().GetAsciiString());
  info.filePath = FabricCore::Variant::CreateString(fileName.GetAsciiString());
  if(!_spliceGraph.saveToFile(fileName.GetAsciiString(), &info))
    return CStatus::Unexpected;

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::loadFromFile(CString fileName, FabricCore::Variant & scriptArgs, bool hideUI)
{
  FabricSplice::Logging::AutoTimer globalTimer("XSI::loadFromFile");
  std::string localTimerName = (std::string("XSI::")+_spliceGraph.getName()+"::loadFromFile()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName);

  XSISPLICE_CATCH_BEGIN()

  _parameters.clear();
  _ports.clear();
  _nbOutputPorts = 0;

  bool skipPicking = FabricSplice::Scripting::consumeBooleanArgument(scriptArgs, "skipPicking", false, true);

  FabricSplice::PersistenceInfo info;
  info.hostAppName = FabricCore::Variant::CreateString("Softimage");
  info.hostAppVersion = FabricCore::Variant::CreateString(Application().GetVersion().GetAsciiString());
  info.filePath = FabricCore::Variant::CreateString(fileName.GetAsciiString());

  if(!_spliceGraph.loadFromFile(fileName.GetAsciiString(), &info))
    return CStatus::Unexpected;

  bool isSoftimageFile = false;
  if(info.hostAppName.isString())
    isSoftimageFile = CString("Softimage").IsEqualNoCase(info.hostAppName.getStringData());

  std::map<CString, CString> targets;
  for(unsigned int i=0;i<_spliceGraph.getDGPortCount();i++)
  {
    CString portName = _spliceGraph.getDGPortName(i);
    FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.GetAsciiString());
    if(!port.isValid())
      continue;

    SoftimagePortType portType = SoftimagePortType_Port;
    FabricCore::Variant portTypeFlag = port.getOption("SoftimagePortType");
    if(portTypeFlag.isSInt32())
      portType = (SoftimagePortType)portTypeFlag.getSInt32();

    CString dataType = port.getDataType();
    if((dataType.IsEqualNoCase(L"Boolean") ||
      dataType.IsEqualNoCase(L"Integer") ||
      dataType.IsEqualNoCase(L"Scalar") ||
      dataType.IsEqualNoCase(L"String")) &&
      port.getMode() == FabricSplice::Port_Mode_IN &&
      !port.isArray() && (!isSoftimageFile || portType == SoftimagePortType_Parameter))
    {
      parameterInfo info;
      info.dataType = port.getDataType();
      if(dataType.IsEqualNoCase(L"Boolean"))
        info.defaultValue = CValue(port.getDefault().getBoolean());
      else if(dataType.IsEqualNoCase(L"Integer"))
        info.defaultValue = CValue((LONG)port.getDefault().getSInt32());
      else if(dataType.IsEqualNoCase(L"Scalar"))
        info.defaultValue = CValue((double)port.getDefault().getFloat64());
      else if(dataType.IsEqualNoCase(L"String"))
        info.defaultValue = CValue(CString(port.getDefault().getStringData()));
      _parameters.insert(std::pair<std::string, parameterInfo>(portName.GetAsciiString(), info));
    }
    else if(dataType.IsEqualNoCase(L"Mat44") ||
      dataType.IsEqualNoCase(L"PolygonMesh") ||
      port.getOption("ICEAttribute").isString() ||
      (!isSoftimageFile || portType == SoftimagePortType_Port || portType == SoftimagePortType_ICE))
    {
      portInfo info;
      info.realPortName = portName;
      info.dataType = dataType;
      info.isArray = port.isArray();
      if(info.isArray)
        info.dataType += L"[]";
      info.portMode = port.getMode();

      CString targetsStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, portName.GetAsciiString(), "", true).c_str();
      if(!targetsStr.IsEmpty())
      {
        CRefArray targetRefs = getCRefArrayFromCString(targetsStr);
        if(targetRefs.GetCount() > 0)
          targets.insert(std::pair<CString, CString>(portName, targetRefs.GetAsText()));
      }
    }
  }

  bool allConnected = true;
  for(unsigned int i=0;i<_spliceGraph.getDGPortCount();i++)
  {
    CString portName = _spliceGraph.getDGPortName(i);
    FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.GetAsciiString());
    if(!port.isValid())
      continue;
    CString dataType = port.getDataType();
    bool isICEAttribute = port.getOption("ICEAttribute").isString();

    SoftimagePortType portType = SoftimagePortType_Port;
    FabricCore::Variant portTypeFlag = port.getOption("SoftimagePortType");
    if(portTypeFlag.isSInt32())
      portType = (SoftimagePortType)portTypeFlag.getSInt32();

    if(((
      dataType.IsEqualNoCase(L"Boolean") ||
      dataType.IsEqualNoCase(L"Integer") ||
      dataType.IsEqualNoCase(L"Scalar") ||
      dataType.IsEqualNoCase(L"String") ||
      dataType.IsEqualNoCase(L"Mat44") ||
      dataType.IsEqualNoCase(L"PolygonMesh"))
      && (!isSoftimageFile || portType == SoftimagePortType_Port)) || isICEAttribute)
    {
      portInfo info;
      info.realPortName = portName;
      info.dataType = dataType;
      info.isArray = port.isArray();
      if(info.isArray)
        info.dataType += L"[]";
      info.portMode = port.getMode();

      CRefArray targetRefs;
      std::map<CString,CString>::iterator targetIt = targets.find(portName);
      if(targetIt != targets.end())
      {
        targetRefs = getCRefArrayFromCString(targetIt->second);
      }
      else if(!skipPicking)
      {
        CString filter = L"global";
        if(info.dataType == L"PolygonMesh")
          filter = L"polymsh";
        if(isICEAttribute)
          filter = L"geometry";

        targetRefs = PickObjectArray(
          L"Pick new target for "+portName, L"Pick next target for "+portName,
          filter, (!info.isArray || isICEAttribute) ? 1 : 0);

        if(isICEAttribute)
        {
          CString iceAttributeName = port.getOption("ICEAttribute").getStringData();
          Primitive prim(targetRefs[0]);
          Geometry geo(prim.GetGeometry());
          if(!geo.GetICEAttributeFromName(iceAttributeName).IsValid())
            targetRefs.Clear();
        }
      }

      if(!skipPicking)
      {
        if(targetRefs.GetCount() == 0)
        {
          if(dataType.IsEqualNoCase(L"Boolean") ||
          dataType.IsEqualNoCase(L"Integer") ||
          dataType.IsEqualNoCase(L"Scalar") ||
          dataType.IsEqualNoCase(L"String")) {
            continue;
          }

          allConnected = false;
          break;
        }
        if(isICEAttribute)
        {
          CString iceAttributeName = port.getOption("ICEAttribute").getStringData();
          CString errorMessage;
          CString targetDataType = getSpliceDataTypeFromICEAttribute(targetRefs, iceAttributeName, errorMessage);
          if(targetDataType != info.dataType)
          {
            xsiLogErrorFunc(errorMessage);
            allConnected = false;
            break;
          }
        }
        else
        {
          CString targetDataType = getSpliceDataTypeFromRefArray(targetRefs);
          if(info.dataType != targetDataType && info.dataType != targetDataType + "[]")
          {
            xsiLogErrorFunc("Targets '"+targetRefs.GetAsText()+"' does not match dataType '"+info.dataType+"'.");
            allConnected = false;
            break;
          }
          else
          {
            // remove the parameter if it was connected up
            if(_parameters.find(portName.GetAsciiString()) != _parameters.end())
              _parameters.erase(_parameters.find(portName.GetAsciiString()));
          }
        }
      }
      else if(targetRefs.GetCount() == 0)
      {
        allConnected = false;
      }

      info.targets = targetRefs.GetAsText();
      _ports.insert(std::pair<std::string, portInfo>(portName.GetAsciiString(), info));
      if(info.portMode != FabricSplice::Port_Mode_IN)
        _nbOutputPorts += targetRefs.GetCount() == 0 ? 1 : targetRefs.GetCount();
    }
  }

  if(_nbOutputPorts == 0)
  {
    xsiLogErrorFunc("This splice file does not provide any Softimage-compatible output port.");
    return CStatus::NotImpl;
  }

  if(!allConnected)
    return CStatus::Unexpected;

  if(!updateXSIOperator().Succeeded())
    return CStatus::Unexpected;
  else if(Application().IsInteractive() && !hideUI)
  {
    showSpliceEditor(getObjectID());
  }

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CString FabricSpliceBaseInterface::getDGPortInfo()
{
  try
  {
    std::string portInfoStr = _spliceGraph.getDGPortInfo();
    FabricCore::Variant portInfoVar = FabricCore::Variant::CreateFromJSON(portInfoStr.c_str());

    for(uint32_t i=0;i<portInfoVar.getArraySize();i++)
    {
      FabricCore::Variant * arrayElement = (FabricCore::Variant *)portInfoVar.getArrayElement(i);
      const FabricCore::Variant * portName = arrayElement->getDictValue("name");
      if(portName == NULL)
        continue;
      std::map<std::string, portInfo>::iterator it = _ports.find(portName->getStringData());
      if(it == _ports.end())
        continue;

      CString targets = it->second.targets;
      if(targets.Length() == 0)
        continue;
      arrayElement->setDictValue("targets", FabricCore::Variant::CreateString(targets.GetAsciiString()));
    }
    return CString(portInfoVar.getJSONEncoding().getStringData());
  }
  catch(FabricSplice::Exception e)
  {
    return "";
  }
}

CStatus FabricSpliceBaseInterface::disconnectForExport(XSI::CString file, Model & model)
{
  FabricSplice::Logging::AutoTimer globalTimer("XSI::disconnectForExport");
  std::string localTimerName = (std::string("XSI::")+_spliceGraph.getName()+"::disconnectForExport()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName);

  XSISPLICE_CATCH_BEGIN()

  CRef ref = Application().GetObjectFromID(_objectID);
  CustomOperator op(ref);
  if(!op.IsValid())
    return CStatus::Unexpected;

  FabricCore::Variant retargetData = FabricCore::Variant::CreateDict();

  CComAPIHandler factory;
  factory.CreateInstance(L"XSI.Factory");

  std::map<std::string, std::string> guidMap;
  for(std::map<std::string, portInfo>::iterator it = _ports.begin(); it != _ports.end(); it++)
  {
    CString targets = it->second.targets;
    CRefArray targetRefs = getCRefArrayFromCString(targets);

    for(ULONG i=0;i<targetRefs.GetCount();i++)
    {
      CRef targetRef = targetRefs[i];
      CString targetStr = targetRef.GetAsText();

      X3DObject targetX3D = getX3DObjectFromRef(targetRef);
      if(!targetX3D.IsValid())
        continue;

      if(guidMap.find(targetX3D.GetFullName().GetAsciiString()) != guidMap.end())
        continue;

      CRef propRef;
      propRef.Set(targetX3D.GetFullName()+".SpliceInfo");
      if(!propRef.IsValid())
      {
        CValueArray addpropArgs(5) ;
        addpropArgs[0] = L"SpliceInfo";
        addpropArgs[1] = targetX3D.GetFullName();
        addpropArgs[3] = L"SpliceInfo";

        CValue retVal;
        Application().ExecuteCommand( L"SIAddProp", addpropArgs, retVal );

        factory.Call(L"CreateGuid",retVal);
        CString guid = retVal;

        propRef.Set(targetX3D.GetFullName()+".SpliceInfo");
        if(!propRef.IsValid())
          continue;

        CustomProperty prop(propRef);
        prop.PutParameterValue("target", guid);
      }

      CustomProperty prop(propRef);
      CString guid = prop.GetParameterValue("target");

      guidMap.insert(std::pair<std::string, std::string>(targetX3D.GetFullName().GetAsciiString(), guid.GetAsciiString()));
    }
  }

  for(std::map<std::string, portInfo>::iterator it = _ports.begin(); it != _ports.end(); it++)
  {
    CString targets = it->second.targets;
    CRefArray targetRefs = getCRefArrayFromCString(targets);

    FabricCore::Variant portData = FabricCore::Variant::CreateDict();
    portData.setDictValue("isArray", FabricCore::Variant::CreateBoolean(it->second.isArray));
    portData.setDictValue("portMode", FabricCore::Variant::CreateSInt32((int)it->second.portMode));
    portData.setDictValue("dataType", FabricCore::Variant::CreateString(it->second.dataType.GetAsciiString()));
    FabricCore::Variant targetsData = FabricCore::Variant::CreateArray();

    for(ULONG i=0;i<targetRefs.GetCount();i++)
    {
      CRef targetRef = targetRefs[i];
      CString targetStr = targetRef.GetAsText();

      for(std::map<std::string, std::string>::reverse_iterator it=guidMap.rbegin();it!=guidMap.rend();it++)
      {
        CString first = it->first.c_str();
        first += ".";
        if(targetStr.GetSubString(0, first.Length()).IsEqualNoCase(first))
        {
          targetStr = CString(it->second.c_str()) + targetStr.GetSubString(first.Length()-1, UINT_MAX);
          break;
        }
      }

      targetsData.arrayAppend(FabricCore::Variant::CreateString(targetStr.GetAsciiString()));
    }

    portData.setDictValue("targets", targetsData);
    retargetData.setDictValue(it->first.c_str(), portData);
  }

  std::map<std::string, portInfo> newPorts;
  for(std::map<std::string, portInfo>::iterator it = _ports.begin(); it != _ports.end(); it++)
  {
    if(it->second.portMode != FabricSplice::Port_Mode_IN)
    {
      portInfo info = it->second;

      CRefArray targetRefs = getCRefArrayFromCString(it->second.targets);
      info.targets = targetRefs[0].GetAsText();
      newPorts.insert(std::pair<std::string, portInfo>(it->first, info));
      break;
    }
  }
  _ports = newPorts;

  updateXSIOperator();
  storePersistenceData(file);

  ref = Application().GetObjectFromID(_objectID);
  op = CustomOperator(ref);
  if(!op.IsValid())
    return CStatus::Unexpected;

  CString persistenceDataStr = op.GetParameterValue("persistenceData");
  FabricCore::Variant persistenceData = FabricCore::Variant::CreateFromJSON(persistenceDataStr.GetAsciiString());
  persistenceData.setDictValue("retargeting", retargetData);

  op.PutParameterValue("persistenceData", CString(persistenceData.getJSONEncoding().getStringData()));

  XSISPLICE_CATCH_END_CSTATUS()

  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::reconnectForImport(Model & model)
{
  FabricSplice::Logging::AutoTimer globalTimer("XSI::reconnectForImport");
  std::string localTimerName = (std::string("XSI::")+_spliceGraph.getName()+"::reconnectForImport()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName);

  CRef ref = Application().GetObjectFromID(_objectID);
  CustomOperator op(ref);
  if(!op.IsValid())
    return CStatus::Unexpected;

  std::map<std::string, std::string> guidMap;

  CString persistenceDataStr = op.GetParameterValue("persistenceData");
  if(!persistenceDataStr.IsEmpty())
  {
    XSISPLICE_CATCH_BEGIN()
    FabricCore::Variant persistenceData = FabricCore::Variant::CreateFromJSON(persistenceDataStr.GetAsciiString());
    const FabricCore::Variant * retargetData = persistenceData.getDictValue("retargeting");
    if(!retargetData)
      return CStatus::OK;

    CRefArray x3ds;
    x3ds.Add(model.GetRef());
    for(ULONG i=0;i<x3ds.GetCount();i++)
    {
      X3DObject x3d(x3ds[i]);
      if(!x3d.IsValid())
        continue;

      CRef propRef;
      propRef.Set(x3d.GetFullName()+".SpliceInfo");
      if(propRef.IsValid())
      {
        CustomProperty prop(propRef);
        CString guid = prop.GetParameterValue("target");
        if(guidMap.find(guid.GetAsciiString()) == guidMap.end())
          guidMap.insert(std::pair<std::string, std::string>(guid.GetAsciiString(), x3d.GetFullName().GetAsciiString()));
      }

      CRefArray children = x3d.GetChildren();
      for(ULONG j=0;j<children.GetCount();j++)
        x3ds.Add(children[j]);
    }

    std::map<std::string, portInfo> newPorts;
    for(FabricCore::Variant::DictIter keyIter(*retargetData); !keyIter.isDone(); keyIter.next())
    {
      std::string key = keyIter.getKey()->getStringData();
      const FabricCore::Variant * portVar = keyIter.getValue();
      const FabricCore::Variant * isArrayVar = portVar->getDictValue("isArray");
      if(!isArrayVar)
        continue;
      const FabricCore::Variant * portModeVar = portVar->getDictValue("portMode");
      if(!portModeVar)
        continue;
      const FabricCore::Variant * dataTypeVar = portVar->getDictValue("dataType");
      if(!dataTypeVar)
        continue;
      const FabricCore::Variant * targetsVar = portVar->getDictValue("targets");
      if(!targetsVar)
        continue;
      portInfo info;
      info.realPortName = key.c_str();
      info.isArray = isArrayVar->getBoolean();
      info.portMode = (FabricSplice::Port_Mode)portModeVar->getSInt32();
      info.dataType = dataTypeVar->getStringData();

      CRefArray targetRefs;
      for(uint32_t i=0;i<targetsVar->getArraySize();i++)
      {
        const FabricCore::Variant * targetVar = targetsVar->getArrayElement(i);
        CString targetStr = targetVar->getStringData();
        for(std::map<std::string, std::string>::reverse_iterator it=guidMap.rbegin();it!=guidMap.rend();it++)
        {
          CString first = it->first.c_str();
          first += ".";
          if(targetStr.GetSubString(0, first.Length()).IsEqualNoCase(first))
          {
            targetStr = CString(it->second.c_str()) + targetStr.GetSubString(first.Length()-1, UINT_MAX);
            break;
          }
        }
        CRef targetRef;
        targetRef.Set(targetStr);
        if(!targetRef.IsValid())
          continue;
        targetRefs.Add(targetRef);
      }
      info.targets = targetRefs.GetAsText();
      newPorts.insert(std::pair<std::string, portInfo>(key, info));
    }
    _ports = newPorts;

    XSISPLICE_CATCH_END_CSTATUS()

    return updateXSIOperator();
  }

  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::cleanupForImport(Model & model)
{
  FabricSplice::Logging::AutoTimer globalTimer("XSI::cleanupForImport");
  std::string localTimerName = (std::string("XSI::")+std::string("FabricSpliceBaseInterface::cleanupForImport()")).c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName);

  CRefArray toDelete;

  CRefArray x3ds;
  x3ds.Add(model.GetRef());
  for(ULONG i=0;i<x3ds.GetCount();i++)
  {
    X3DObject x3d(x3ds[i]);
    if(!x3d.IsValid())
      continue;

    CRef propRef;
    propRef.Set(x3d.GetFullName()+".SpliceInfo");
    if(propRef.IsValid())
      toDelete.Add(propRef);

    CRefArray children = x3d.GetChildren();
    for(ULONG j=0;j<children.GetCount();j++)
      x3ds.Add(children[j]);
  }

  if(toDelete.GetCount() > 0)
  {
    CValueArray deleteArgs(1) ;
    deleteArgs[0] = toDelete.GetAsText();
    CValue retVal;
    Application().ExecuteCommand( L"DeleteObj", deleteArgs, retVal );
  }

  return CStatus::OK;
}

bool FabricSpliceBaseInterface::processNameChange(CString prevFullPath, CString newFullPath)
{
  bool result = false;
  for(std::map<std::string, portInfo>::iterator it = _ports.begin(); it != _ports.end(); it++)
  {
    CString portName = it->first.c_str();
    FabricSplice::Port_Mode portMode = it->second.portMode;
    it->second.portIndices.Clear();
    CString targets = it->second.targets;
    CString newTargets;
    CStringArray parts = targets.Split(L",");
    for(LONG i=0;i<parts.GetCount();i++)
    {
      if(parts[i] == prevFullPath)
        parts[i] = newFullPath;
      else if(parts[i].GetSubString(0, prevFullPath.Length()+1) == prevFullPath + ".")
        parts[i] = newFullPath + parts[i].GetSubString(prevFullPath.Length());
      if(i > 0)
        newTargets += ",";
      newTargets += parts[i];
    }
    if(it->second.targets != newTargets)
    {
      it->second.targets = newTargets;
      result = true;
    }
  }
  return result;
}
