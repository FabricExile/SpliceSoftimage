
#ifndef _FabricSpliceConversion_H_
#define _FabricSpliceConversion_H_

#include <FabricSplice.h>
#include <xsi_string.h>
#include <xsi_ref.h>
#include <xsi_matrix4.h>
#include <xsi_parameter.h>
#include <xsi_actionsource.h>
#include <xsi_iceattribute.h>
#include <xsi_geometry.h>
#include <xsi_polygonmesh.h>
#include <xsi_nurbscurve.h>
#include <xsi_nurbscurvelist.h>
#include <string>
#include <vector>

#define XSISPLICE_MEMORY_ALLOCATE(type, count) size_t valuesSize = sizeof(type) * count; type * values = (type*) malloc(valuesSize)
#define XSISPLICE_MEMORY_SETITEM(index, value) values[index] = value
#define XSISPLICE_MEMORY_GETITEM(index) values[index]
#define XSISPLICE_MEMORY_SETPORT(port) port.setArrayData(values, valuesSize)
#define XSISPLICE_MEMORY_GETPORT(port) port.getArrayData(values, valuesSize)
#define XSISPLICE_MEMORY_FREE() free(values)

double getFloat64FromRTVal(FabricCore::RTVal rtVal);
void getRTValFromCMatrix4(const XSI::MATH::CMatrix4 & value, FabricCore::RTVal & rtVal);
void getCMatrix4FromRTVal(const FabricCore::RTVal & rtVal, XSI::MATH::CMatrix4 & value);
void getRTValFromActionSource(const XSI::ActionSource & value, FabricCore::RTVal & rtVal);
void getActionSourceFromRTVal(const FabricCore::RTVal & rtVal, XSI::ActionSource & value);

XSI::CRefArray getCRefArrayFromCString(const XSI::CString & targets);
XSI::CString getSpliceDataTypeFromRefArray(const XSI::CRefArray &refs, const XSI::CString & portType = L"");
XSI::CString getSpliceDataTypeFromRef(const XSI::CRef &ref, const XSI::CString & portType = L"");
XSI::CString getSpliceDataTypeFromICEAttribute(const XSI::CRefArray &refs, const XSI::CString & iceAttrName, XSI::CString & errorMessage);
void convertInputICEAttribute(FabricSplice::DGPort & port, XSI::CString dataType, XSI::ICEAttribute & attr, XSI::Geometry & geo);
XSI::X3DObject getX3DObjectFromRef(const XSI::CRef &ref);

void convertInputPolygonMesh(XSI::PolygonMesh mesh, FabricCore::RTVal & rtVal);
void convertInputLines(XSI::NurbsCurveList curveList, FabricCore::RTVal & rtVal);

void convertOutputPolygonMesh(XSI::PolygonMesh mesh, FabricCore::RTVal & rtVal);
void convertOutputLines(XSI::NurbsCurveList curveList, FabricCore::RTVal & rtVal);

XSI::CRefArray PickObjectArray(XSI::CString firstTitle, XSI::CString nextTitle, XSI::CString filter = L"global", ULONG maxCount = 0);

XSI::CString processNameCString(XSI::CString name);

#endif
