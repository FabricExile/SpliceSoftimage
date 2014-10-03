
#include "FabricSpliceConversion.h"
#include "plugin.h"

#ifdef _WIN32
  #include <windows.h>
#endif

#include <GL/gl.h>

#include <xsi_animationsourceitem.h>
#include <xsi_application.h>
#include <xsi_fcurve.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_primitive.h>
#include <xsi_geometry.h>
#include <xsi_iceattribute.h>
#include <xsi_iceattributedataarray.h>
#include <xsi_comapihandler.h>
#include <xsi_x3dobject.h>
#include <xsi_group.h>
#include <xsi_iceattributedataarray2D.h>

#include <algorithm>

using namespace XSI;

double getFloat64FromRTVal(FabricCore::RTVal rtVal)
{
  FabricCore::RTVal::SimpleData simpleData;
  if(!rtVal.maybeGetSimpleData(&simpleData))
    return DBL_MAX;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_FLOAT32)
    return simpleData.value.float32;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_FLOAT64)
    return simpleData.value.float64;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_SINT32)
    return simpleData.value.sint32;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_UINT32)
    return simpleData.value.uint32;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_UINT8)
    return simpleData.value.uint8;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_UINT16)
    return simpleData.value.uint16;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_UINT64)
    return simpleData.value.uint64;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_SINT8)
    return simpleData.value.sint8;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_SINT16)
    return simpleData.value.sint16;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_SINT64)
    return simpleData.value.sint64;
  return DBL_MAX;
}

void getRTValFromCMatrix4(const MATH::CMatrix4 & value, FabricCore::RTVal & rtVal)
{
  rtVal = FabricSplice::constructRTVal("Mat44");
  FabricCore::RTVal row = FabricSplice::constructRTVal("Vec4");
  row.setMember("x", FabricSplice::constructFloat64RTVal(value.GetValue(0, 0)));
  row.setMember("y", FabricSplice::constructFloat64RTVal(value.GetValue(1, 0)));
  row.setMember("z", FabricSplice::constructFloat64RTVal(value.GetValue(2, 0)));
  row.setMember("t", FabricSplice::constructFloat64RTVal(value.GetValue(3, 0)));
  rtVal.setMember("row0", row);
  row.setMember("x", FabricSplice::constructFloat64RTVal(value.GetValue(0, 1)));
  row.setMember("y", FabricSplice::constructFloat64RTVal(value.GetValue(1, 1)));
  row.setMember("z", FabricSplice::constructFloat64RTVal(value.GetValue(2, 1)));
  row.setMember("t", FabricSplice::constructFloat64RTVal(value.GetValue(3, 1)));
  rtVal.setMember("row1", row);
  row.setMember("x", FabricSplice::constructFloat64RTVal(value.GetValue(0, 2)));
  row.setMember("y", FabricSplice::constructFloat64RTVal(value.GetValue(1, 2)));
  row.setMember("z", FabricSplice::constructFloat64RTVal(value.GetValue(2, 2)));
  row.setMember("t", FabricSplice::constructFloat64RTVal(value.GetValue(3, 2)));
  rtVal.setMember("row2", row);
  row.setMember("x", FabricSplice::constructFloat64RTVal(value.GetValue(0, 3)));
  row.setMember("y", FabricSplice::constructFloat64RTVal(value.GetValue(1, 3)));
  row.setMember("z", FabricSplice::constructFloat64RTVal(value.GetValue(2, 3)));
  row.setMember("t", FabricSplice::constructFloat64RTVal(value.GetValue(3, 3)));
  rtVal.setMember("row3", row);
}

