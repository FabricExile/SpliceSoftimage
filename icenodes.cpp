
#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>
#include <xsi_pluginregistrar.h>

#include <xsi_command.h>
#include <xsi_argument.h>
#include <xsi_factory.h>
#include <xsi_longarray.h>
#include <xsi_doublearray.h>
#include <xsi_math.h>
#include <xsi_customproperty.h>
#include <xsi_ppglayout.h>
#include <xsi_menu.h>
#include <xsi_selection.h>
#include <xsi_project.h>
#include <xsi_utils.h>
#include <xsi_customoperator.h>
#include <xsi_operatorcontext.h>

#include <xsi_icenodecontext.h>
#include <xsi_icenodedef.h>
#include <xsi_command.h>
#include <xsi_factory.h>
#include <xsi_longarray.h>
#include <xsi_doublearray.h>
#include <xsi_math.h>
#include <xsi_vector2f.h>
#include <xsi_vector3f.h>
#include <xsi_vector4f.h>
#include <xsi_matrix3f.h>
#include <xsi_matrix4f.h>
#include <xsi_color4f.h>
#include <xsi_shape.h>
#include <xsi_iceportstate.h>
#include <xsi_indexset.h>
#include <xsi_dataarray.h>
#include <xsi_dataarray2D.h>
#include <xsi_icenode.h>

// project includes
#include "plugin.h"
#include "icenodes.h"
#include "FabricSpliceBaseInterface.h"

using namespace XSI;

enum IDs
{
  spliceGetData_ID_IN_trigger = 0,
  spliceGetData_ID_IN_reference = 1,
  spliceGetData_ID_IN_klType = 2,
  spliceGetData_ID_IN_element = 3,
  spliceGetData_ID_G_100 = 100,
  spliceGetData_ID_OUT_Result = 200,
  spliceGetData_ID_TYPE_CNS = 400,
  spliceGetData_ID_STRUCT_CNS,
  spliceGetData_ID_CTXT_CNS,
  spliceGetData_ID_UNDEF = ULONG_MAX
};

using namespace XSI;

