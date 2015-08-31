
#ifndef _FabricSpliceBaseInterface_H_
#define _FabricSpliceBaseInterface_H_

#include "FabricSpliceConversion.h"

#include <xsi_string.h>
#include <xsi_value.h>
#include <xsi_status.h>
#include <xsi_customoperator.h>
#include <xsi_operatorcontext.h>
#include <xsi_factory.h>
#include <xsi_longarray.h>
#include <xsi_model.h>
#include <vector>
#include <map>

#include <FabricSplice.h>

#define XSISPLICE_CATCH_BEGIN() try{
#define XSISPLICE_CATCH_END() } catch(FabricSplice::Exception e){}
#define XSISPLICE_CATCH_END_VOID() } catch(FabricSplice::Exception e){ return; }
#define XSISPLICE_CATCH_END_CSTATUS() } catch(FabricSplice::Exception e){ return CStatus::Fail; }

class FabricSpliceBaseInterface {

public:

  FabricSpliceBaseInterface();
  ~FabricSpliceBaseInterface();

  virtual unsigned int getObjectID() const;
  virtual void setObjectID(unsigned int objectID);
  virtual bool needsDeletion() { return !_persist; }
  virtual void setNeedsDeletion(bool removeInstance) { _persist = !removeInstance; }
  virtual bool usedInICENode() { return _usedInICENode; }
  virtual void setUsedInICENode(bool flag) { _usedInICENode = flag; }
  
  static std::vector<FabricSpliceBaseInterface*> getInstances();
  static FabricSpliceBaseInterface * getInstanceByObjectID(unsigned int objectID);

  XSI::CStatus updateXSIOperator();
  void forceEvaluate();
  static XSI::CStatus constructXSIParameters(XSI::CustomOperator & op, XSI::Factory & factory);
  static XSI::CValueArray getSpliceParamTypeCombo();
  static XSI::CValueArray getSpliceXSIPortTypeCombo();
  static XSI::CValueArray getSpliceInternalPortTypeCombo();
  XSI::CStatus addXSIParameter(const XSI::CString &portName, const XSI::CString &dataType, const XSI::CString &portModeStr, const XSI::CString & dgNode, const FabricCore::Variant & defaultValue = FabricCore::Variant(), const XSI::CString & extStr = "");
  XSI::CStatus addXSIPort(const XSI::CRefArray & targets, const XSI::CString &portName, const XSI::CString &dataType, const FabricSplice::Port_Mode &portMode, const XSI::CString & dgNode, bool validateDataType = true);
  XSI::CStatus addXSIICEPort(const XSI::CRefArray & targets, const XSI::CString &portName, const XSI::CString &dataType, const XSI::CString &iceAttrName, const XSI::CString & dgNode);
  XSI::CStatus addSplicePort(const XSI::CString &portName, const XSI::CString &dataType, const FabricSplice::Port_Mode &portMode, const XSI::CString & dgNode, bool autoInitObjects = true, const FabricCore::Variant & defaultValue = FabricCore::Variant(), const XSI::CString & extStr = "");
  XSI::CStatus removeSplicePort(const XSI::CString &portName);
  XSI::CStatus rerouteXSIPort(const XSI::CString &portName, FabricCore::Variant & scriptArgs);
  XSI::CString getXSIPortTargets(const XSI::CString &portName);
  void setXSIPortTargets(const XSI::CString &portName, const XSI::CString &targets);
  XSI::CString getParameterString();

  bool transferInputPorts(XSI::CRef opRef, XSI::OperatorContext & context);
  XSI::CStatus transferOutputPort(XSI::OperatorContext & context);
  XSI::CStatus evaluate();

  FabricSplice::DGGraph getSpliceGraph();

  XSI::CStatus addKLOperator(const XSI::CString &operatorName, const XSI::CString &operatorCode, const XSI::CString &operatorEntry, const XSI::CString & dgNode, const FabricCore::Variant & portMap);
  bool hasKLOperator(const XSI::CString &operatorName, const XSI::CString & dgNode);
  XSI::CString getKLOperatorCode(const XSI::CString &operatorName);
  XSI::CStatus setKLOperatorCode(const XSI::CString &operatorName, const XSI::CString &operatorCode, const XSI::CString &operatorEntry);
  XSI::CStatus setKLOperatorFile(const XSI::CString &operatorName, const XSI::CString &filename, const XSI::CString &entry);
  XSI::CStatus setKLOperatorEntry(const XSI::CString &operatorName, const XSI::CString &operatorEntry);
  XSI::CStatus setKLOperatorIndex(const XSI::CString &operatorName, unsigned int operatorIndex);
  XSI::CStatus removeKLOperator(const XSI::CString &operatorName, const XSI::CString & dgNode);

  XSI::CStatus storePersistenceData(XSI::CString fileName);
  XSI::CStatus restoreFromPersistenceData(XSI::CString fileName);
  XSI::CStatus saveToFile(XSI::CString fileName);
  XSI::CStatus loadFromFile(XSI::CString fileName, FabricCore::Variant & scriptArgs, bool hideUI);

  XSI::CString getDGPortInfo();

  XSI::CStatus disconnectForExport(XSI::CString fileName, XSI::Model & model);
  XSI::CStatus reconnectForImport(XSI::Model & model);
  static XSI::CStatus cleanupForImport(XSI::Model & model);

  void setICENodeRTVal(FabricCore::RTVal val) { iceNodeRTVal = val; }
  FabricCore::RTVal getICENodeRTVal() { return iceNodeRTVal; }

  bool processNameChange(XSI::CString prevFullPath, XSI::CString newFullPath);

protected:
  // private members and helper methods
  unsigned int _objectID;
  static std::vector<FabricSpliceBaseInterface*> _instances;
  static FabricSpliceBaseInterface* _currentInstance;
  FabricSplice::DGGraph _spliceGraph;
  bool _persist;
  bool _usedInICENode;

  struct parameterInfo
  {
    XSI::CString dataType;
    XSI::CValue defaultValue;
    XSI::CStringArray paramNames;
    XSI::CValueArray paramValues;
  };

  struct portInfo
  {
    XSI::CString realPortName;
    bool isArray;
    XSI::CString dataType;
    FabricSplice::Port_Mode portMode;
    XSI::CString targets;
    XSI::CLongArray portIndices;
    LONG outPortElementsProcessed;
    std::vector<float> floats;
  };

  std::map<std::string, parameterInfo> _parameters;
  std::map<std::string, portInfo> _ports;
  unsigned int _nbOutputPorts;
  std::vector<std::string> _processedPorts;

  std::vector< std::vector<XSI::CValue> > valuesCache;
  std::vector<LONG> evalIDsCache;

  FabricCore::RTVal iceNodeRTVal;

  void addDirtyInput(std::string portName, FabricCore::RTVal evalContext, int index);
  bool checkIfValueChangedAndDirtyInput(XSI::CValue value, std::vector<XSI::CValue> &cachedValues, bool alwaysEvaluate, std::string portName, FabricCore::RTVal evalContext, int index);
  bool checkEvalIDCache(LONG evalID, int &evalIDCacheIndex, bool alwaysEvaluate);

};

enum SoftimagePortType {
  SoftimagePortType_Port,
  SoftimagePortType_Parameter,
  SoftimagePortType_Internal,
  SoftimagePortType_ICE
};

#endif
