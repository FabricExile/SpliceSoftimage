
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
  _spliceGraph = FabricSplice::DGGraph("softimageGraph");
  _spliceGraph.constructDGNode("DGNode");
  _spliceGraph.setUserPointer(this);
  _objectID = UINT_MAX;
  _instances.push_back(this);
  _nbOutputPorts = 0;

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
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::updateXSIOperator");

  _currentInstance = this;

  setNeedsDeletion(false);

  // delete the previous one
  CRef oldRef = Application().GetObjectFromID(_objectID);

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
    else if(dataType == "Vec3")
    {
      xsiDataType = CValue::siVector3;
    }
    else if(dataType == "Color")
    {
      xsiDataType = CValue::siColor4f;
    }

    // else if(port.isManipulatable())
    // {
    //   it->second.paramNames.Clear();
    //   it->second.paramValues.Clear();
    //   xsiDataType = CValue::siDouble;
    //   try
    //   {
    //     FabricCore::RTVal channels = port.getAnimationChannels();
    //     FabricCore::RTVal paramNamesVal = channels.maybeGetMember("paramNames");
    //     FabricCore::RTVal paramValuesVal = channels.maybeGetMember("paramValues");
    //     for(uint32_t i=0;i<paramNamesVal.getArraySize();i++)
    //     {
    //       FabricCore::RTVal paramNameVal = paramNamesVal.getArrayElement(i);
    //       FabricCore::RTVal paramValueVal = paramValuesVal.getArrayElement(i);
    //       it->second.paramNames.Add(paramNameVal.getStringCString());
    //       it->second.paramValues.Add(paramValueVal.getFloat32());
    //     }
    //   }
    //   catch(FabricSplice::Exception e)
    //   {
    //     return CStatus::OK;
    //   }
    //   catch(FabricCore::Exception e)
    //   {
    //     return CStatus::OK;
    //   }
    // }

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
  // combo.Add(L"ScalarSliderManipulator"); combo.Add(L"ScalarSliderManipulator");
  // combo.Add(L"Vec2SliderManipulator"); combo.Add(L"Vec2SliderManipulator");
  // combo.Add(L"PositionManipulator"); combo.Add(L"PositionManipulator");
  // combo.Add(L"RotationManipulator"); combo.Add(L"RotationManipulator");
  // combo.Add(L"TransformManipulator"); combo.Add(L"TransformManipulator");
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
     dataType != "PolygonMesh" &&
     dataType != "PolygonMesh[]" &&
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
    // if(port.isManipulatable())
    //   port.setMode(FabricSplice::Port_Mode_IO);
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
  if(targetRefs.GetCount() == 0)
  {
    CString filter = L"global";
    if(portIt->second.dataType == L"PolygonMesh")
      filter = L"polymsh";
    if(portIt->second.dataType == L"Lines")
      filter = L"crvlist";
    else if(isICEAttribute)
      filter = L"geometry";

    targetRefs = PickObjectArray(
      L"Pick new target for "+portName, L"Pick next target for "+portName, 
      filter, (!portIt->second.isArray || isICEAttribute) ? 1 : 0);
  }
  if(targetRefs.GetCount() == 0)
    return CStatus::Unexpected;

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

