#include "FabricDFGUICmdHandlerXSI.h"
#include "FabricDFGBaseInterface.h"
#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"

#include <FabricUI/DFG/DFGUICmd/DFGUICmds.h>

#include <sstream>

/*----------------
  helper functions.
*/

bool executeCommand(const XSI::CString &cmdName, const XSI::CValueArray &args, XSI::CValue &io_result)
{
  // log debug.
  if (true)
  {
    XSI::Application().LogMessage(L"about to execute command \"" + cmdName + L"\" with the following arguments:");
    for (LONG i=0;i<args.GetCount();i++)
      XSI::Application().LogMessage(L"    \"" + args[i].GetAsText() + L"\"");
  }

  XSI::CValueArray bla;
  bla.Add(L"null");
  bla.Add(1.0f);
  bla.Add(2.0f);
  bla.Add(3.0f);
  XSI::Application().ExecuteCommand(L"Translate", bla, XSI::CValue());

  // execute command and return true on success.
  return (XSI::Application().ExecuteCommand(cmdName, args, io_result) == XSI::CStatus::OK);
}

/*-----------------------------------------------------
  implementation of DFGUICmdHandlerXSI memebr functions.
*/

void DFGUICmdHandlerXSI::encodeMELString(
  FTL::CStrRef str,
  std::stringstream &ss
  )
{
  ss << '"';
  for ( FTL::CStrRef::IT it = str.begin(); it != str.end(); ++it )
  {
    switch ( *it )
    {
      case '"':
        ss << "\\\"";
        break;
      
      case '\\':
        ss << "\\\\";
        break;
      
      case '\t':
        ss << "\\t";
        break;
      
      case '\r':
        ss << "\\r";
        break;
      
      case '\n':
        ss << "\\n";
        break;
      
      default:
        ss << *it;
        break;
    }
  }
  ss << '"';
}

void DFGUICmdHandlerXSI::encodeStringArg(
  FTL::CStrRef name,
  FTL::CStrRef value,
  std::stringstream &cmd
  )
{
  cmd << " -";
  cmd << name.c_str();
  cmd << " ";
  encodeMELString( value, cmd );
}

void DFGUICmdHandlerXSI::encodeStringsArg(
  FTL::CStrRef name,
  FTL::ArrayRef<FTL::CStrRef> values,
  std::stringstream &cmd
  )
{
  for ( FTL::ArrayRef<FTL::CStrRef>::IT it = values.begin();
    it != values.end(); ++it )
  {
    FTL::CStrRef value = *it;
    encodeStringArg( name, value, cmd );
  }
}

void DFGUICmdHandlerXSI::encodePositionArg(
  FTL::CStrRef name,
  QPointF value,
  std::stringstream &cmd
  )
{
  cmd << " -";
  cmd << name.c_str();
  cmd << " ";
  cmd << value.x();
  cmd << " ";
  cmd << value.y();
}

void DFGUICmdHandlerXSI::encodePositionsArg(
  FTL::CStrRef name,
  FTL::ArrayRef<QPointF> values,
  std::stringstream &cmd
  )
{
  for ( FTL::ArrayRef<QPointF>::IT it = values.begin();
    it != values.end(); ++it )
  {
    QPointF value = *it;
    encodePositionArg( name, value, cmd );
  }
}

void DFGUICmdHandlerXSI::encodeSizeArg(
  FTL::CStrRef name,
  QSizeF value,
  std::stringstream &cmd
  )
{
  cmd << " -";
  cmd << name.c_str();
  cmd << " ";
  cmd << value.width();
  cmd << " ";
  cmd << value.height();
}

void DFGUICmdHandlerXSI::encodeSizesArg(
  FTL::CStrRef name,
  FTL::ArrayRef<QSizeF> values,
  std::stringstream &cmd
  )
{
  for ( FTL::ArrayRef<QSizeF>::IT it = values.begin();
    it != values.end(); ++it )
  {
    QSizeF value = *it;
    encodeSizeArg( name, value, cmd );
  }
}

