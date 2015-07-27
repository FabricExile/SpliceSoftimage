#include "FabricDFGUICmdHandlerXSI.h"
#include "FabricDFGBaseInterface.h"
#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"

#include <FabricUI/DFG/DFGUICmd/DFGUICmds.h>

#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_argument.h>
#include <xsi_customoperator.h>
#include <xsi_command.h>
#include <xsi_status.h>
#include <xsi_value.h>
#include <xsi_vector2f.h>

#include <sstream>

using namespace XSI;

/*----------------
  helper functions.
*/

// execute a XSI command.
// return values: on success: true and io_result contains the command's return value.
//                on failure: false and io_result has no type.
bool executeCommand(const CString &cmdName, const CValueArray &args, CValue &io_result)
{
  // init result.
  io_result.Clear();

  // execute command.
  if (Application().ExecuteCommand(cmdName, args, io_result) == CStatus::OK)
  {
    // success.
    return true;
  }
  else
  {
    // failed: log some info about this command and return false.
    Application().LogMessage(L"failed to execute \"" + cmdName + L"\" with the following array of arguments:", siWarningMsg);
    for (LONG i=0;i<args.GetCount();i++)
      Application().LogMessage(L"    \"" + args[i].GetAsText() + L"\"", siWarningMsg);
    return false;
  }
}

/*-----------------------------------------------------
  implementation of DFGUICmdHandlerXSI member functions.
*/

void DFGUICmdHandlerXSI::dfgDoRemoveNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodeNames
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_RemoveNodes::CmdName().c_str());
  CValueArray args;

  Application().LogMessage(L"not yet implemented", siWarningMsg);
}

void DFGUICmdHandlerXSI::dfgDoConnect(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef srcPort, 
  FTL::CStrRef dstPort
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_Connect::CmdName().c_str());
  CValueArray args;

  Application().LogMessage(L"not yet implemented", siWarningMsg);
}

void DFGUICmdHandlerXSI::dfgDoDisconnect(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef srcPort, 
  FTL::CStrRef dstPort
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_Disconnect::CmdName().c_str());
  CValueArray args;

  Application().LogMessage(L"not yet implemented", siWarningMsg);
}