CStatus FabricSpliceBaseInterface::transferInputParameters(OperatorContext & context)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::transferInputParameters");
  try
  {
    FabricCore::RTVal evalContext = _spliceGraph.getEvalContext();

    FabricCore::RTVal rtVal;
    CValue value;

    for(std::map<std::string, parameterInfo>::iterator it = _parameters.begin(); it != _parameters.end(); it++)
    {
      FabricSplice::DGPort port = _spliceGraph.getDGPort(it->first.c_str());
      // if(port.isManipulatable())
      // {
      //   if(it->second.paramNames.GetCount() == 0)
      //     return CStatus::OK;

      //   std::vector<float> values(it->second.paramNames.GetCount());
      //   for(ULONG i=0;i<it->second.paramNames.GetCount();i++)
      //     values[i] = (float)(double)context.GetParameterValue(it->second.paramNames[i]);

      //   port.setAnimationChannelValues(values.size(), &values[0]);
      // }
      // else
      {
        value = context.GetParameterValue(it->first.c_str());
        if(!convertBasicInputParameter(it->second.dataType, value, rtVal))
          continue;
        port.setRTVal(rtVal);
      }

      // update the evaluation context about this
      std::vector<FabricCore::RTVal> args(1);
      args[0] = FabricSplice::constructStringRTVal(it->first.c_str());
      if(port.isValid()){
        if(port.getMode() != FabricSplice::Port_Mode_OUT)
          evalContext.callMethod("", "_addDirtyInput", args.size(), &args[0]);
      }

    }
  }
  catch(FabricSplice::Exception e)
  {
    xsiLogFunc(e.what());
    return CStatus::Unexpected;
  }
  catch(FabricCore::Exception e)
  {
    xsiLogFunc(e.getDesc_cstr());
    return CStatus::Unexpected;
  }
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::transferInputPorts(OperatorContext & context)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::transferInputPorts");

  FabricCore::RTVal evalContext = _spliceGraph.getEvalContext();

  OutputPort xsiPort(context.GetOutputPort());
  std::string outPortName = xsiPort.GetGroupName().GetAsciiString();

  for(std::map<std::string, portInfo>::iterator it = _ports.begin(); it != _ports.end(); it++)
  {
    if(it->second.portMode == FabricSplice::Port_Mode_OUT)
      continue;

    FabricSplice::DGPort splicePort = _spliceGraph.getDGPort(it->first.c_str());

    // update the evaluation context about this
    std::vector<FabricCore::RTVal> args(1);
    args[0] = FabricSplice::constructStringRTVal(it->first.c_str());
    if(splicePort.isValid()){
      if(splicePort.getMode() != FabricSplice::Port_Mode_OUT)
        evalContext.callMethod("", "_addDirtyInput", args.size(), &args[0]);
    }

    LONG portIndex = 0;
    CString portIndexStr(portIndex++);

    FabricCore::Variant iceAttrName = splicePort.getOption("ICEAttribute");
    if(iceAttrName.isString())
    {
      Primitive xsiPrim((CRef)context.GetInputValue(it->second.realPortName+portIndexStr));
      Geometry xsiGeo = xsiPrim.GetGeometry();
      CString iceAttrStr = iceAttrName.getStringData();
      ICEAttribute iceAttr = xsiGeo.GetICEAttributeFromName(iceAttrStr);
      if(iceAttr.IsValid())
        convertInputICEAttribute(splicePort, it->second.dataType, iceAttr, xsiGeo);
    }
    else if(it->second.dataType == "Boolean" || 
       it->second.dataType == "Integer" || 
       it->second.dataType == "Scalar" || 
       it->second.dataType == "String")
    {
      FabricCore::RTVal rtVal;
      CValue value = context.GetInputValue(it->first.c_str()+portIndexStr);
      if(convertBasicInputParameter(it->second.dataType, value, rtVal))
        splicePort.setRTVal(rtVal);
    }
    else if(it->second.dataType == "Boolean[]" || 
       it->second.dataType == "Integer[]" || 
       it->second.dataType == "Scalar[]" || 
       it->second.dataType == "String[]")
    {
      CString singleDataType = it->second.dataType.GetSubString(0, it->second.dataType.Length()-2);
      FabricCore::RTVal arrayVal = FabricSplice::constructVariableArrayRTVal(singleDataType.GetAsciiString());
      while(true)
      {
        FabricCore::RTVal rtVal;
        CValue value = context.GetInputValue(it->first.c_str()+portIndexStr);
        if(value.IsEmpty())
          break;
        if(convertBasicInputParameter(singleDataType, value, rtVal))
          arrayVal.callMethod("", "push", 1, &rtVal);
        portIndexStr = CString(portIndex++);
      }
      splicePort.setRTVal(arrayVal);
    }
    else if(it->second.dataType == "Mat44")
    {
      KinematicState kine((CRef)context.GetInputValue(it->second.realPortName+portIndexStr));
      MATH::CMatrix4 matrix = kine.GetTransform().GetMatrix4();
      FabricCore::RTVal rtVal;
      getRTValFromCMatrix4(matrix, rtVal);
      splicePort.setRTVal(rtVal);
    }
    else if(it->second.dataType == "Mat44[]")
    {
      FabricCore::RTVal arrayVal = FabricSplice::constructVariableArrayRTVal("Mat44");
      while(true)
      {
        KinematicState kine((CRef)context.GetInputValue(it->second.realPortName+portIndexStr));
        if(!kine.IsValid())
          break;
        MATH::CMatrix4 matrix = kine.GetTransform().GetMatrix4();
        FabricCore::RTVal rtVal;
        getRTValFromCMatrix4(matrix, rtVal);
        arrayVal.callMethod("", "push", 1, &rtVal);
        portIndexStr = CString(portIndex++);
      }
      splicePort.setRTVal(arrayVal);
    }
    else if(it->second.dataType == "PolygonMesh" || it->second.dataType == "PolygonMesh[]")
    {
      bool isMeshArray = it->second.dataType == "PolygonMesh[]";

      FabricCore::RTVal mainRTVal = splicePort.getRTVal();
      std::vector<FabricCore::RTVal> rtVals;
      CRefArray meshes;
      if(isMeshArray)
      {
        while(true)
        {
          Primitive prim((CRef)context.GetInputValue(it->second.realPortName+portIndexStr));
          if(!prim.IsValid())
            break;
          meshes.Add(prim.GetGeometry().GetRef());
          if(mainRTVal.getArraySize() < portIndex)
          {
            FabricCore::RTVal rtVal = FabricSplice::constructObjectRTVal("PolygonMesh");
            mainRTVal.callMethod("", "push", 1, &rtVal);
            rtVals.push_back(rtVal);
          }
          else
          {
            FabricCore::RTVal rtVal = mainRTVal.getArrayElement(portIndex-1);
            if(!rtVal.isValid() || rtVal.isNullObject())
            {
              rtVal = FabricSplice::constructObjectRTVal("PolygonMesh");
              mainRTVal.setArrayElement(portIndex-1, rtVal);
            }
            rtVals.push_back(rtVal);
          }
          portIndexStr = CString(portIndex++);
        }
      }
      else
      { 
        Primitive prim;
        if(it->second.portMode == FabricSplice::Port_Mode_IO && it->first == outPortName)
          prim = context.GetOutputTarget();
        else
          prim = (CRef)context.GetInputValue(it->second.realPortName+portIndexStr);
        meshes.Add(prim.GetGeometry().GetRef());
        if(!mainRTVal.isValid() || mainRTVal.isNullObject())
          mainRTVal = FabricSplice::constructObjectRTVal("PolygonMesh");
        rtVals.push_back(mainRTVal);
      }

      for(size_t i=0;i<rtVals.size();i++)
      {
        FabricCore::RTVal rtVal = rtVals[i];
        PolygonMesh mesh = meshes[i];
        CGeometryAccessor acc = mesh.GetGeometryAccessor();


        // determine if we need a topology update
        bool requireTopoUpdate = false;
        bool requireShapeUpdate = true;
        if(!requireTopoUpdate)
        {
          unsigned int nbPolygons = rtVal.callMethod("UInt64", "polygonCount", 0, 0).getUInt64();
          requireTopoUpdate = nbPolygons != acc.GetPolygonCount();
        }
        if(!requireTopoUpdate)
        {
          unsigned int nbSamples = rtVal.callMethod("UInt64", "polygonPointsCount", 0, 0).getUInt64();
          requireTopoUpdate = nbSamples != acc.GetNodeCount();
        }
        requireShapeUpdate = requireShapeUpdate || requireTopoUpdate;
        if(!requireShapeUpdate && !requireTopoUpdate)
          continue;

        MATH::CVector3Array xsiPoints;
        CLongArray xsiIndices;
        if(requireTopoUpdate)
          mesh.Get(xsiPoints, xsiIndices);
        else
          xsiPoints = mesh.GetPoints().GetPositionArray();

        if(xsiPoints.GetCount() > 0 && requireShapeUpdate)
        {
          try
          {
            std::vector<FabricCore::RTVal> args(2);
            args[0] = FabricSplice::constructExternalArrayRTVal("Float64", xsiPoints.GetCount() * 3, &xsiPoints[0]);
            args[1] = FabricSplice::constructUInt32RTVal(3); // components
            rtVal.callMethod("", "setPointsFromExternalArray_d", 2, &args[0]);
            xsiPoints.Clear();
          }
          catch(FabricCore::Exception e)
          {
            xsiLogFunc(e.getDesc_cstr());
            continue;
          }
        }

        if(xsiIndices.GetCount() > 0 && requireTopoUpdate)
        {
          try
          {
            std::vector<FabricCore::RTVal> args(1);
            args[0] = FabricSplice::constructExternalArrayRTVal("UInt32", xsiIndices.GetCount(), &xsiIndices[0]);
            rtVal.callMethod("", "setTopologyFromCombinedExternalArray", 1, &args[0]);
            xsiIndices.Clear();
          }
          catch(FabricCore::Exception e)
          {
            xsiLogFunc(e.getDesc_cstr());
            continue;
          }
        }

        CRefArray uvRefs = acc.GetUVs();
        if(uvRefs.GetCount() > 0)
        {
          ClusterProperty prop(uvRefs[0]);
          CFloatArray values;
          prop.GetValues(values);

          try
          {
            std::vector<FabricCore::RTVal> args(2);
            args[0] = FabricSplice::constructExternalArrayRTVal("Float32", values.GetCount(), &values[0]);
            args[1] = FabricSplice::constructUInt32RTVal(3); // components
            rtVal.callMethod("", "setUVsFromExternalArray", 2, &args[0]);
            values.Clear();
          }
          catch(FabricCore::Exception e)
          {
            xsiLogFunc(e.getDesc_cstr());
            continue;
          }
        }

        CRefArray vertexColorRefs = acc.GetVertexColors();
        if(vertexColorRefs.GetCount() > 0)
        {
          ClusterProperty prop(vertexColorRefs[0]);
          CFloatArray values;
          prop.GetValues(values);

          try
          {
            std::vector<FabricCore::RTVal> args(2);
            args[0] = FabricSplice::constructExternalArrayRTVal("Float32", values.GetCount(), &values[0]);
            args[1] = FabricSplice::constructUInt32RTVal(4); // components
            rtVal.callMethod("", "setVertexColorsFromExternalArray", 2, &args[0]);
            values.Clear();
          }
          catch(FabricCore::Exception e)
          {
            xsiLogFunc(e.getDesc_cstr());
            continue;
          }
        }
      }
      splicePort.setRTVal(mainRTVal);
    }
    else if(it->second.dataType == "Lines" || it->second.dataType == "Lines[]")
    {
      bool isLinesArray = it->second.dataType == "Lines[]";

      FabricCore::RTVal mainRTVal = splicePort.getRTVal();
      std::vector<FabricCore::RTVal> rtVals;
      CRefArray curveLists;
      if(isLinesArray)
      {
        while(true)
        {
          Primitive prim((CRef)context.GetInputValue(it->first.c_str()+portIndexStr));
          if(!prim.IsValid())
            break;
          curveLists.Add(prim.GetGeometry().GetRef());
          if(mainRTVal.getArraySize() < portIndex)
          {
            FabricCore::RTVal rtVal = FabricSplice::constructObjectRTVal("Lines");
            mainRTVal.callMethod("", "push", 1, &rtVal);
            rtVals.push_back(rtVal);
          }
          else
          {
            FabricCore::RTVal rtVal = mainRTVal.getArrayElement(portIndex-1);
            if(!rtVal.isValid() || rtVal.isNullObject())
            {
              rtVal = FabricSplice::constructObjectRTVal("Lines");
              mainRTVal.setArrayElement(portIndex-1, rtVal);
            }
            rtVals.push_back(rtVal);
          }
          portIndexStr = CString(portIndex++);
        }
      }
      else
      { 
        Primitive prim;
        if(it->second.portMode == FabricSplice::Port_Mode_IO && it->first == outPortName)
          prim = context.GetOutputTarget();
        else
          prim = (CRef)context.GetInputValue(it->first.c_str()+portIndexStr);
        curveLists.Add(prim.GetGeometry().GetRef());
        if(!mainRTVal.isValid() || mainRTVal.isNullObject())
          mainRTVal = FabricSplice::constructObjectRTVal("Lines");
        rtVals.push_back(mainRTVal);
      }

      for(size_t i=0;i<rtVals.size();i++)
      {
        FabricCore::RTVal rtVal = rtVals[i];
        NurbsCurveList curveList = curveLists[i];

        MATH::CVector3Array xsiPoints = curveList.GetPoints().GetPositionArray();
        FabricCore::RTVal xsiPointsVal = FabricSplice::constructExternalArrayRTVal("Float64", xsiPoints.GetCount() * 3, &xsiPoints[0]);
        rtVal.callMethod("", "_setPositionsFromExternalArray_d", 1, &xsiPointsVal);

        CNurbsCurveRefArray xsiCurves = curveList.GetCurves();
        size_t nbSegments = 0;
        for(ULONG j=0;j<xsiCurves.GetCount();j++)
        {
          NurbsCurve xsiCurve = xsiCurves[j];
          CControlPointRefArray controls = xsiCurve.GetControlPoints();
          CKnotArray knots = xsiCurve.GetKnots();
          nbSegments += controls.GetCount() - 1;
          bool closed = false;
          knots.GetClosed(closed);
          if(closed)
            nbSegments++;
        }

        size_t voffset = 0;
        size_t coffset = 0;
        std::vector<uint32_t> indices(nbSegments*2);
        for(ULONG j=0;j<xsiCurves.GetCount();j++)
        {
          NurbsCurve xsiCurve = xsiCurves[j];
          CControlPointRefArray controls = xsiCurve.GetControlPoints();
          for(ULONG k=0;k<controls.GetCount()-1;k++)
          {
            indices[voffset++] = coffset++;
            indices[voffset++] = coffset;
          }
          bool closed = false;
          CKnotArray knots = xsiCurve.GetKnots();
          knots.GetClosed(closed);
          if(closed) {
            indices[voffset++] = coffset;
            indices[voffset++] = coffset - controls.GetCount() + 1;
          }
          coffset++;
        }

        FabricCore::RTVal indicesVal = FabricSplice::constructExternalArrayRTVal("UInt32", indices.size(), &indices[0]);
        rtVal.callMethod("", "_setTopologyFromExternalArray", 1, &indicesVal);
      }

      splicePort.setRTVal(mainRTVal);
    }
    else
    {
      xsiLogFunc("Skipping input port of type "+it->second.dataType);
    }

  }
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::transferOutputPort(OperatorContext & context)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::transferOutputPort");

  OutputPort xsiPort(context.GetOutputPort());

  std::map<std::string, portInfo>::iterator it = _ports.find(xsiPort.GetGroupName().GetAsciiString());
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
  else if(it->second.dataType == "PolygonMesh" || it->second.dataType == "PolygonMesh[]")
  {
    bool isArray = it->second.dataType == "PolygonMesh[]";

    FabricCore::RTVal mainRTVal = splicePort.getRTVal();
    FabricCore::RTVal rtVal;
    if(isArray)
    {
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
      if(arrayIndex <  mainRTVal.getArraySize())
      {
        rtVal = mainRTVal.getArrayElement(arrayIndex);
      }
      else
      {
        xsiLogErrorFunc("Dereferenced PolygonMesh. Please initiate the PolygonMesh array first.");
        return CStatus::Unexpected;
      }
    }
    else
    {
      rtVal = mainRTVal;
    }

    if(!rtVal.isValid() || rtVal.isNullObject())
    {
      xsiLogErrorFunc("Dereferenced PolygonMesh. Please initiate the PolygonMesh first.");
      return CStatus::Unexpected;
    }

    try
    {
      Primitive prim(context.GetOutputTarget());
      PolygonMesh mesh(prim.GetGeometry());
      CGeometryAccessor acc = mesh.GetGeometryAccessor();

      unsigned int nbPoints = rtVal.callMethod("UInt64", "pointCount", 0, 0).getUInt64();
      unsigned int nbPolygons = rtVal.callMethod("UInt64", "polygonCount", 0, 0).getUInt64();
      unsigned int nbSamples = rtVal.callMethod("UInt64", "polygonPointsCount", 0, 0).getUInt64();

      bool requireTopoUpdate = nbPolygons != acc.GetPolygonCount() || nbSamples != acc.GetNodeCount();

      MATH::CVector3Array xsiPoints;
      CLongArray xsiIndices;
      xsiPoints.Resize(nbPoints);

      if(xsiPoints.GetCount() > 0)
      {
        std::vector<FabricCore::RTVal> args(2);
        args[0] = FabricSplice::constructExternalArrayRTVal("Float64", xsiPoints.GetCount() * 3, &xsiPoints[0]);
        args[1] = FabricSplice::constructUInt32RTVal(3); // components
        rtVal.callMethod("", "getPointsAsExternalArray_d", 2, &args[0]);
      }

      if(requireTopoUpdate)
      {
        xsiIndices.Resize(nbPolygons  + nbSamples);
        FabricCore::RTVal indices = 
          FabricSplice::constructExternalArrayRTVal("UInt32", xsiIndices.GetCount(), &xsiIndices[0]);
        rtVal.callMethod("", "getTopologyAsCombinedExternalArray", 1, &indices);
      }

      if(requireTopoUpdate)
      {
        if(xsiIndices.GetCount() > 0)
          mesh.Set(xsiPoints, xsiIndices);
        else
        {
          xsiPoints.Resize(3);
          xsiIndices.Resize(4);
          xsiIndices[0] = 3;
          xsiIndices[1] = 0;
          xsiIndices[2] = 0;
          xsiIndices[3] = 0;
          mesh.Set(xsiPoints, xsiIndices);
        }
      }
      else
        mesh.GetPoints().PutPositionArray(xsiPoints);

      if(rtVal.callMethod("Boolean", "hasUVs", 0, 0).getBoolean())
      {
        CRefArray uvRefs = acc.GetUVs();
        if(uvRefs.GetCount() > 0)
        {
          ClusterProperty prop(uvRefs[0]);
          CFloatArray values(nbSamples * 3);

          try
          {
            std::vector<FabricCore::RTVal> args(2);
            args[0] = FabricSplice::constructExternalArrayRTVal("Float32", values.GetCount(), &values[0]);
            args[1] = FabricSplice::constructUInt32RTVal(3); // components
            rtVal.callMethod("", "getUVsAsExternalArray", 2, &args[0]);
            prop.SetValues(&values[0], values.GetCount() / 3);
            values.Clear();
          }
          catch(FabricCore::Exception e)

          {
            try
            {
              std::vector<FabricCore::RTVal> args(2);
              args[0] = FabricSplice::constructExternalArrayRTVal("Float32", values.GetCount(), &values[0]);
              args[1] = FabricSplice::constructUInt32RTVal(3); // components
              rtVal.callMethod("", "getUVsAsExternalArray", 2, &args[0]);
              prop.SetValues(&values[0], values.GetCount() / 3);
              values.Clear();
            }
            catch(FabricCore::Exception e)
            {
              xsiLogFunc(e.getDesc_cstr());
            }
          }
        }
      }

      if(rtVal.callMethod("Boolean", "hasVertexColors", 0, 0).getBoolean())
      {
        CRefArray vertexColorRefs = acc.GetVertexColors();
        if(vertexColorRefs.GetCount() > 0)
        {
          ClusterProperty prop(vertexColorRefs[0]);
          CFloatArray values(nbSamples * 4);

          if(values.GetCount() > 0 && values.GetCount() == prop.GetElements().GetCount() * 4)
          {
            try
            {
              std::vector<FabricCore::RTVal> args(2);
              args[0] = FabricSplice::constructExternalArrayRTVal("Float32", values.GetCount(), &values[0]);
              args[1] = FabricSplice::constructUInt32RTVal(4); // components
              rtVal.callMethod("", "getVertexColorsAsExternalArray", 2, &args[0]);
              prop.SetValues(&values[0], values.GetCount() / 4);
              values.Clear();
            }
            catch(FabricCore::Exception e)
            {
              xsiLogFunc(e.getDesc_cstr());
            }
          }
        }
      }
    }
    catch(FabricCore::Exception e)
    {
      xsiLogErrorFunc(e.getDesc_cstr());
      return CStatus::Unexpected;
    }
  }
  else if(it->second.dataType == "Lines" || it->second.dataType == "Lines[]")
  {
    bool isArray = it->second.dataType == "Lines[]";

    FabricCore::RTVal mainRTVal = splicePort.getRTVal();
    FabricCore::RTVal rtVal;
    if(isArray)
    {
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
      if(arrayIndex <  mainRTVal.getArraySize())
      {
        rtVal = mainRTVal.getArrayElement(arrayIndex);
      }
      else
      {
        xsiLogErrorFunc("Dereferenced Lines. Please initiate the Lines array first.");
        return CStatus::Unexpected;
      }
    }
    else
    {
      rtVal = mainRTVal;
    }

    if(!rtVal.isValid() || rtVal.isNullObject())
    {
      xsiLogErrorFunc("Dereferenced Lines. Please initiate the Lines first.");
      return CStatus::Unexpected;
    }

    try
    {
      Primitive prim(context.GetOutputTarget());
      NurbsCurveList curveList(prim.GetGeometry());

      unsigned int nbPoints = rtVal.callMethod("UInt64", "pointCount", 0, 0).getUInt64();
      unsigned int nbSegments = rtVal.callMethod("UInt64", "lineCount", 0, 0).getUInt64();

      MATH::CVector3Array xsiPoints;
      std::vector<uint32_t> xsiIndices;
      xsiPoints.Resize(nbPoints);
      xsiIndices.resize(nbSegments * 2);

      if(nbPoints > 0)
      {
        FabricCore::RTVal xsiPointsVal = FabricSplice::constructExternalArrayRTVal("Float64", xsiPoints.GetCount() * 3, &xsiPoints[0]);
        rtVal.callMethod("", "_getPositionsAsExternalArray_d", 1, &xsiPointsVal);
      }

      if(nbSegments > 0)
      {
        FabricCore::RTVal indices = FabricSplice::constructExternalArrayRTVal("UInt32", xsiIndices.size(), &xsiIndices[0]);
        rtVal.callMethod("", "_getTopologyAsExternalArray", 1, &indices);
      }

      size_t nbCurves = 0;
      if(nbPoints > 0 && nbSegments > 0)
      {
        nbCurves++;
        for(size_t i=2;i<xsiIndices.size();i+=2)
        {
          if(xsiIndices[i] != xsiIndices[i-1])
            nbCurves++;
        }
      }

      CNurbsCurveDataArray curveData(nbCurves);

      if(nbCurves > 0)
      {
        size_t coffset = 0;
        for(size_t i=0;i<nbCurves;i++)
        {
          curveData[i].m_siParameterization = (siKnotParameterization)1;//siNonUniformParameterization;
          curveData[i].m_lDegree = 1;

          size_t voffset = coffset + 2;
          while(voffset < xsiIndices.size())
          {
            if(xsiIndices[voffset] != xsiIndices[voffset-1])
              break;
            voffset += 2;
          }

          size_t nbVertices = (voffset - coffset) / 2 + 1;
          bool isClosed = xsiIndices[coffset] == xsiIndices[voffset-1];
          if(isClosed)
            nbVertices--;
          curveData[i].m_aControlPoints.Resize(nbVertices);
          curveData[i].m_aKnots.Resize(nbVertices + (isClosed ? 1 : 0));
          curveData[i].m_bClosed = isClosed;

          for(size_t j=0;j<nbVertices;j++)
          {
            size_t vindex;
            if(j == 0)
              vindex = xsiIndices[coffset];
            else
              vindex = xsiIndices[coffset + 1 + 2 * (j-1)];
            curveData[i].m_aControlPoints[j].Set(
              xsiPoints[vindex].GetX(),
              xsiPoints[vindex].GetY(),
              xsiPoints[vindex].GetZ(),
              1.0);
            curveData[i].m_aKnots[j] = j;
          }
          if(isClosed)
            curveData[i].m_aKnots[nbVertices] = nbVertices;

          coffset = voffset;
        }
      }

      curveList.Set(curveData, siSINurbs);
    }
    catch(FabricCore::Exception e)
    {
      xsiLogErrorFunc(e.getDesc_cstr());
      return CStatus::Unexpected;
    }
  }
  else
  {
    xsiLogFunc("Skipping output port of type "+it->second.dataType);
  }

  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::evaluate()
{
  CRef ofRef = Application().GetObjectFromID((LONG)getObjectID());

  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::evaluate");

  // setup the context
  FabricCore::RTVal context = _spliceGraph.getEvalContext();
  context.setMember("host", FabricSplice::constructStringRTVal("Softimage"));
  context.setMember("graph", FabricSplice::constructStringRTVal(ofRef.GetAsText().GetAsciiString()));
  context.setMember("time", FabricSplice::constructFloat32RTVal(CTime().GetTime( CTime::Seconds )));

  _spliceGraph.evaluate();
  return CStatus::OK;
}