CStatus Register_spliceGetData( PluginRegistrar& in_reg )
{
  CStatus st;

  ICENodeDef nodeDef;
  PluginItem nodeItem;

  int supportedDataTypes = 
    siICENodeDataLong |
    siICENodeDataFloat |
    siICENodeDataVector3 |
    siICENodeDataColor4;

  int supportedTriggerTypes = supportedDataTypes |
    siICENodeDataQuaternion |
    siICENodeDataMatrix33 |
    siICENodeDataMatrix44 |
    siICENodeDataGeometry |
    siICENodeDataString;

  // ---------------------------------------------------------------------------------------------

  // nodeDef = Application().GetFactory().CreateICENodeDef(L"spliceGetDataSingle",L"spliceGetDataSingle");

  // st = nodeDef.PutColor(39,168,223);
  // st.AssertSucceeded( ) ;

  // st = nodeDef.PutThreadingModel(siICENodeSingleThreading);
  // st.AssertSucceeded( ) ;

  // // Add input ports and groups.
  // st = nodeDef.AddPortGroup(spliceGetData_ID_G_100);
  // st.AssertSucceeded( ) ;
  // st = nodeDef.AddInputPort(spliceGetData_ID_IN_trigger, spliceGetData_ID_G_100, supportedTriggerTypes, siICENodeStructureAny, siICENodeContextAny, L"trigger", L"trigger", L"", CValue(), CValue(), spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF);
  // st.AssertSucceeded( ) ;
  // st = nodeDef.AddInputPort(spliceGetData_ID_IN_reference, spliceGetData_ID_G_100, siICENodeDataString, siICENodeStructureSingle, siICENodeContextSingleton, L"reference", L"reference", L"", CString(), CString(), spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF);
  // st.AssertSucceeded( ) ;
  // st = nodeDef.AddInputPort(spliceGetData_ID_IN_klType, spliceGetData_ID_G_100, siICENodeDataString, siICENodeStructureSingle, siICENodeContextSingleton, L"klType", L"klType", L"", CString(), CString(), spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF);
  // st.AssertSucceeded( ) ;
  // st = nodeDef.AddOutputPort(spliceGetData_ID_OUT_Result, supportedDataTypes, siICENodeStructureAny, siICENodeContextSingleton, L"result", L"result", spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF);
  // st.AssertSucceeded( ) ;

  // nodeItem = in_reg.RegisterICENode(nodeDef);
  // nodeItem.PutCategories(L"Fabric Engine");

  // ---------------------------------------------------------------------------------------------

  nodeDef = Application().GetFactory().CreateICENodeDef(L"spliceGetData",L"spliceGetData");

  st = nodeDef.PutColor(39,168,223);
  st.AssertSucceeded( ) ;

  st = nodeDef.PutThreadingModel(siICENodeSingleThreading);
  st.AssertSucceeded( ) ;

  // Add input ports and groups.
  st = nodeDef.AddPortGroup(spliceGetData_ID_G_100);
  st.AssertSucceeded( ) ;
  st = nodeDef.AddInputPort(spliceGetData_ID_IN_trigger, spliceGetData_ID_G_100, supportedTriggerTypes, siICENodeStructureAny, siICENodeContextAny, L"trigger", L"trigger", L"", CValue(), CValue(), spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF);
  st.AssertSucceeded( ) ;
  st = nodeDef.AddInputPort(spliceGetData_ID_IN_reference, spliceGetData_ID_G_100, siICENodeDataString, siICENodeStructureSingle, siICENodeContextSingleton, L"reference", L"reference", L"", CString(), CString(), spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF);
  st.AssertSucceeded( ) ;
  st = nodeDef.AddInputPort(spliceGetData_ID_IN_klType, spliceGetData_ID_G_100, siICENodeDataString, siICENodeStructureSingle, siICENodeContextSingleton, L"klType", L"klType", L"", CString(), CString(), spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF, spliceGetData_ID_UNDEF);
  st.AssertSucceeded( ) ;
  st = nodeDef.AddInputPort(spliceGetData_ID_IN_element, spliceGetData_ID_G_100, supportedDataTypes, siICENodeStructureAny, siICENodeContextAny, L"element", L"element", L"", CValue(), CValue(), spliceGetData_ID_TYPE_CNS, spliceGetData_ID_STRUCT_CNS, spliceGetData_ID_CTXT_CNS);
  st.AssertSucceeded( ) ;
  st = nodeDef.AddOutputPort(spliceGetData_ID_OUT_Result, supportedDataTypes, siICENodeStructureAny, siICENodeContextAny/*siICENodeContextComponent0D*/, L"result", L"result", spliceGetData_ID_TYPE_CNS, spliceGetData_ID_STRUCT_CNS, spliceGetData_ID_CTXT_CNS);
  st.AssertSucceeded( ) ;

  nodeItem = in_reg.RegisterICENode(nodeDef);
  nodeItem.PutCategories(L"Fabric Engine");

  return CStatus::OK;
}

