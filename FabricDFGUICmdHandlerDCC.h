// Copyright 2010-2015 Fabric Software Inc. All rights reserved.

#ifndef _FabricDFGUICmdHandlerDCC_H_
#define _FabricDFGUICmdHandlerDCC_H_

#include <FabricUI/DFG/DFGUICmdHandler.h>
#include <FabricUI/DFG/DFGUICmd/DFGUICmds.h>

#define DFGUICmdHandlerLOG        false  // FOR DEBUGGING: log some info.
#define DFGUICmdHandlerByPassDCC  false  // FOR DEBUGGING: execute the dfg commands directly instead of using the DCC's commands.

class BaseInterface;

class DFGUICmdHandlerDCC : public FabricUI::DFG::DFGUICmdHandler
{
public:

  DFGUICmdHandlerDCC(void)
  {
    m_parentBaseInterface = NULL;
  }

  DFGUICmdHandlerDCC(BaseInterface *parentBaseInterface)
  {
    m_parentBaseInterface = parentBaseInterface;
  }

private:

  BaseInterface *m_parentBaseInterface;  // pointer at parent BaseInterface class.

protected:

  virtual void dfgDoRemoveNodes(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::ArrayRef<FTL::StrRef> nodeNames
    );

  virtual void dfgDoConnect(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef srcPath, 
    FTL::CStrRef dstPath
    );

  virtual void dfgDoDisconnect(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef srcPath, 
    FTL::CStrRef dstPath
    );

  virtual std::string dfgDoAddGraph(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef title,
    QPointF pos
    );

  virtual std::string dfgDoAddFunc(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef title,
    FTL::CStrRef initialCode,
    QPointF pos
    );

  virtual std::string dfgDoInstPreset(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef presetPath,
    QPointF pos
    );

  virtual std::string dfgDoAddVar(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef desiredNodeName,
    FTL::CStrRef dataType,
    FTL::CStrRef extDep,
    QPointF pos
    );

  virtual std::string dfgDoAddGet(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef desiredNodeName,
    FTL::CStrRef varPath,
    QPointF pos
    );

  virtual std::string dfgDoAddSet(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef desiredNodeName,
    FTL::CStrRef varPath,
    QPointF pos
    );

  virtual std::string dfgDoAddPort(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef desiredPortName,
    FabricCore::DFGPortType portType,
    FTL::CStrRef typeSpec,
    FTL::CStrRef portToConnect,
    FTL::StrRef extDep,
    FTL::CStrRef uiMetadata
    );

  virtual std::string dfgDoEditPort(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::StrRef oldPortName,
    FTL::StrRef desiredNewPortName,
    FTL::StrRef typeSpec,
    FTL::StrRef extDep,
    FTL::StrRef uiMetadata
    );

  virtual void dfgDoRemovePort(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef portName
    );

  virtual void dfgDoMoveNodes(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::ArrayRef<FTL::StrRef> nodeNames,
    FTL::ArrayRef<QPointF> newTopLeftPoss
    );

  virtual void dfgDoResizeBackDrop(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef backDropNodeName,
    QPointF newTopLeftPos,
    QSizeF newSize
    );

  virtual std::string dfgDoImplodeNodes(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::ArrayRef<FTL::StrRef> nodeNames,
    FTL::CStrRef desiredNodeName
    );

  virtual std::vector<std::string> dfgDoExplodeNode(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef nodeName
    );

  virtual void dfgDoAddBackDrop(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef title,
    QPointF pos
    );

  virtual void dfgDoSetTitle(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef title
    );

  virtual void dfgDoSetNodeComment(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef nodeName,
    FTL::CStrRef comment
    );

  virtual void dfgDoSetCode(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef code
    );

  virtual std::string dfgDoRenameNode(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef oldNodeName,
    FTL::CStrRef desiredNewNodeName
    );

  virtual std::string dfgDoRenamePort(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef oldPortName,
    FTL::CStrRef desiredNewPortName
    );

  virtual std::vector<std::string> dfgDoPaste(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef json,
    QPointF cursorPos
    );

  virtual void dfgDoSetArgType(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef argName,
    FTL::CStrRef typeName
    );

  virtual void dfgDoSetArgValue(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef argName,
    FabricCore::RTVal const &value
    );

  virtual void dfgDoSetExtDeps(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::ArrayRef<FTL::StrRef> extDeps
    );

  virtual void dfgDoSetPortDefaultValue(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef portPath,
    FabricCore::RTVal const &value
    );

  virtual void dfgDoSetRefVarPath(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef refName,
    FTL::CStrRef varPath
    );

  virtual void dfgDoSplitFromPreset(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec
    );

  virtual void dfgDoReorderPorts(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    const std::vector<unsigned int> &indices
    );

protected:
    
  std::string getDCCObjectNameFromBinding(FabricCore::DFGBinding const &binding);

public:
    
  static FabricCore::DFGBinding getBindingFromDCCObjectName(std::string name);

public:

  static FabricUI::DFG::DFGUICmd *createAndExecuteDFGCommand(std::string &in_cmdName, std::vector<std::string> &in_args);
  static FabricUI::DFG::DFGUICmd_RemoveNodes *createAndExecuteDFGCommand_RemoveNodes(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_Connect *createAndExecuteDFGCommand_Connect(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_Disconnect *createAndExecuteDFGCommand_Disconnect(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddGraph *createAndExecuteDFGCommand_AddGraph(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddFunc *createAndExecuteDFGCommand_AddFunc(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_InstPreset *createAndExecuteDFGCommand_InstPreset(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddVar *createAndExecuteDFGCommand_AddVar(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddGet *createAndExecuteDFGCommand_AddGet(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddSet *createAndExecuteDFGCommand_AddSet(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddPort *createAndExecuteDFGCommand_AddPort(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_EditPort *createAndExecuteDFGCommand_EditPort(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_RemovePort *createAndExecuteDFGCommand_RemovePort(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_MoveNodes *createAndExecuteDFGCommand_MoveNodes(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_ResizeBackDrop *createAndExecuteDFGCommand_ResizeBackDrop(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_ImplodeNodes *createAndExecuteDFGCommand_ImplodeNodes(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_ExplodeNode *createAndExecuteDFGCommand_ExplodeNode(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddBackDrop *createAndExecuteDFGCommand_AddBackDrop(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetTitle *createAndExecuteDFGCommand_SetTitle(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetNodeComment *createAndExecuteDFGCommand_SetNodeComment(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetCode *createAndExecuteDFGCommand_SetCode(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_RenameNode *createAndExecuteDFGCommand_RenameNode(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_RenamePort *createAndExecuteDFGCommand_RenamePort(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_Paste *createAndExecuteDFGCommand_Paste(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetArgType *createAndExecuteDFGCommand_SetArgType(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetArgValue *createAndExecuteDFGCommand_SetArgValue(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetExtDeps *createAndExecuteDFGCommand_SetExtDeps(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetPortDefaultValue *createAndExecuteDFGCommand_SetPortDefaultValue(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetRefVarPath *createAndExecuteDFGCommand_SetRefVarPath(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SplitFromPreset *createAndExecuteDFGCommand_SplitFromPreset(std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_ReorderPorts *createAndExecuteDFGCommand_ReorderPorts(std::vector<std::string> &args);
};

#endif
