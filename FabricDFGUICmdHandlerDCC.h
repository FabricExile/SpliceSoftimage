// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.

#ifndef _FabricDFGUICmdHandlerDCC_H_
#define _FabricDFGUICmdHandlerDCC_H_

#include <FabricUI/DFG/DFGUICmdHandler.h>
#include <FabricUI/DFG/DFGUICmd/DFGUICmds.h>

#define DFGUICmdHandlerLOG        false  // FOR DEBUGGING: log some info.

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
    QString execPath,
    FabricCore::DFGExec const &exec,
    QStringList nodeNames
    );

  virtual void dfgDoConnect(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QStringList srcPaths,
    QStringList dstPaths
    );

  virtual void dfgDoDisconnect(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QStringList srcPaths,
    QStringList dstPaths
    );

  virtual QString dfgDoAddGraph(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString title,
    QPointF pos
    );

  virtual QString dfgDoAddFunc(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString title,
    QString initialCode,
    QPointF pos
    );

  virtual QString dfgDoInstPreset(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString presetPath,
    QPointF pos
    );

  virtual QString dfgDoAddVar(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString desiredNodeName,
    QString dataType,
    QString extDep,
    QPointF pos
    );

  virtual QString dfgDoAddGet(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString desiredNodeName,
    QString varPath,
    QPointF pos
    );

  virtual QString dfgDoAddSet(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString desiredNodeName,
    QString varPath,
    QPointF pos
    );

  virtual QString dfgDoAddPort(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString desiredPortName,
    FabricCore::DFGPortType portType,
    QString typeSpec,
    QString portToConnect,
    QString extDep,
    QString uiMetadata
    );

  virtual QString dfgDoAddInstPort(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString instName,
    QString desiredPortName,
    FabricCore::DFGPortType portType,
    QString typeSpec,
    QString pathToConnect,
    FabricCore::DFGPortType connectType,
    QString extDep,
    QString metaData
    );

  virtual QString dfgDoAddInstBlockPort(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString instName,
    QString blockName,
    QString desiredPortName,
    QString typeSpec,
    QString pathToConnect,
    QString extDep,
    QString metaData
    );

  virtual QString dfgDoCreatePreset(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString nodeName,
    QString presetDirPath,
    QString presetName
    );

  virtual QString dfgDoEditPort(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString oldPortName,
    QString desiredNewPortName,
    FabricCore::DFGPortType portType,
    QString typeSpec,
    QString extDep,
    QString uiMetadata
    );

  virtual void dfgDoRemovePort(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString portName
    );

  virtual void dfgDoMoveNodes(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QStringList nodeNames,
    QList<QPointF> newTopLeftPoss
    );

  virtual void dfgDoResizeBackDrop(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString backDropNodeName,
    QPointF newTopLeftPos,
    QSizeF newSize
    );

  virtual QString dfgDoImplodeNodes(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QStringList nodeNames,
    QString desiredNodeName
    );

  virtual QStringList dfgDoExplodeNode(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString nodeName
    );

  virtual void dfgDoAddBackDrop(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString title,
    QPointF pos
    );

  virtual void dfgDoSetNodeComment(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString nodeName,
    QString comment
    );

  virtual void dfgDoSetCode(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString code
    );

  virtual QString dfgDoEditNode(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString oldNodeName,
    QString desiredNewNodeName,
    QString nodeMetadata,
    QString execMetadata
    );

  virtual QString dfgDoRenamePort(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString portPath,
    QString desiredNewPortName
    );

  virtual QStringList dfgDoPaste(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString json,
    QPointF cursorPos
    );

  virtual void dfgDoSetArgValue(
    FabricCore::DFGBinding const &binding,
    QString argName,
    FabricCore::RTVal const &value
    );

  virtual void dfgDoSetPortDefaultValue(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString portPath,
    FabricCore::RTVal const &value
    );

  virtual void dfgDoSetRefVarPath(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString refName,
    QString varPath
    );

  virtual void dfgDoReorderPorts(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString itemPath,
    QList<int> indices
    );

  virtual void dfgDoSetExtDeps(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QStringList extDeps
    );

  virtual void dfgDoSplitFromPreset(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec
    );

  virtual void dfgDoDismissLoadDiags(
    FabricCore::DFGBinding const &binding,
    QList<int> diagIndices
    );

  virtual QString dfgDoAddBlock(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString desiredName,
    QPointF pos
    );

  virtual QString dfgDoAddBlockPort(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString blockName,
    QString desiredPortName,
    FabricCore::DFGPortType portType,
    QString typeSpec,
    QString pathToConnect,
    FabricCore::DFGPortType connectType,
    QString extDep,
    QString metaData
    );