SICALLBACK spliceGetData_BeginEvaluate(ICENodeContext& in_ctxt)
{
  XSI::siICENodeDataType dataType;
  XSI::siICENodeStructureType dataStruct;
  XSI::siICENodeContextType dataContext;
  in_ctxt.GetPortInfo( spliceGetData_ID_OUT_Result, dataType, dataStruct, dataContext );

  CDataArrayString referenceArray(in_ctxt, spliceGetData_ID_IN_reference);
  CString reference = referenceArray[0];
  CDataArrayString klTypeArray(in_ctxt, spliceGetData_ID_IN_klType);
  CString klType = klTypeArray[0];

  // invalidate the stored rtval
  CValue udVal = in_ctxt.GetUserData();
  iceNodeUD * ud = (iceNodeUD*)(CValue::siPtrType)udVal;
  FabricSpliceBaseInterface * interf = FabricSpliceBaseInterface::getInstanceByObjectID(ud->objectID);
  ud->interf = interf;
  if(!interf)
    return CStatus::OK;
  interf->setICENodeRTVal(FabricCore::RTVal());

  if(reference.Length() == 0)
  {
    Application().LogMessage(CString("spliceGetData") + " uses an empty reference string.", siErrorMsg );
    return CStatus::OK;
  }

  CStringArray referenceParts = reference.Split(".");
  if(referenceParts.GetCount() <= 1)
  {
    Application().LogMessage(CString("spliceGetData") + "'s reference contains less than two parts.", siErrorMsg );
    return CStatus::OK;
  }

  if(klType.Length() == 0)
  {
    Application().LogMessage(CString("spliceGetData") + " uses an empty klType string.", siErrorMsg );
    return CStatus::OK;
  }

  try
  {
    FabricCore::RTVal handleVal = FabricSplice::constructObjectRTVal("SingletonHandle");
    FabricCore::RTVal keyVal = FabricSplice::constructStringRTVal(referenceParts[0].GetAsciiString());
    FabricCore::RTVal singletonVal = handleVal.callMethod(klType.GetAsciiString(), "getObject", 1, &keyVal);
    if(!singletonVal.isValid())
    {
      Application().LogMessage(CString("spliceGetData") + "'s reference refers to a singleton, which does not exist: " + referenceParts[0], siErrorMsg );
      return CStatus::OK;
    }

    if(singletonVal.isNullObject())
    {
      Application().LogMessage(CString("spliceGetData") + "'s reference refers to a singleton, which does not exist: " + referenceParts[0], siErrorMsg );
      return CStatus::OK;
    }

    for(unsigned int i=1;i<referenceParts.GetCount();i++)
    {
      FabricCore::RTVal memberVal = singletonVal.maybeGetMember(referenceParts[i].GetAsciiString());
      if(!memberVal.isValid())
      {
        Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " to a member, which does not exist: " + referenceParts[i], siErrorMsg );
        return CStatus::OK;
      }
      singletonVal = memberVal;
    }

    std::string klDataType = singletonVal.getTypeName().getStringCString();
    std::string klBaseDataType = klDataType;
    std::string klBrackets = klDataType;
    int klArrayDimensions = 0;
    int klBracketPos = klBrackets.find('[');
    while(klBracketPos != std::string::npos)
    {
      if(klBrackets[0] != ']')
        klBaseDataType = klBrackets.substr(0, klBracketPos);
      klBrackets = klBrackets.substr(klBracketPos+1, klBrackets.length());
      klBracketPos = klBrackets.find('[');
      klArrayDimensions++;
    }

    if(klArrayDimensions == 1)
    {
      if(dataStruct == siICENodeStructureSingle && dataContext == siICENodeContextSingleton)
      {
        Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " refers to a " + CString((LONG)klArrayDimensions) + " dimensional array, but the connected ICE ports are singleton of single value.", siErrorMsg );
        return CStatus::OK;
      }
      else if(dataStruct == siICENodeStructureArray && dataContext != siICENodeContextSingleton)
      {
        Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " refers to a " + CString((LONG)klArrayDimensions) + " dimensional array, but the connected ICE ports are a multi of array value.", siErrorMsg );
        return CStatus::OK;
      }
    }
    else if(klArrayDimensions == 2)
    {
      if(dataContext == siICENodeContextSingleton || dataStruct != siICENodeStructureArray)
      {
        Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " refers to a " + CString((LONG)klArrayDimensions) + " dimensional array, but the connected ICE ports are not a multi of array value.", siErrorMsg );
        return CStatus::OK;
      }
    }
    else if(klArrayDimensions > 2)
    {
      Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " refers to a " + CString((LONG)klArrayDimensions) + " dimensional array, but only a max of 2 dimensions are supported.", siErrorMsg );
      return CStatus::OK;
    }

    if(klBaseDataType == "Integer" || klBaseDataType == "SInt32")
    {
      if(dataType != siICENodeDataLong)
      {
        Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " refers to a " + CString(klDataType.c_str()) + " value, but the ICE port is not of siICENodeDataLong.", siErrorMsg );
        return CStatus::OK;
      }
    }
    else if(klBaseDataType == "Scalar" || klBaseDataType == "Float32")
    {
      if(dataType != siICENodeDataFloat)
      {
        Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " refers to a " + CString(klDataType.c_str()) + " value, but the ICE port is not of siICENodeDataFloat.", siErrorMsg );
        return CStatus::OK;
      }
    }
    else if(klBaseDataType == "Vec3")
    {
      if(dataType != siICENodeDataVector3)
      {
        Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " refers to a " + CString(klDataType.c_str()) + " value, but the ICE port is not of siICENodeDataVector3.", siErrorMsg );
        return CStatus::OK;
      }
    }
    else if(klBaseDataType == "Color")
    {
      if(dataType != siICENodeDataColor4)
      {
        Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " refers to a " + CString(klDataType.c_str()) + " value, but the ICE port is not of siICENodeDataColor4.", siErrorMsg );
        return CStatus::OK;
      }
    }
    else
    {
      Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " refers to a " + CString(klDataType.c_str()) + " value, which is not supported.", siErrorMsg );
      return CStatus::OK;
    }

    interf->setICENodeRTVal(singletonVal);
  }
  catch(FabricCore::Exception e)
  {
    Application().LogMessage(CString("spliceGetData") + " hit exception: " + e.getDesc_cstr(), siErrorMsg );
    return CStatus::OK;
  }
  catch(FabricSplice::Exception e)
  {
    Application().LogMessage(CString("spliceGetData") + " hit exception: " + e.what(), siErrorMsg );
    return CStatus::OK;
  }

  return CStatus::OK;
}