bool FabricSpliceBaseInterface::requiresEvaluate(XSI::OperatorContext & context)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::requiresEvaluate");
  std::string portName = OutputPort(context.GetOutputPort()).GetName().GetAsciiString();
  if(_processedPorts.size() == 0)
  {
    _processedPorts.push_back(portName);
    return true;
  }
  if(_processedPorts[0] == portName)
  {
    _processedPorts.resize(1);
    return true;
  }
  if(_processedPorts.size() < _nbOutputPorts)
  {
    _processedPorts.push_back(portName);
    return false;
  }
  else
  {
    for(size_t i=1;i<_nbOutputPorts;i++)
    {
      if(_processedPorts[i] == portName)
      {
        _processedPorts.clear();
        _processedPorts.push_back(portName);
        return true;
      }
    }
  }

  return true;
}

FabricSplice::DGGraph FabricSpliceBaseInterface::getSpliceGraph()
{
  return _spliceGraph;
}

CStatus FabricSpliceBaseInterface::addKLOperator(const CString &operatorName, const CString &operatorCode, const CString &operatorEntry, const XSI::CString & dgNode, const FabricCore::Variant & portMap)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::addKLOperator");
  XSISPLICE_CATCH_BEGIN()

  _spliceGraph.constructKLOperator(operatorName.GetAsciiString(), operatorCode.GetAsciiString(), operatorEntry.GetAsciiString(), dgNode.GetAsciiString(), portMap);

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