protected:
    
  std::string getDCCObjectNameFromBinding(FabricCore::DFGBinding const &binding);

public:
    
  static FabricCore::DFGBinding getBindingFromDCCObjectName(std::string name);

public:

  static FabricUI::DFG::DFGUICmd                      *createAndExecuteDFGCommand                      (std::string &in_cmdName, std::vector<std::string> &in_args);
  static FabricUI::DFG::DFGUICmd_RemoveNodes          *createAndExecuteDFGCommand_RemoveNodes          (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_Connect              *createAndExecuteDFGCommand_Connect              (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_Disconnect           *createAndExecuteDFGCommand_Disconnect           (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddGraph             *createAndExecuteDFGCommand_AddGraph             (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddFunc              *createAndExecuteDFGCommand_AddFunc              (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_InstPreset           *createAndExecuteDFGCommand_InstPreset           (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddVar               *createAndExecuteDFGCommand_AddVar               (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddGet               *createAndExecuteDFGCommand_AddGet               (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddSet               *createAndExecuteDFGCommand_AddSet               (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddPort              *createAndExecuteDFGCommand_AddPort              (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddInstPort          *createAndExecuteDFGCommand_AddInstPort          (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddInstBlockPort     *createAndExecuteDFGCommand_AddInstBlockPort     (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_CreatePreset         *createAndExecuteDFGCommand_CreatePreset         (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_EditPort             *createAndExecuteDFGCommand_EditPort             (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_RemovePort           *createAndExecuteDFGCommand_RemovePort           (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_MoveNodes            *createAndExecuteDFGCommand_MoveNodes            (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_ResizeBackDrop       *createAndExecuteDFGCommand_ResizeBackDrop       (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_ImplodeNodes         *createAndExecuteDFGCommand_ImplodeNodes         (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_ExplodeNode          *createAndExecuteDFGCommand_ExplodeNode          (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddBackDrop          *createAndExecuteDFGCommand_AddBackDrop          (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetNodeComment       *createAndExecuteDFGCommand_SetNodeComment       (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetCode              *createAndExecuteDFGCommand_SetCode              (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_EditNode             *createAndExecuteDFGCommand_EditNode             (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_RenamePort           *createAndExecuteDFGCommand_RenamePort           (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_Paste                *createAndExecuteDFGCommand_Paste                (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetArgValue          *createAndExecuteDFGCommand_SetArgValue          (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetPortDefaultValue  *createAndExecuteDFGCommand_SetPortDefaultValue  (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetRefVarPath        *createAndExecuteDFGCommand_SetRefVarPath        (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_ReorderPorts         *createAndExecuteDFGCommand_ReorderPorts         (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SetExtDeps           *createAndExecuteDFGCommand_SetExtDeps           (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_SplitFromPreset      *createAndExecuteDFGCommand_SplitFromPreset      (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_DismissLoadDiags     *createAndExecuteDFGCommand_DismissLoadDiags     (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddBlock             *createAndExecuteDFGCommand_AddBlock             (std::vector<std::string> &args);
  static FabricUI::DFG::DFGUICmd_AddBlockPort         *createAndExecuteDFGCommand_AddBlockPort         (std::vector<std::string> &args);
};

#endif