SICALLBACK spliceGetData_Evaluate(ICENodeContext& in_ctxt)
{
  XSI::siICENodeDataType dataType;
  XSI::siICENodeStructureType dataStruct;
  XSI::siICENodeContextType dataContext;
  in_ctxt.GetPortInfo( spliceGetData_ID_OUT_Result, dataType, dataStruct, dataContext );

  CDataArrayString referenceArray(in_ctxt, spliceGetData_ID_IN_reference);
  CString reference = referenceArray[0];

  CValue udVal = in_ctxt.GetUserData();
  iceNodeUD * ud = (iceNodeUD*)(CValue::siPtrType)udVal;
  FabricSpliceBaseInterface * interf = ud->interf;
  if(!interf)
    return CStatus::OK;
  FabricCore::RTVal rtVal = interf->getICENodeRTVal();
  if(!rtVal.isValid())
    return CStatus::OK;

  std::string klDataType = rtVal.getTypeName().getStringCString();
  std::string klBrackets = klDataType;
  int klArrayDimensions = 0;
  int klBracketPos = klBrackets.find('[');
  while(klBracketPos != std::string::npos)
  {
    klBrackets = klBrackets.substr(klBracketPos+1, klBrackets.length());
    klBracketPos = klBrackets.find('[');
    klArrayDimensions++;
  }

  try
  {
    void * data = NULL;
    if(klArrayDimensions < 2)
    {
      FabricCore::RTVal dataRtVal = rtVal.callMethod("Data", "data", 0, 0);
      data = dataRtVal.getData();
    }

    if(dataType == siICENodeDataLong)
    {
      if(dataContext == siICENodeContextSingleton)
      {
        if(dataStruct == siICENodeStructureSingle)
        {
          CDataArrayLong inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArrayLong outData(in_ctxt);
          outData[0] = ((int32_t*)data)[0];
        }
        else if(dataStruct == siICENodeStructureArray)
        {
          CDataArray2DLong inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArray2DLong outData2D(in_ctxt);
          if(in_ctxt.GetNumberOfElementsToProcess() != rtVal.getArraySize())
          {
            Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " contains " + CString((LONG)rtVal.getArraySize()) + " values, ICE expects " + CString((LONG)in_ctxt.GetNumberOfElementsToProcess()) + " values.", siErrorMsg );
            return CStatus::OK;
          }
          CDataArray2DLong::Accessor outData = outData2D.Resize(0, rtVal.getArraySize());
          memcpy(&outData[0], data, sizeof(int32_t) * outData.GetCount());
        }
      }
      else
      {
        if(dataStruct == siICENodeStructureSingle)
        {
          CDataArrayLong inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArrayLong outData(in_ctxt);
          if(in_ctxt.GetNumberOfElementsToProcess() != rtVal.getArraySize())
          {
            Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " contains " + CString((LONG)rtVal.getArraySize()) + " values, ICE expects " + CString((LONG)in_ctxt.GetNumberOfElementsToProcess()) + " values.", siErrorMsg );
            return CStatus::OK;
          }
          memcpy(&outData[0], data, sizeof(int32_t) * in_ctxt.GetNumberOfElementsToProcess());
        }
        else if(dataStruct == siICENodeStructureArray)
        {
          CDataArray2DLong inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArray2DLong outData2D(in_ctxt);
          if(in_ctxt.GetNumberOfElementsToProcess() != rtVal.getArraySize())
          {
            Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " contains " + CString((LONG)rtVal.getArraySize()) + " values, ICE expects " + CString((LONG)in_ctxt.GetNumberOfElementsToProcess()) + " values.", siErrorMsg );
            return CStatus::OK;
          }
          for(unsigned int i=0;i<outData2D.GetCount();i++)
          {
            FabricCore::RTVal subRtVal = rtVal.getArrayElement(i);
            FabricCore::RTVal subDataRtVal = subRtVal.callMethod("Data", "data", 0, 0);
            void * subData = subDataRtVal.getData();
            CDataArray2DLong::Accessor outData = outData2D.Resize(i, subRtVal.getArraySize());
            memcpy(&outData[0], subData, sizeof(int32_t) * outData.GetCount());
          }
        }
      }
    }
    else if(dataType == siICENodeDataFloat)
    {
      if(dataContext == siICENodeContextSingleton)
      {
        if(dataStruct == siICENodeStructureSingle)
        {
          CDataArrayFloat inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArrayFloat outData(in_ctxt);
          outData[0] = ((float*)data)[0];
        }
        else if(dataStruct == siICENodeStructureArray)
        {
          CDataArray2DFloat inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArray2DFloat outData2D(in_ctxt);
          if(in_ctxt.GetNumberOfElementsToProcess() != rtVal.getArraySize())
          {
            Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " contains " + CString((LONG)rtVal.getArraySize()) + " values, ICE expects " + CString((LONG)in_ctxt.GetNumberOfElementsToProcess()) + " values.", siErrorMsg );
            return CStatus::OK;
          }
          CDataArray2DFloat::Accessor outData = outData2D.Resize(0, rtVal.getArraySize());
          memcpy(&outData[0], data, sizeof(float) * in_ctxt.GetNumberOfElementsToProcess());
        }
      }
      else
      {
        if(dataStruct == siICENodeStructureSingle)
        {
          CDataArrayFloat inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArrayFloat outData(in_ctxt);
          if(in_ctxt.GetNumberOfElementsToProcess() != rtVal.getArraySize())
          {
            Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " contains " + CString((LONG)rtVal.getArraySize()) + " values, ICE expects " + CString((LONG)in_ctxt.GetNumberOfElementsToProcess()) + " values.", siErrorMsg );
            return CStatus::OK;
          }
          memcpy(&outData[0], data, sizeof(float) * in_ctxt.GetNumberOfElementsToProcess());
        }
        else if(dataStruct == siICENodeStructureArray)
        {
          CDataArray2DFloat inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArray2DFloat outData2D(in_ctxt);
          if(in_ctxt.GetNumberOfElementsToProcess() != rtVal.getArraySize())
          {
            Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " contains " + CString((LONG)rtVal.getArraySize()) + " values, ICE expects " + CString((LONG)in_ctxt.GetNumberOfElementsToProcess()) + " values.", siErrorMsg );
            return CStatus::OK;
          }
          for(unsigned int i=0;i<outData2D.GetCount();i++)
          {
            FabricCore::RTVal subRtVal = rtVal.getArrayElement(i);
            FabricCore::RTVal subDataRtVal = subRtVal.callMethod("Data", "data", 0, 0);
            void * subData = subDataRtVal.getData();
            CDataArray2DFloat::Accessor outData = outData2D.Resize(i, subRtVal.getArraySize());
            memcpy(&outData[0], subData, sizeof(float) * outData.GetCount());
            
          }
        }
      }
    }
    else if(dataType == siICENodeDataVector3)
    {
      if(dataContext == siICENodeContextSingleton)
      {
        if(dataStruct == siICENodeStructureSingle)
        {
          CDataArrayVector3f inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArrayVector3f outData(in_ctxt);
          outData[0].PutX(((float*)data)[0]);
          outData[0].PutY(((float*)data)[1]);
          outData[0].PutZ(((float*)data)[2]);
        }
        else if(dataStruct == siICENodeStructureArray)
        {
          CDataArray2DVector3f inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArray2DVector3f outData2D(in_ctxt);
          if(in_ctxt.GetNumberOfElementsToProcess() != rtVal.getArraySize())
          {
            Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " contains " + CString((LONG)rtVal.getArraySize()) + " values, ICE expects " + CString((LONG)in_ctxt.GetNumberOfElementsToProcess()) + " values.", siErrorMsg );
            return CStatus::OK;
          }
          CDataArray2DVector3f::Accessor outData = outData2D.Resize(0, rtVal.getArraySize());
          memcpy(&outData[0], data, sizeof(float) * 3 * outData.GetCount());
        }
      }
      else
      {
        if(dataStruct == siICENodeStructureSingle)
        {
          CDataArrayVector3f inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArrayVector3f outData(in_ctxt);

          if(in_ctxt.GetNumberOfElementsToProcess() != rtVal.getArraySize())
          {
            Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " contains " + CString((LONG)rtVal.getArraySize()) + " values, ICE expects " + CString((LONG)in_ctxt.GetNumberOfElementsToProcess()) + " values.", siErrorMsg );
            return CStatus::OK;
          }
          memcpy(&outData[0], data, sizeof(float) * 3 * in_ctxt.GetNumberOfElementsToProcess());
        }
        else if(dataStruct == siICENodeStructureArray)
        {
          CDataArray2DVector3f inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArray2DVector3f outData2D(in_ctxt);
          if(in_ctxt.GetNumberOfElementsToProcess() != rtVal.getArraySize())
          {
            Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " contains " + CString((LONG)rtVal.getArraySize()) + " values, ICE expects " + CString((LONG)in_ctxt.GetNumberOfElementsToProcess()) + " values.", siErrorMsg );
            return CStatus::OK;
          }
          for(unsigned int i=0;i<outData2D.GetCount();i++)
          {
            FabricCore::RTVal subRtVal = rtVal.getArrayElement(i);
            FabricCore::RTVal subDataRtVal = subRtVal.callMethod("Data", "data", 0, 0);
            void * subData = subDataRtVal.getData();
            CDataArray2DVector3f::Accessor outData = outData2D.Resize(i, subRtVal.getArraySize());
            memcpy(&outData[0], subData, sizeof(float) * 3 * outData.GetCount());
            
          }
        }
      }
    }
    else if(dataType == siICENodeDataColor4)
    {
      if(dataContext == siICENodeContextSingleton)
      {
        if(dataStruct == siICENodeStructureSingle)
        {
          CDataArrayColor4f inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArrayColor4f outData(in_ctxt);
          outData[0].PutR(((float*)data)[0]);
          outData[0].PutG(((float*)data)[1]);
          outData[0].PutB(((float*)data)[2]);
          outData[0].PutA(((float*)data)[2]);
        }
        else if(dataStruct == siICENodeStructureArray)
        {
          CDataArray2DColor4f inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArray2DColor4f outData2D(in_ctxt);
          if(in_ctxt.GetNumberOfElementsToProcess() != rtVal.getArraySize())
          {
            Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " contains " + CString((LONG)rtVal.getArraySize()) + " values, ICE expects " + CString((LONG)in_ctxt.GetNumberOfElementsToProcess()) + " values.", siErrorMsg );
            return CStatus::OK;
          }
          CDataArray2DColor4f::Accessor outData = outData2D.Resize(0, rtVal.getArraySize());
          memcpy(&outData[0], data, sizeof(float) * 4 * outData.GetCount());
        }
      }
      else
      {
        if(dataStruct == siICENodeStructureSingle)
        {
          CDataArrayColor4f inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArrayColor4f outData(in_ctxt);
          if(in_ctxt.GetNumberOfElementsToProcess() != rtVal.getArraySize())
          {
            Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " contains " + CString((LONG)rtVal.getArraySize()) + " values, ICE expects " + CString((LONG)in_ctxt.GetNumberOfElementsToProcess()) + " values.", siErrorMsg );
            return CStatus::OK;
          }
          memcpy(&outData[0], data, sizeof(float) * 4 * in_ctxt.GetNumberOfElementsToProcess());
        }
        else if(dataStruct == siICENodeStructureArray)
        {
          CDataArray2DColor4f inData(in_ctxt, spliceGetData_ID_IN_element);
          CDataArray2DColor4f outData2D(in_ctxt);
          if(in_ctxt.GetNumberOfElementsToProcess() != rtVal.getArraySize())
          {
            Application().LogMessage(CString("spliceGetData") + "'s reference " + reference + " contains " + CString((LONG)rtVal.getArraySize()) + " values, ICE expects " + CString((LONG)in_ctxt.GetNumberOfElementsToProcess()) + " values.", siErrorMsg );
            return CStatus::OK;
          }
          for(unsigned int i=0;i<outData2D.GetCount();i++)
          {
            FabricCore::RTVal subRtVal = rtVal.getArrayElement(i);
            FabricCore::RTVal subDataRtVal = subRtVal.callMethod("Data", "data", 0, 0);
            void * subData = subDataRtVal.getData();
            CDataArray2DColor4f::Accessor outData = outData2D.Resize(i, subRtVal.getArraySize());
            memcpy(&outData[0], subData, sizeof(float) * 4 * outData.GetCount());
            
          }
        }
      }
    }
  }
  catch(FabricCore::Exception e)
  {
    Application().LogMessage(CString("spliceGetData") + " hit exception: " + e.getDesc_cstr(), siErrorMsg );
    return CStatus::OK;
  }
  catch(FabricSplice::Exception e)
  {
    Application().LogMessage(CString("spliceGetData") + " hit exception: " + e.what(), siErrorMsg );
    return CStatus::OK;
  }

  return CStatus::OK;
}