bool FabricSpliceBaseInterface::hasKLOperator(const XSI::CString &operatorName, const XSI::CString & dgNode)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::hasKLOperator");
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

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::setKLOperatorFile(const CString &operatorName, const CString &filename, const CString &entry)
{
  XSISPLICE_CATCH_BEGIN()

  _spliceGraph.setKLOperatorFilePath(operatorName.GetAsciiString(), filename.GetAsciiString(), entry.GetAsciiString());

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::setKLOperatorEntry(const CString &operatorName, const CString &operatorEntry)
{
  XSISPLICE_CATCH_BEGIN()

  _spliceGraph.setKLOperatorEntry(operatorName.GetAsciiString(), operatorEntry.GetAsciiString());

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

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::storePersistenceData(CString file)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::storePersistenceData");
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
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::restoreFromPersistenceData");
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
    // if(!port.isManipulatable())
    //   continue;

    parameterInfo info;
    info.dataType = port.getDataType();
    // try
    // {
    //   FabricCore::RTVal channels = port.getAnimationChannels();
    //   FabricCore::RTVal paramNamesVal = channels.maybeGetMember("paramNames");
    //   FabricCore::RTVal paramValuesVal = channels.maybeGetMember("paramValues");
    //   for(uint32_t i=0;i<paramNamesVal.getArraySize();i++)
    //   {
    //     FabricCore::RTVal paramNameVal = paramNamesVal.getArrayElement(i);
    //     FabricCore::RTVal paramValueVal = paramValuesVal.getArrayElement(i);
    //     info.paramNames.Add(paramNameVal.getStringCString());
    //     info.paramValues.Add(paramValueVal.getFloat32());
    //     if(!Parameter(params.GetItem(info.paramNames[i])).IsValid())
    //     {
    //       info.paramNames.Clear();
    //       break;
    //     }
    //   }
    // }
    // catch(FabricCore::Exception e)
    // {
    //   continue;
    // }

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
  if(!_spliceGraph.saveToFile(fileName.GetAsciiString(), &info))
    return CStatus::Unexpected;

  XSISPLICE_CATCH_END_CSTATUS()
  return CStatus::OK;
}

CStatus FabricSpliceBaseInterface::loadFromFile(CString fileName, FabricCore::Variant & scriptArgs, bool hideUI)
{
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::loadFromFile");
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
    // else if(port.isManipulatable())
    // {
    //   parameterInfo info;
    //   info.dataType = port.getDataType();
    //   try
    //   {
    //     FabricCore::RTVal channels = port.getAnimationChannels();
    //     FabricCore::RTVal paramNamesVal = channels.maybeGetMember("paramNames");
    //     FabricCore::RTVal paramValuesVal = channels.maybeGetMember("paramValues");
    //     for(uint32_t i=0;i<paramNamesVal.getArraySize();i++)
    //     {
    //       FabricCore::RTVal paramNameVal = paramNamesVal.getArrayElement(i);
    //       FabricCore::RTVal paramValueVal = paramValuesVal.getArrayElement(i);
    //       info.paramNames.Add(paramNameVal.getStringCString());
    //       info.paramValues.Add(paramValueVal.getFloat32());
    //     }
    //     _parameters.insert(std::pair<std::string, parameterInfo>(portName.GetAsciiString(), info));
    //   }
    //   catch(FabricCore::Exception e)
    //   {
    //     continue;
    //   }
    //   catch(FabricSplice::Exception e)
    //   {
    //     continue;
    //   }
    // }
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
    showSpliceEcitor(getObjectID());
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
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::disconnectForExport");
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
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::reconnectForImport");
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
  FabricSplice::Logging::AutoTimer("FabricSpliceBaseInterface::cleanupForImport");
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