void DFGUICmdHandlerXSI::addBindingToArgs(
  FabricCore::DFGBinding const &binding,
  XSI::CValueArray &args
  )
{
  args.Add(L"operatorName");
  args.Add(getOperatorNameFromBinding(binding).c_str());
}

void DFGUICmdHandlerXSI::addExecToArgs(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  XSI::CValueArray &args
  )
{
  addBindingToArgs(binding, args);
  args.Add(L"exec");
  args.Add(execPath.c_str());
}

std::string DFGUICmdHandlerXSI::dfgDoInstPreset(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef preset,
  QPointF pos
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_InstPreset::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"preset");
  args.Add(preset.c_str());
  args.Add(L"position");
  args.Add(XSI::MATH::CVector2f(pos.x(), pos.y()));

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);

  return result.GetAsText().GetAsciiString();
}

std::string DFGUICmdHandlerXSI::dfgDoAddVar(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredName,
  FTL::CStrRef dataType,
  FTL::CStrRef extDep,
  QPointF pos
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_AddVar::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"desiredName");
  args.Add(desiredName.c_str());
  args.Add(L"dataType");
  args.Add(dataType.c_str());
  args.Add(L"extDep");
  args.Add(extDep.c_str());
  args.Add(L"position");
  args.Add(XSI::MATH::CVector2f(pos.x(), pos.y()));

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);

  return result.GetAsText().GetAsciiString();
}

std::string DFGUICmdHandlerXSI::dfgDoAddGet(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredName,
  FTL::CStrRef varPath,
  QPointF pos
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_AddGet::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"desiredName");
  args.Add(desiredName.c_str());
  args.Add(L"varPath");
  args.Add(varPath.c_str());
  args.Add(L"position");
  args.Add(XSI::MATH::CVector2f(pos.x(), pos.y()));

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);

  return result.GetAsText().GetAsciiString();
}

std::string DFGUICmdHandlerXSI::dfgDoAddSet(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredName,
  FTL::CStrRef varPath,
  QPointF pos
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_AddSet::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"desiredName");
  args.Add(desiredName.c_str());
  args.Add(L"varPath");
  args.Add(varPath.c_str());
  args.Add(L"position");
  args.Add(XSI::MATH::CVector2f(pos.x(), pos.y()));

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);

  return result.GetAsText().GetAsciiString();
}

std::string DFGUICmdHandlerXSI::dfgDoAddGraph(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  QPointF pos
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_AddGraph::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"title");
  args.Add(title.c_str());
  args.Add(L"position");
  args.Add(XSI::MATH::CVector2f(pos.x(), pos.y()));

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);

  return result.GetAsText().GetAsciiString();
}

std::string DFGUICmdHandlerXSI::dfgDoAddFunc(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  FTL::CStrRef code,
  QPointF pos
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_AddFunc::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"title");
  args.Add(title.c_str());
  args.Add(L"code");
  args.Add(code.c_str());
  args.Add(L"position");
  args.Add(XSI::MATH::CVector2f(pos.x(), pos.y()));

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);

  return result.GetAsText().GetAsciiString();
}

void DFGUICmdHandlerXSI::dfgDoRemoveNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodes
  )
{
  // todo
}

void DFGUICmdHandlerXSI::dfgDoConnect(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef srcPort, 
  FTL::CStrRef dstPort
  )
{
  // todo
 }

void DFGUICmdHandlerXSI::dfgDoDisconnect(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef srcPort, 
  FTL::CStrRef dstPort
  )
{
  // todo
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
  return "";
}

void DFGUICmdHandlerXSI::dfgDoRemovePort(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef portName
  )
{
  // todo
}

void DFGUICmdHandlerXSI::dfgDoMoveNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodes,
  FTL::ArrayRef<QPointF> poss
  )
{
  // todo
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
  // todo
}