std::string DFGUICmdHandlerXSI::dfgDoAddGraph(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  QPointF pos
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_AddGraph::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(title.c_str());                                // title.
  args.Add(pos.x());                                      // positionX.
  args.Add(pos.y());                                      // positionY.

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

std::string DFGUICmdHandlerXSI::dfgDoAddFunc(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  FTL::CStrRef initialCode,
  QPointF pos
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_AddFunc::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(title.c_str());                                // title.
  args.Add(initialCode.c_str());                          // initialCode.
  args.Add(pos.x());                                      // positionX.
  args.Add(pos.y());                                      // positionY.

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

std::string DFGUICmdHandlerXSI::dfgDoInstPreset(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef presetPath,
  QPointF pos
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_InstPreset::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(presetPath.c_str());                           // preset.
  args.Add(pos.x());                                      // positionX.
  args.Add(pos.y());                                      // positionY.

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

std::string DFGUICmdHandlerXSI::dfgDoAddVar(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredNodeName,
  FTL::CStrRef dataType,
  FTL::CStrRef extDep,
  QPointF pos
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_AddVar::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(desiredNodeName.c_str());                      // desiredNodeName.
  args.Add(dataType.c_str());                             // dataType.
  args.Add(extDep.c_str());                               // extDep.
  args.Add(pos.x());                                      // positionX.
  args.Add(pos.y());                                      // positionY.

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

std::string DFGUICmdHandlerXSI::dfgDoAddGet(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredNodeName,
  FTL::CStrRef varPath,
  QPointF pos
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_AddGet::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(desiredNodeName.c_str());                      // desiredNodeName.
  args.Add(varPath.c_str());                              // varPath.
  args.Add(pos.x());                                      // positionX.
  args.Add(pos.y());                                      // positionY.

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

std::string DFGUICmdHandlerXSI::dfgDoAddSet(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredNodeName,
  FTL::CStrRef varPath,
  QPointF pos
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_AddSet::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(desiredNodeName.c_str());                      // desiredNodeName.
  args.Add(varPath.c_str());                              // varPath.
  args.Add(pos.x());                                      // positionX.
  args.Add(pos.y());                                      // positionY.

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

std::string DFGUICmdHandlerXSI::dfgDoAddPort(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredPortName,
  FabricCore::DFGPortType portType,
  FTL::CStrRef typeSpec,
  FTL::CStrRef portToConnect
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_AddPort::CmdName().c_str());
  CValueArray args;

  Application().LogMessage(L"not yet implemented", siWarningMsg);

  return "";
}

void DFGUICmdHandlerXSI::dfgDoRemovePort(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef portName
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_RemovePort::CmdName().c_str());
  CValueArray args;

  Application().LogMessage(L"not yet implemented", siWarningMsg);
}

void DFGUICmdHandlerXSI::dfgDoMoveNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodeNames,
  FTL::ArrayRef<QPointF> newTopLeftPoss
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_MoveNodes::CmdName().c_str());
  CValueArray args;

  Application().LogMessage(L"not yet implemented", siWarningMsg);
}

void DFGUICmdHandlerXSI::dfgDoResizeBackDrop(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef name,
  QPointF newTopLeftPos,
  QSizeF newSize
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_ResizeBackDrop::CmdName().c_str());
  CValueArray args;

  Application().LogMessage(L"not yet implemented", siWarningMsg);
}

std::string DFGUICmdHandlerXSI::dfgDoImplodeNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredNodeName,
  FTL::ArrayRef<FTL::CStrRef> nodeNames
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_ImplodeNodes::CmdName().c_str());
  CValueArray args;

  Application().LogMessage(L"not yet implemented", siWarningMsg);

  return "";
}

std::vector<std::string> DFGUICmdHandlerXSI::dfgDoExplodeNode(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef name
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_ExplodeNode::CmdName().c_str());
  CValueArray args;

  Application().LogMessage(L"not yet implemented", siWarningMsg);

  std::vector<std::string> result;
  return result;
}

void DFGUICmdHandlerXSI::dfgDoAddBackDrop(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  QPointF pos
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_AddBackDrop::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(title.c_str());                                // title.
  args.Add(pos.x());                                      // positionX.
  args.Add(pos.y());                                      // positionY.

  executeCommand(cmdName, args, CValue());
}

void DFGUICmdHandlerXSI::dfgDoSetNodeTitle(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef nodeName,
  FTL::CStrRef title
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_SetNodeTitle::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(nodeName.c_str());                             // nodeName.
  args.Add(title.c_str());                                // title.

  executeCommand(cmdName, args, CValue());
}

void DFGUICmdHandlerXSI::dfgDoSetNodeComment(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef nodeName,
  FTL::CStrRef comment
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_SetNodeComment::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(nodeName.c_str());                             // nodeName.
  args.Add(comment.c_str());                              // comment.

  executeCommand(cmdName, args, CValue());
}

void DFGUICmdHandlerXSI::dfgDoSetCode(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef code
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_SetCode::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(code.c_str());                                 // code.

  executeCommand(cmdName, args, CValue());
}

std::string DFGUICmdHandlerXSI::dfgDoRenamePort(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef oldPortName,
  FTL::CStrRef desiredNewPortName
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_RenamePort::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(oldPortName.c_str());                          // oldPortName.
  args.Add(desiredNewPortName.c_str());                   // desiredNewPortName.

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

std::vector<std::string> DFGUICmdHandlerXSI::dfgDoPaste(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef json,
  QPointF cursorPos
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_Paste::CmdName().c_str());
  CValueArray args;

  Application().LogMessage(L"not yet implemented", siWarningMsg);

  std::vector<std::string> result;
  return result;
}