void getCMatrix4FromRTVal(const FabricCore::RTVal & rtVal, MATH::CMatrix4 & value)
{
  FabricCore::RTVal row0 = rtVal.maybeGetMember("row0");
  FabricCore::RTVal row1 = rtVal.maybeGetMember("row1");
  FabricCore::RTVal row2 = rtVal.maybeGetMember("row2");
  FabricCore::RTVal row3 = rtVal.maybeGetMember("row3");
  value.SetValue(0, 0, getFloat64FromRTVal(row0.maybeGetMember("x")));
  value.SetValue(1, 0, getFloat64FromRTVal(row0.maybeGetMember("y")));
  value.SetValue(2, 0, getFloat64FromRTVal(row0.maybeGetMember("z")));
  value.SetValue(3, 0, getFloat64FromRTVal(row0.maybeGetMember("t")));
  value.SetValue(0, 1, getFloat64FromRTVal(row1.maybeGetMember("x")));
  value.SetValue(1, 1, getFloat64FromRTVal(row1.maybeGetMember("y")));
  value.SetValue(2, 1, getFloat64FromRTVal(row1.maybeGetMember("z")));
  value.SetValue(3, 1, getFloat64FromRTVal(row1.maybeGetMember("t")));
  value.SetValue(0, 2, getFloat64FromRTVal(row2.maybeGetMember("x")));
  value.SetValue(1, 2, getFloat64FromRTVal(row2.maybeGetMember("y")));
  value.SetValue(2, 2, getFloat64FromRTVal(row2.maybeGetMember("z")));
  value.SetValue(3, 2, getFloat64FromRTVal(row2.maybeGetMember("t")));
  value.SetValue(0, 3, getFloat64FromRTVal(row3.maybeGetMember("x")));
  value.SetValue(1, 3, getFloat64FromRTVal(row3.maybeGetMember("y")));
  value.SetValue(2, 3, getFloat64FromRTVal(row3.maybeGetMember("z")));
  value.SetValue(3, 3, getFloat64FromRTVal(row3.maybeGetMember("t")));
}