SICALLBACK spliceGetData_Init( CRef & in_ctxt )
{
  Context ctxt(in_ctxt);
  ICENode node(ctxt.GetSource());

  FabricSpliceBaseInterface * interf = new FabricSpliceBaseInterface();
  interf->setUsedInICENode(true);
  iceNodeUD * ud = new iceNodeUD();
  ud->objectID = node.GetObjectID();
  interf->setObjectID(ud->objectID);

  CValue udVal = (CValue::siPtrType)ud;
  ctxt.PutUserData(udVal);  

  return CStatus::OK;
}

SICALLBACK spliceGetData_Term( CRef & in_ctxt )
{
  Context ctxt(in_ctxt);

  CValue udVal = ctxt.GetUserData();
  iceNodeUD * ud = (iceNodeUD*)(CValue::siPtrType)udVal;
  if(ud)
  {
    // look it up by id, so in case in was deleted already....
    FabricSpliceBaseInterface * interf = FabricSpliceBaseInterface::getInstanceByObjectID(ud->objectID);
    if(interf)
    {
      if(interf->needsDeletion())
        delete(interf);
    }

    delete(ud);
  }

  return CStatus::OK;
}

// SICALLBACK spliceGetDataPerPoint_BeginEvaluate(ICENodeContext& in_ctxt)
// {
//   return spliceGetDataSingle_BeginEvaluate(in_ctxt);
// }

// SICALLBACK spliceGetDataPerPoint_Evaluate(ICENodeContext& in_ctxt)
// {
//   return spliceGetDataSingle_Evaluate(in_ctxt);
// }

// SICALLBACK spliceGetDataPerPoint_Init( CRef & in_ctxt )
// {
//   return spliceGetDataSingle_Init(in_ctxt);
// }

// SICALLBACK spliceGetDataPerPoint_Term( CRef & in_ctxt )
// {
//   return spliceGetDataSingle_Term(in_ctxt);
// }