void DFGUICmdHandlerXSI::dfgDoSetArgType(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef argName,
  FTL::CStrRef typeName
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_SetArgType::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(argName.c_str());                              // argName.
  args.Add(typeName.c_str());                             // typeName.

  executeCommand(cmdName, args, CValue());
}

void DFGUICmdHandlerXSI::dfgDoSetArgValue(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef argName,
  FabricCore::RTVal const &value
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_SetArgValue::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(argName.c_str());                              // argName.
  args.Add(value.getJSON().getStringCString());           // value.

  executeCommand(cmdName, args, CValue());
}

void DFGUICmdHandlerXSI::dfgDoSetPortDefaultValue(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef portPath,
  FabricCore::RTVal const &value
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_SetPortDefaultValue::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(portPath.c_str());                             // portPath.
  args.Add(value.getJSON().getStringCString());           // value.

  executeCommand(cmdName, args, CValue());
}

void DFGUICmdHandlerXSI::dfgDoSetRefVarPath(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef refName,
  FTL::CStrRef varPath
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_SetRefVarPath::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(refName.c_str());                              // refName.
  args.Add(varPath.c_str());                              // varPath.

  executeCommand(cmdName, args, CValue());
}

std::string DFGUICmdHandlerXSI::getOperatorNameFromBinding( FabricCore::DFGBinding const &binding )
{
  // try to get the operator's name for this binding.
  if (m_parentBaseInterface && m_parentBaseInterface->getBinding() == binding)
  {
    unsigned int opObjID = _opUserData::GetOperatorObjectID(m_parentBaseInterface);
    if (opObjID != UINT_MAX)
    {
      CRef opRef = Application().GetObjectFromID(opObjID).GetRef();
      if (opRef.IsValid())
      {
        // got it.
        return opRef.GetAsText().GetAsciiString();
      }
    }
  }

  // not found.
  return "";
}

// ---
// command "dfgUICmdInstPreset"
// ---

SICALLBACK dfgUICmdInstPreset_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",   CString());
  oArgs.Add(L"exec",      CString());
  oArgs.Add(L"preset",    CString());
  oArgs.Add(L"positionX", 0.0);
  oArgs.Add(L"positionY", 0.0);

  return CStatus::OK;
}

SICALLBACK dfgUICmdInstPreset_Execute(CRef &in_ctxt)
{
  // init.
  Context ctxt(in_ctxt);
	CValueArray args = ctxt.GetAttribute(L"Arguments");

  // declare/init task.
  _dfgCmdTask taskTmp, *task = &taskTmp;
 	if ((bool)ctxt.GetAttribute(L"UndoRequired") == true)
	{
    task = new _dfgCmdTask;
		ctxt.PutAttribute(L"UndoRedoData", (CValue::siPtrType)task);
	}

  //
	task->Do();

  // done.
  return CStatus::OK;
}

SICALLBACK dfgUICmdInstPreset_Undo(CRef &in_ctxt)
{
	_dfgCmdTask *task = (_dfgCmdTask *)(CValue::siPtrType)Context(in_ctxt).GetAttribute(L"UndoRedoData");
	if (task)  task->Undo();
  return CStatus::OK;
}

SICALLBACK dfgUICmdInstPreset_Redo(CRef &in_ctxt)
{
	_dfgCmdTask *task = (_dfgCmdTask *)(CValue::siPtrType)Context(in_ctxt).GetAttribute(L"UndoRedoData");
	if (task)  task->Redo();
  return CStatus::OK;
}

SICALLBACK dfgUICmdInstPreset_TermUndoRedo(CRef &in_ctxt)
{
	_dfgCmdTask *task = (_dfgCmdTask *)(CValue::siPtrType)Context(in_ctxt).GetAttribute(L"UndoRedoData");
	if (task)  delete task;
  return CStatus::OK;
}