void getRTValFromCTransformation(const MATH::CTransformation & value, FabricCore::RTVal & rtVal)
{
  rtVal = FabricSplice::constructRTVal("Xfo");
  FabricCore::RTVal sc = FabricSplice::constructRTVal("Vec3");
  FabricCore::RTVal ori = FabricSplice::constructRTVal("Quat");
  FabricCore::RTVal oriAxis = FabricSplice::constructRTVal("Vec3");
  FabricCore::RTVal tr = FabricSplice::constructRTVal("Vec3");

  MATH::CQuaternion quat = value.GetRotationQuaternion();

  sc.setMember("x", FabricSplice::constructFloat64RTVal(value.GetSclX());
  sc.setMember("y", FabricSplice::constructFloat64RTVal(value.GetSclY()));
  sc.setMember("z", FabricSplice::constructFloat64RTVal(value.GetSclZ()));
  rtVal.setMember("sc", sc);
  oriAxis.setMember("x", FabricSplice::constructFloat64RTVal(quat.GetX()));
  oriAxis.setMember("y", FabricSplice::constructFloat64RTVal(quat.GetY()));
  oriAxis.setMember("z", FabricSplice::constructFloat64RTVal(quat.GetZ()));
  ori.setMember("v", oriAxis);
  ori.setMember("w", FabricSplice::constructFloat64RTVal(quat.GetW()));
  rtVal.setMember("ori", ori);
  tr.setMember("x", FabricSplice::constructFloat64RTVal(value.GetPosX()));
  tr.setMember("y", FabricSplice::constructFloat64RTVal(value.GetPosY());
  tr.setMember("z", FabricSplice::constructFloat64RTVal(value.GetPosZ());
  rtVal.setMember("tr", tr);

}

void getCTransformationFromRTVal(const FabricCore::RTVal & rtVal, MATH::CTransformation & value)
{
  FabricCore::RTVal sc = rtVal.maybeGetMember("sc");
  FabricCore::RTVal ori = rtVal.maybeGetMember("ori");
  FabricCore::RTVal oriAxis = ori.maybeGetMember("v");
  FabricCore::RTVal tr = rtVal.maybeGetMember("tr");

  MATH::CQuaternion quat(	getFloat64FromRTVal(ori.maybeGetMember("w")),
							getFloat64FromRTVal(oriAxis.maybeGetMember("x")), 
							getFloat64FromRTVal(oriAxis.maybeGetMember("y")), 
							getFloat64FromRTVal(oriAxis.maybeGetMember("z")));

  value.SetSclX(getFloat64FromRTVal(sc.maybeGetMember("x")));
  value.SetSclY(getFloat64FromRTVal(sc.maybeGetMember("y")));
  value.SetSclZ(getFloat64FromRTVal(sc.maybeGetMember("z")));
  value.SetRotationFromQuaternion(quat);
  value.SetPosX(getFloat64FromRTVal(tr.maybeGetMember("x")));
  value.SetPosY(getFloat64FromRTVal(tr.maybeGetMember("y")));
  value.SetPosZ(getFloat64FromRTVal(tr.maybeGetMember("z")));
}

CRefArray getCRefArrayFromCString(const CString & targets)
{
  if(targets.IsEmpty())
    return CRefArray();
  CRefArray refs;
  CStringArray parts = targets.Split(L",");
  for(LONG i=0;i<parts.GetCount();i++)
  {
    CRef ref;
    bool isIceAttribute = parts[i].GetSubString(0,1) == ":";
    if(!isIceAttribute)
      isIceAttribute = parts[i].Split(":").GetCount() > 1;
    if(isIceAttribute)
    {
      CStringArray refParts = parts[i].Split(L":");
      ref.Set(refParts[0]);
    }
    else
      ref.Set(parts[i]);
    if(!ref.IsValid())
      return CRefArray();
    refs.Add(ref);
  }
  return refs;
}

CString getSpliceDataTypeFromRefArray(const CRefArray &refs, const CString & portType)
{
  CString result;
  for(LONG i=0;i<refs.GetCount();i++)
  {
    CString singleResult = getSpliceDataTypeFromRef(refs[i], portType);
    if(!result.IsEmpty())
    {
      if(singleResult != result)
        return L"";
    }
    else
    {
      result = singleResult;
    }
  }
  if(refs.GetCount() > 1)
    result += L"[]";
  return result;
}

CString getSpliceDataTypeFromRef(const CRef &ref, const CString & portType)
{
  if(KinematicState(ref).IsValid())
	switch(portType)
    {
      case "":
      {
		return "Mat44";
	  }
      case "":
      {
		return "Xfo";
	  }
	}
  if(Primitive(ref).GetType().IsEqualNoCase("polymsh"))
    return "PolygonMesh";
  if(Primitive(ref).GetType().IsEqualNoCase("crvlist"))
    return "Lines";
  
  Parameter param(ref);
  if(param.IsValid())
  {
    CValue::DataType dataType = param.GetValueType();
    switch(dataType)
    {
      case CValue::siBool:
      {
        return "Boolean";
      }
      case CValue::siInt2:
      case CValue::siInt4:
      case CValue::siInt8:
      case CValue::siUInt2:
      case CValue::siUInt4:
      case CValue::siUInt8:
      {
        return "Integer";
      }
      case CValue::siFloat:
      case CValue::siDouble:
      {
        return "Scalar";
      }
      case CValue::siVector3:
      case CValue::siVector3f:
      case CValue::siQuaternionf:
      case CValue::siRotationf:
      {
        return "Vec3";
      }
      case CValue::siMatrix4f:
      {
        return "Mat44";
      }
      case CValue::siString:
      {
        return "String";
      }
      default:
      {
        return CString();
      }
    }
  }
  return CString();
}

CString getSpliceDataTypeFromICEAttribute(const CRefArray &refs, const CString & iceAttrName, CString & errorMessage)
{
  if(refs.GetCount() > 1)
    return L"";
  Primitive prim(refs[0]);
  if(!prim.IsValid())
    return L"";
  Geometry geo(prim.GetGeometry());
  if(!prim.IsValid())
    return L"";
  ICEAttribute attr = geo.GetICEAttributeFromName(iceAttrName);
  if(!attr.IsValid())
  {
    errorMessage = "ICE Attribute '"+refs[0].GetAsText()+"."+iceAttrName+"' does not exist.";
    return L"";
  }

  if(attr.GetStructureType() == siICENodeStructureSingle)
  {
    if(attr.GetDataType() == siICENodeDataLong)
      return L"Integer[]";
    if(attr.GetDataType() == siICENodeDataFloat)
      return L"Scalar[]";
    if(attr.GetDataType() == siICENodeDataString)
      return L"String[]";
    if(attr.GetDataType() == siICENodeDataVector2)
      return L"Vec2[]";
    if(attr.GetDataType() == siICENodeDataVector3)
      return L"Vec3[]";
    if(attr.GetDataType() == siICENodeDataQuaternion)
      return L"Quat[]";
    if(attr.GetDataType() == siICENodeDataMatrix44)
      return L"Mat44[]";
    if(attr.GetDataType() == siICENodeDataColor4)
      return L"Color[]";

    errorMessage = "ICE Attribute '"+refs[0].GetAsText()+"."+iceAttrName+"' uses an unsupported data type.";
  }
  else if(attr.GetStructureType() == siICENodeStructureArray)
  {
    if(attr.GetDataType() == siICENodeDataLong)
      return L"Integer[][]";
    if(attr.GetDataType() == siICENodeDataFloat)
      return L"Scalar[][]";
    if(attr.GetDataType() == siICENodeDataString)
      return L"String[][]";
    if(attr.GetDataType() == siICENodeDataVector2)
      return L"Vec2[][]";
    if(attr.GetDataType() == siICENodeDataVector3)
      return L"Vec3[][]";
    if(attr.GetDataType() == siICENodeDataQuaternion)
      return L"Quat[][]";
    if(attr.GetDataType() == siICENodeDataMatrix44)
      return L"Mat44[][]";
    if(attr.GetDataType() == siICENodeDataColor4)
      return L"Color[][]";

    errorMessage = "ICE Attribute '"+refs[0].GetAsText()+"."+iceAttrName+"' uses an unsupported data type.";
  }
  else
  {
    errorMessage = "ICE Attribute '"+refs[0].GetAsText()+"."+iceAttrName+"' uses an unsupported structure type.";
  }
  return L"";
}

void convertInputICEAttribute(FabricSplice::DGPort & port, CString dataType, ICEAttribute & attr, XSI::Geometry & geo)
{
  if(attr.GetStructureType() == siICENodeStructureSingle)
  {
    if(attr.GetDataType() == siICENodeDataLong && dataType == L"Integer[]")
    {
      CICEAttributeDataArrayLong xsiValues;
      attr.GetDataArray(xsiValues);
      std::vector<int32_t> portValues(xsiValues.GetCount());
      for(size_t i=0;i<portValues.size();i++)
        portValues[i] = xsiValues[i];
      port.setArrayData(&portValues[0], sizeof(int32_t) * portValues.size());
    }
    else if(attr.GetDataType() == siICENodeDataFloat && dataType == L"Scalar[]")
    {
      CICEAttributeDataArrayFloat xsiValues;
      attr.GetDataArray(xsiValues);
      port.setArrayData(&xsiValues[0], sizeof(float) * xsiValues.GetCount());
    }
    else if(attr.GetDataType() == siICENodeDataString && dataType == L"String[]")
    {
      CICEAttributeDataArrayString xsiValues;
      attr.GetDataArray(xsiValues);
      FabricCore::RTVal value = FabricSplice::constructRTVal("String[]");
      value.setArraySize(xsiValues.GetCount());
      for(ULONG i=0;i<xsiValues.GetCount();i++)
        value.setArrayElement(i, FabricSplice::constructStringRTVal(xsiValues[i].GetAsciiString()));
      port.setRTVal(value);
    }
    else if(attr.GetDataType() == siICENodeDataVector2 && dataType == L"Vec2[]")
    {
      CICEAttributeDataArrayVector2f xsiValues;
      attr.GetDataArray(xsiValues);
      port.setArrayData(&xsiValues[0], sizeof(float) * 2 * xsiValues.GetCount());
    }
    else if(attr.GetDataType() == siICENodeDataVector3 && dataType == L"Vec3[]")
    {
      CICEAttributeDataArrayVector3f xsiValues;
      attr.GetDataArray(xsiValues);
      port.setArrayData(&xsiValues[0], sizeof(float) * 3 * xsiValues.GetCount());
    }
    else if(attr.GetDataType() == siICENodeDataQuaternion && dataType == L"Quat[]")
    {
      CICEAttributeDataArrayQuaternionf xsiValues;
      attr.GetDataArray(xsiValues);
      std::vector<float> portValues(xsiValues.GetCount()*4);
      size_t offset = 0;
      for(LONG i=0;i<xsiValues.GetCount();i++)
      {
        portValues[offset++] = xsiValues[i].GetX();
        portValues[offset++] = xsiValues[i].GetY();
        portValues[offset++] = xsiValues[i].GetZ();
        portValues[offset++] = xsiValues[i].GetW();
      }
      port.setArrayData(&portValues[0], sizeof(float) * portValues.size());
    }
    else if(attr.GetDataType() == siICENodeDataMatrix44 && dataType == L"Mat44[]")
    {
      CICEAttributeDataArrayMatrix4f xsiValues;
      attr.GetDataArray(xsiValues);
      std::vector<float> portValues(xsiValues.GetCount()*16);
      size_t offset = 0;
      for(LONG i=0;i<xsiValues.GetCount();i++)
      {
        portValues[offset++] = xsiValues[i].GetValue(0, 0);
        portValues[offset++] = xsiValues[i].GetValue(1, 0);
        portValues[offset++] = xsiValues[i].GetValue(2, 0);
        portValues[offset++] = xsiValues[i].GetValue(3, 0);
        portValues[offset++] = xsiValues[i].GetValue(0, 1);
        portValues[offset++] = xsiValues[i].GetValue(1, 1);
        portValues[offset++] = xsiValues[i].GetValue(2, 1);
        portValues[offset++] = xsiValues[i].GetValue(3, 1);
        portValues[offset++] = xsiValues[i].GetValue(0, 2);
        portValues[offset++] = xsiValues[i].GetValue(1, 2);
        portValues[offset++] = xsiValues[i].GetValue(2, 2);
        portValues[offset++] = xsiValues[i].GetValue(3, 2);
        portValues[offset++] = xsiValues[i].GetValue(0, 3);
        portValues[offset++] = xsiValues[i].GetValue(1, 3);
        portValues[offset++] = xsiValues[i].GetValue(2, 3);
        portValues[offset++] = xsiValues[i].GetValue(3, 3);
      }
      port.setArrayData(&portValues[0], sizeof(float) * portValues.size());
    }
    else if(attr.GetDataType() == siICENodeDataColor4 && dataType == L"Color[]")
    {
      CICEAttributeDataArrayColor4f xsiValues;
      attr.GetDataArray(xsiValues);
      port.setArrayData(&xsiValues[0], sizeof(float) * 4 * xsiValues.GetCount());
    }
  }
  else if(attr.GetStructureType() == siICENodeStructureArray)
  {
    if(attr.GetDataType() == siICENodeDataLong && dataType == L"Integer[][]")
    {
      CICEAttributeDataArray2DLong xsiValues;
      CICEAttributeDataArrayLong xsiSubValues;
      attr.GetDataArray2D(xsiValues);

      FabricCore::RTVal value = FabricSplice::constructRTVal("Integer[][]");
      value.setArraySize(xsiValues.GetCount());

      for(ULONG i=0;i<xsiValues.GetCount();i++)
      {
        xsiValues.GetSubArray(i, xsiSubValues);

        std::vector<int32_t> portValues(xsiSubValues.GetCount());
        for(size_t j=0;j<portValues.size();j++)
          portValues[j] = xsiSubValues[j];

        FabricCore::RTVal element = value.getArrayElement(i);
        element.setArraySize(xsiSubValues.GetCount());
        FabricCore::RTVal data = element.callMethod("Data", "data", 0, 0);
        memcpy(data.getData(), &portValues[0], sizeof(int32_t) * xsiSubValues.GetCount());

        value.setArrayElement(i, element);
      }

      port.setRTVal(value);
    }
    else if(attr.GetDataType() == siICENodeDataFloat && dataType == L"Scalar[][]")
    {
      CICEAttributeDataArray2DFloat xsiValues;
      CICEAttributeDataArrayFloat xsiSubValues;
      attr.GetDataArray2D(xsiValues);

      FabricCore::RTVal value = FabricSplice::constructRTVal("Scalar[][]");
      value.setArraySize(xsiValues.GetCount());

      for(ULONG i=0;i<xsiValues.GetCount();i++)
      {
        xsiValues.GetSubArray(i, xsiSubValues);

        FabricCore::RTVal element = value.getArrayElement(i);
        element.setArraySize(xsiSubValues.GetCount());
        FabricCore::RTVal data = element.callMethod("Data", "data", 0, 0);
        memcpy(data.getData(), &xsiSubValues[0], sizeof(float) * xsiSubValues.GetCount());

        value.setArrayElement(i, element);
      }

      port.setRTVal(value);
    }
    else if(attr.GetDataType() == siICENodeDataString && dataType == L"String[][]")
    {
      CICEAttributeDataArray2DString xsiValues;
      CICEAttributeDataArrayString xsiSubValues;
      attr.GetDataArray2D(xsiValues);

      FabricCore::RTVal value = FabricSplice::constructRTVal("String[][]");
      value.setArraySize(xsiValues.GetCount());

      for(ULONG i=0;i<xsiValues.GetCount();i++)
      {
        xsiValues.GetSubArray(i, xsiSubValues);

        FabricCore::RTVal element = value.getArrayElement(i);
        element.setArraySize(xsiSubValues.GetCount());
        for(ULONG j=0;j<xsiSubValues.GetCount();j++)
          element.setArrayElement(j, FabricSplice::constructStringRTVal(xsiSubValues[j].GetAsciiString()));

        value.setArrayElement(i, element);
      }

      port.setRTVal(value);
    }
    else if(attr.GetDataType() == siICENodeDataVector2 && dataType == L"Vec2[][]")
    {
      CICEAttributeDataArray2DVector2f xsiValues;
      CICEAttributeDataArrayVector2f xsiSubValues;
      attr.GetDataArray2D(xsiValues);

      FabricCore::RTVal value = FabricSplice::constructRTVal("Vec2[][]");
      value.setArraySize(xsiValues.GetCount());

      for(ULONG i=0;i<xsiValues.GetCount();i++)
      {
        xsiValues.GetSubArray(i, xsiSubValues);

        FabricCore::RTVal element = value.getArrayElement(i);
        element.setArraySize(xsiSubValues.GetCount());
        FabricCore::RTVal data = element.callMethod("Data", "data", 0, 0);
        memcpy(data.getData(), &xsiSubValues[0], sizeof(float) * 2 * xsiSubValues.GetCount());

        value.setArrayElement(i, element);
      }

      port.setRTVal(value);
    }
    else if(attr.GetDataType() == siICENodeDataVector3 && dataType == L"Vec3[][]")
    {
      CICEAttributeDataArray2DVector3f xsiValues;
      CICEAttributeDataArrayVector3f xsiSubValues;
      attr.GetDataArray2D(xsiValues);

      FabricCore::RTVal value = FabricSplice::constructRTVal("Vec3[][]");
      value.setArraySize(xsiValues.GetCount());

      for(ULONG i=0;i<xsiValues.GetCount();i++)
      {
        xsiValues.GetSubArray(i, xsiSubValues);

        FabricCore::RTVal element = value.getArrayElement(i);
        element.setArraySize(xsiSubValues.GetCount());
        FabricCore::RTVal data = element.callMethod("Data", "data", 0, 0);
        memcpy(data.getData(), &xsiSubValues[0], sizeof(float) * 3 * xsiSubValues.GetCount());

        value.setArrayElement(i, element);
      }

      port.setRTVal(value);
    }
    else if(attr.GetDataType() == siICENodeDataQuaternion && dataType == L"Quat[][]")
    {
      CICEAttributeDataArray2DQuaternionf xsiValues;
      CICEAttributeDataArrayQuaternionf xsiSubValues;
      attr.GetDataArray2D(xsiValues);

      FabricCore::RTVal value = FabricSplice::constructRTVal("Quat[][]");
      value.setArraySize(xsiValues.GetCount());

      for(ULONG i=0;i<xsiValues.GetCount();i++)
      {
        xsiValues.GetSubArray(i, xsiSubValues);

        size_t offset = 0;
        std::vector<float> portValues(xsiSubValues.GetCount() * 4);
        for(size_t j=0;j<portValues.size();j++)
        {
          portValues[offset++] = xsiSubValues[j].GetX();
          portValues[offset++] = xsiSubValues[j].GetY();
          portValues[offset++] = xsiSubValues[j].GetZ();
          portValues[offset++] = xsiSubValues[j].GetW();
        }

        FabricCore::RTVal element = value.getArrayElement(i);
        element.setArraySize(xsiSubValues.GetCount());
        FabricCore::RTVal data = element.callMethod("Data", "data", 0, 0);
        memcpy(data.getData(), &portValues[0], sizeof(float) * 4 * xsiSubValues.GetCount());

        value.setArrayElement(i, element);
      }

      port.setRTVal(value);
    }
    else if(attr.GetDataType() == siICENodeDataMatrix44 && dataType == L"Mat44[][]")
    {
      CICEAttributeDataArray2DMatrix4f xsiValues;
      CICEAttributeDataArrayMatrix4f xsiSubValues;
      attr.GetDataArray2D(xsiValues);

      FabricCore::RTVal value = FabricSplice::constructRTVal("Mat44[][]");
      value.setArraySize(xsiValues.GetCount());

      for(ULONG i=0;i<xsiValues.GetCount();i++)
      {
        xsiValues.GetSubArray(i, xsiSubValues);

        size_t offset = 0;
        std::vector<float> portValues(xsiSubValues.GetCount() * 16);
        for(size_t j=0;j<portValues.size();j++)
        {
          portValues[offset++] = xsiSubValues[j].GetValue(0, 0);
          portValues[offset++] = xsiSubValues[j].GetValue(1, 0);
          portValues[offset++] = xsiSubValues[j].GetValue(2, 0);
          portValues[offset++] = xsiSubValues[j].GetValue(3, 0);
          portValues[offset++] = xsiSubValues[j].GetValue(0, 1);
          portValues[offset++] = xsiSubValues[j].GetValue(1, 1);
          portValues[offset++] = xsiSubValues[j].GetValue(2, 1);
          portValues[offset++] = xsiSubValues[j].GetValue(3, 1);
          portValues[offset++] = xsiSubValues[j].GetValue(0, 2);
          portValues[offset++] = xsiSubValues[j].GetValue(1, 2);
          portValues[offset++] = xsiSubValues[j].GetValue(2, 2);
          portValues[offset++] = xsiSubValues[j].GetValue(3, 2);
          portValues[offset++] = xsiSubValues[j].GetValue(0, 3);
          portValues[offset++] = xsiSubValues[j].GetValue(1, 3);
          portValues[offset++] = xsiSubValues[j].GetValue(2, 3);
          portValues[offset++] = xsiSubValues[j].GetValue(3, 3);
        }

        FabricCore::RTVal element = value.getArrayElement(i);
        element.setArraySize(xsiSubValues.GetCount());
        FabricCore::RTVal data = element.callMethod("Data", "data", 0, 0);
        memcpy(data.getData(), &portValues[0], sizeof(float) * 16 * xsiSubValues.GetCount());

        value.setArrayElement(i, element);
      }

      port.setRTVal(value);
    }
    else if(attr.GetDataType() == siICENodeDataColor4 && dataType == L"Color[][]")
    {
      CICEAttributeDataArray2DColor4f xsiValues;
      CICEAttributeDataArrayColor4f xsiSubValues;
      attr.GetDataArray2D(xsiValues);

      FabricCore::RTVal value = FabricSplice::constructRTVal("Color[][]");
      value.setArraySize(xsiValues.GetCount());

      for(ULONG i=0;i<xsiValues.GetCount();i++)
      {
        xsiValues.GetSubArray(i, xsiSubValues);

        FabricCore::RTVal element = value.getArrayElement(i);
        element.setArraySize(xsiSubValues.GetCount());
        FabricCore::RTVal data = element.callMethod("Data", "data", 0, 0);
        memcpy(data.getData(), &xsiSubValues[0], sizeof(float) * 4 * xsiSubValues.GetCount());

        value.setArrayElement(i, element);
      }

      port.setRTVal(value);
    }
  }
}

X3DObject getX3DObjectFromRef(const CRef &ref)
{
  X3DObject x3d(ref);
  if(x3d.IsValid())
    return x3d;

  SIObject obj(ref);
  if(!obj.IsValid())
    return X3DObject();
  return getX3DObjectFromRef(obj.GetParent());
}

CRef filterX3DObjectPickedRef(CRef ref, CString filter)
{
  CRef target = ref;
  if(filter.IsEqualNoCase(L"global"))
    target = X3DObject(target).GetKinematics().GetGlobal().GetRef();
  else if(filter.IsEqualNoCase(L"local"))
    target = X3DObject(target).GetKinematics().GetLocal().GetRef();
  else if(filter.IsEqualNoCase(L"polymsh") || filter.IsEqualNoCase(L"geometry"))
    target = X3DObject(target).GetActivePrimitive().GetRef();
  else if(filter.IsEqualNoCase(L"crvlist") || filter.IsEqualNoCase(L"geometry"))
    target = X3DObject(target).GetActivePrimitive().GetRef();
  return target;
}

CRefArray PickSingleObject(CString title, CString filter)
{
  CValue returnVal;
  CValueArray args(2);
  args[0] = title;
  args[1] = "End Picking (Skip)";
  Application().ExecuteCommand(L"PickObject", args, returnVal);
  CValueArray results = returnVal;
  CRef target = results[2];
  CRefArray targets;
  if(target.IsValid())
  {
    if(X3DObject(target).IsValid())
    {
      target = filterX3DObjectPickedRef(target, filter);
      targets.Add(target);
    }
    else if(Group(target).IsValid())
    {
      CRefArray members = Group(target).GetMembers();
      for(LONG i=0;i<members.GetCount();i++)
      {
        X3DObject member(members[i]);
        if(!member.IsValid())
          continue;
        target = filterX3DObjectPickedRef(members[i], filter);
        targets.Add(target);
      }
    }
    else if(Primitive(target).IsValid())
    {
      if(Primitive(target).GetType().IsEqualNoCase(L"polymsh"))
        targets.Add(target);
      if(Primitive(target).GetType().IsEqualNoCase(L"crvlist"))
        targets.Add(target);
    }
    else if(Parameter(target).IsValid())
    {
      targets.Add(target);
    }
  }
  return targets;
}

CRefArray PickObjectArray(CString firstTitle, CString nextTitle, CString filter, ULONG maxCount)
{
  CRefArray refs;
  CRefArray newRefs = PickSingleObject(firstTitle, filter);
  while(newRefs.GetCount() > 0)
  {
    for(LONG i=0;i<newRefs.GetCount();i++)
    {
      bool unique = true;
      CRef ref = newRefs[i];
      for(LONG j=0;j<refs.GetCount();j++)
      {
        if(refs[j].GetAsText() == ref.GetAsText())
        {
          unique = false;
          break;
        }
      }
      if(unique)
      {
        refs.Add(ref);
        if(maxCount == refs.GetCount())
          break;
      }
    }
    if(maxCount == refs.GetCount())
      break;
    newRefs = PickSingleObject(nextTitle, filter);
  }
  return refs;
}

CString processNameCString(CString name)
{
  CString result;
  for(LONG i=0;i<name.Length();i++)
  {
    char c = name.GetAsciiString()[i];
    if(c == ' ')
    {
      result += '_';
      continue;
    }
    if(!isalpha(c) && !isdigit(c) && c != '_')
      continue;
    result += c;
  }
  return result;
}