std::string DFGUICmdHandlerXSI::dfgDoImplodeNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredName,
  FTL::ArrayRef<FTL::CStrRef> nodes
  )
{
  // todo
  return "";
}

std::vector<std::string> DFGUICmdHandlerXSI::dfgDoExplodeNode(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef name
  )
{
  // todo
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
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_AddBackDrop::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"title");
  args.Add(title.c_str());
  args.Add(L"position");
  args.Add(XSI::MATH::CVector2f(pos.x(), pos.y()));

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);
}

void DFGUICmdHandlerXSI::dfgDoSetNodeTitle(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef node,
  FTL::CStrRef title
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_SetNodeTitle::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"node");
  args.Add(node.c_str());
  args.Add(L"title");
  args.Add(title.c_str());

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);
}

void DFGUICmdHandlerXSI::dfgDoSetNodeComment(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef node,
  FTL::CStrRef comment
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_SetNodeComment::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"node");
  args.Add(node.c_str());
  args.Add(L"comment");
  args.Add(comment.c_str());

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);
}

void DFGUICmdHandlerXSI::dfgDoSetCode(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef code
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_SetCode::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"code");
  args.Add(code.c_str());

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);
}

std::string DFGUICmdHandlerXSI::dfgDoRenamePort(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef name,
  FTL::CStrRef desiredName
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_RenamePort::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"name");
  args.Add(name.c_str());
  args.Add(L"desiredName");
  args.Add(desiredName.c_str());

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);

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
  // todo
  std::vector<std::string> result;
  return result;
}

void DFGUICmdHandlerXSI::dfgDoSetArgType(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef name,
  FTL::CStrRef type
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_SetArgType::CmdName().c_str());
  XSI::CValueArray args;
  addBindingToArgs( binding, args );
  args.Add(L"name");
  args.Add(name.c_str());
  args.Add(L"type");
  args.Add(type.c_str());

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);
}

void DFGUICmdHandlerXSI::dfgDoSetArgValue(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef name,
  FabricCore::RTVal const &value
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_SetArgValue::CmdName().c_str());
  XSI::CValueArray args;
  addBindingToArgs( binding, args );
  args.Add(L"name");
  args.Add(name.c_str());
  FabricCore::RTVal valueJSON = value.getJSON();
  args.Add(L"value");
  args.Add(valueJSON.getStringCString());

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);
}

void DFGUICmdHandlerXSI::dfgDoSetPortDefaultValue(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef port,
  FabricCore::RTVal const &value
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_SetPortDefaultValue::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"port");
  args.Add(port.c_str());
  FabricCore::RTVal valueJSON = value.getJSON();
  args.Add(L"value");
  args.Add(valueJSON.getStringCString());

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);
}

void DFGUICmdHandlerXSI::dfgDoSetRefVarPath(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef node,
  FTL::CStrRef varPath
  )
{
  XSI::CString cmdName(FabricUI::DFG::DFGUICmd_SetRefVarPath::CmdName().c_str());
  XSI::CValueArray args;
  addExecToArgs( binding, execPath, exec, args );
  args.Add(L"node");
  args.Add(node.c_str());
  args.Add(L"varPath");
  args.Add(varPath.c_str());

  XSI::CValue result;
  bool ok = executeCommand(cmdName, args, result);
}

std::string DFGUICmdHandlerXSI::getOperatorNameFromBinding( FabricCore::DFGBinding const &binding )
{
  // try to get the operator's name for this binding.
  if (m_parentBaseInterface && m_parentBaseInterface->getBinding() == binding)
  {
    unsigned int opObjID = _opUserData::GetOperatorObjectID(m_parentBaseInterface);
    if (opObjID != UINT_MAX)
    {
      XSI::CRef opRef = XSI::Application().GetObjectFromID(opObjID).GetRef();
      if (opRef.IsValid())
        return opRef.GetAsText().GetAsciiString();
    }
  }

  // not found.
  return "";
}
