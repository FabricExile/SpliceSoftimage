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

FabricCore::DFGBinding DFGUICmdHandlerXSI::getBindingFromOperatorName(std::string operatorName)
{
  // try to get the binding from the operator's name.
  CRef opRef;
  opRef.Set(CString(operatorName.c_str()));
  CustomOperator op(opRef);
  BaseInterface *baseInterface = _opUserData::GetBaseInterface(op.GetObjectID());
  if (baseInterface)
    return baseInterface->getBinding();

  // not found.
  return FabricCore::DFGBinding();
}

// execute a XSI command.
// return values: on success: true and io_result contains the command's return value.
//                on failure: false and io_result has no type.
bool executeCommand(const CString &cmdName, const CValueArray &args, CValue &io_result)
{
  // init result.
  io_result.Clear();

  // log.
  if (DFGUICmdHandlerLOG)
  {
    Application().LogMessage(L"[DFGUICmd] about to execute \"" + cmdName + L"\" with the following array of arguments:", siCommentMsg);
    for (LONG i=0;i<args.GetCount();i++)
      Application().LogMessage(L"[DFGUICmd]     \"" + args[i].GetAsText() + L"\"", siCommentMsg);
  }

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

static inline CStatus HandleFabricCoreException(
  FabricCore::Exception const &e
  )
{
  std::wstring msg = L"[DFGUICmd] Fabric Core exception: ";
  FTL::CStrRef desc = e.getDesc_cstr();
  msg.insert( msg.end(), desc.begin(), desc.end() );
  Application().LogMessage( msg.c_str(), siErrorMsg );
  return CStatus::Fail;
}

static inline std::string EncodeNames(
  FTL::ArrayRef<FTL::CStrRef> names
  )
{
  std::stringstream nameSS;
  for ( FTL::ArrayRef<FTL::CStrRef>::IT it = names.begin();
    it != names.end(); ++it )
  {
    if ( it != names.begin() )
      nameSS << '|';
    nameSS << *it;
  }
  return nameSS.str();
}

static inline std::string EncodeXPoss(
  FTL::ArrayRef<QPointF> poss
  )
{
  std::stringstream xPosSS;
  for ( FTL::ArrayRef<QPointF>::IT it = poss.begin(); it != poss.end(); ++it )
  {
    if ( it != poss.begin() )
      xPosSS << '|';
    xPosSS << it->x();
  }
  return xPosSS.str();
}

static inline std::string EncodeYPoss(
  FTL::ArrayRef<QPointF> poss
  )
{
  std::stringstream yPosSS;
  for ( FTL::ArrayRef<QPointF>::IT it = poss.begin(); it != poss.end(); ++it )
  {
    if ( it != poss.begin() )
      yPosSS << '|';
    yPosSS << it->y();
  }
  return yPosSS.str();
}

static inline void EncodePosition(
  QPointF const &position,
  CValueArray &args
  )
{
  {
    std::stringstream ss;
    ss << position.x();
    args.Add( CString( ss.str().c_str() ) );
  }
  {
    std::stringstream ss;
    ss << position.y();
    args.Add( CString( ss.str().c_str() ) );
  }
}

static inline void EncodeSize(
  QSizeF const &size,
  CValueArray &args
  )
{
  {
    std::stringstream ss;
    ss << size.width();
    args.Add( CString( ss.str().c_str() ) );
  }
  {
    std::stringstream ss;
    ss << size.height();
    args.Add( CString( ss.str().c_str() ) );
  }
}

static inline CStatus DecodeString(
  CValueArray const &args,
  unsigned &ai,
  std::string &value
  )
{
  value = CString( args[ai++] ).GetAsciiString();
  return CStatus::OK;
}

static inline CStatus DecodePosition(
  CValueArray const &args,
  unsigned &ai,
  QPointF &position
  )
{
  position.setX(
    FTL::CStrRef( CString( args[ai++] ).GetAsciiString() ).toFloat64()
    );
  position.setY(
    FTL::CStrRef( CString( args[ai++] ).GetAsciiString() ).toFloat64()
    );
  return CStatus::OK;
}

static inline CStatus DecodeSize(
  CValueArray const &args,
  unsigned &ai,
  QSizeF &size
  )
{
  size.setWidth(
    FTL::CStrRef( CString( args[ai++] ).GetAsciiString() ).toFloat64()
    );
  size.setHeight(
    FTL::CStrRef( CString( args[ai++] ).GetAsciiString() ).toFloat64()
    );
  return CStatus::OK;
}

static inline CStatus DecodeExec(
  CValueArray const &args,
  unsigned &ai,
  FabricCore::DFGBinding &binding,
  std::string &execPath,
  FabricCore::DFGExec &exec
  )
{
  binding =
    DFGUICmdHandlerXSI::getBindingFromOperatorName(
      CString(args[ai++]).GetAsciiString()
      );
  if (!binding.isValid())
  {
    Application().LogMessage(L"[DFGUICmd] invalid binding!", siErrorMsg);
    return CStatus::Fail;
  }

  CStatus execPathStatus = DecodeString( args, ai, execPath );
  if ( execPathStatus != CStatus::OK )
    return execPathStatus;

  try
  {
    exec = binding.getExec().getSubExec( execPath.c_str() );
  }
  catch ( FabricCore::Exception e )
  {
    return HandleFabricCoreException( e );
  }

  return CStatus::OK;
}

static inline CStatus DecodeName(
  CValueArray const &args,
  unsigned &ai,
  std::string &value
  )
{
  return DecodeString( args, ai, value );
}

static inline CStatus DecodeNames(
  CValueArray const &args,
  unsigned &ai,
  std::string &namesString,
  std::vector<FTL::StrRef> &names
  )
{
  CStatus namesStatus = DecodeString( args, ai, namesString );
  if ( namesStatus != CStatus::OK )
    return namesStatus;
  
  FTL::StrRef namesStr = namesString;
  while ( !namesStr.empty() )
  {
    FTL::StrRef::Split split = namesStr.trimSplit('|');
    if ( !split.first.empty() )
      names.push_back( split.first );
    namesStr = split.second;
  }

  return CStatus::OK;
}

static inline CStatus DFGUICmd_Finish(
  Context &ctxt,
  FabricUI::DFG::DFGUICmd *cmd
  )
{
  // store or delete the DFG command, depending on XSI's current undo preferences.
  if ( (bool)ctxt.GetAttribute(L"UndoRequired") == true )
    ctxt.PutAttribute( L"UndoRedoData", (CValue::siPtrType)cmd );
  else
    delete cmd;

  // done.
  return CStatus::OK;
}

static inline CStatus DFGUICmd_Undo( CRef &in_ctxt )
{
  Context ctxt( in_ctxt );
  if ( DFGUICmdHandlerLOG )
    Application().LogMessage(L"[DFGUICmd] Undo", siCommentMsg);
  FabricUI::DFG::DFGUICmd *cmd =
    (FabricUI::DFG::DFGUICmd *)(CValue::siPtrType)ctxt.GetAttribute(L"UndoRedoData");
  if (cmd)  cmd->undo();
  return CStatus::OK;
}

static inline CStatus DFGUICmd_Redo( CRef &in_ctxt )
{
  Context ctxt( in_ctxt );
  if ( DFGUICmdHandlerLOG )
    Application().LogMessage(L"[DFGUICmd] Redo", siCommentMsg);
  FabricUI::DFG::DFGUICmd *cmd =
    (FabricUI::DFG::DFGUICmd *)(CValue::siPtrType)ctxt.GetAttribute(L"UndoRedoData");
  if (cmd)  cmd->redo();
  return CStatus::OK;
}

static inline CStatus DFGUICmd_TermUndoRedo(CRef &in_ctxt)
{
  Context ctxt( in_ctxt );
  if ( DFGUICmdHandlerLOG )
    Application().LogMessage(L"[DFGUICmd] TermUndoRedo", siCommentMsg);
  FabricUI::DFG::DFGUICmd *cmd =
    (FabricUI::DFG::DFGUICmd *)(CValue::siPtrType)ctxt.GetAttribute(L"UndoRedoData");
  if (cmd)  delete cmd;
  return CStatus::OK;
}

/*-----------------------------------------------------
  implementation of DFGUICmdHandlerXSI member functions.
*/

// ---
// command "dfgRemoveNodes"
// ---

SICALLBACK dfgRemoveNodes_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"nodeNames",  CString());
  oArgs.Add(L"xPoss",   CString());
  oArgs.Add(L"yPoss",   CString());

  return CStatus::OK;
}

SICALLBACK dfgRemoveNodes_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgRemoveNodes_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_RemoveNodes *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string nodeNamesString;
    std::vector<FTL::StrRef> nodeNames;
    CStatus nodeNamesStatus =
      DecodeNames( args, ai, nodeNamesString, nodeNames );
    if ( nodeNamesStatus != CStatus::OK )
      return nodeNamesStatus;
    
    cmd = new FabricUI::DFG::DFGUICmd_RemoveNodes( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  nodeNames );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // return DFGUICmd_Finish( ctxt, cmd );
  // store or delete the DFG command, depending on XSI's current undo preferences.
  if ( (bool)ctxt.GetAttribute(L"UndoRequired") == true )
    ctxt.PutAttribute( L"UndoRedoData", (CValue::siPtrType)cmd );
  else
    delete cmd;

  // done.
  return CStatus::OK;
}

SICALLBACK dfgRemoveNodes_Undo(CRef &ctxt)
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgRemoveNodes_Redo(CRef &ctxt)
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgRemoveNodes_TermUndoRedo(CRef &ctxt)
{
  return DFGUICmd_TermUndoRedo( ctxt );
}

void DFGUICmdHandlerXSI::dfgDoRemoveNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodeNames
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_RemoveNodes::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());
  args.Add( EncodeNames( nodeNames ).c_str() );

  executeCommand(cmdName, args, CValue());
}

// ---
// command "dfgConnect"
// ---

SICALLBACK dfgConnect_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"srcPortPath",  CString());
  oArgs.Add(L"dstPortPath",   CString());

  return CStatus::OK;
}

SICALLBACK dfgConnect_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgConnect_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_Connect *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string srcPortPath;
    CStatus srcPortPathStatus =
      DecodeName( args, ai, srcPortPath );
    if ( srcPortPathStatus != CStatus::OK )
      return srcPortPathStatus;

    std::string dstPortPath;
    CStatus dstPortPathStatus =
      DecodeName( args, ai, dstPortPath );
    if ( dstPortPathStatus != CStatus::OK )
      return dstPortPathStatus;
    
    cmd = new FabricUI::DFG::DFGUICmd_Connect( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  srcPortPath.c_str(),
                                                  dstPortPath.c_str() );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgConnect_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgConnect_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgConnect_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());
  args.Add( srcPort.c_str() );
  args.Add( dstPort.c_str() );

  executeCommand(cmdName, args, CValue());
}

// ---
// command "dfgDisconnect"
// ---

SICALLBACK dfgDisconnect_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"srcPortPath",  CString());
  oArgs.Add(L"dstPortPath",   CString());

  return CStatus::OK;
}

SICALLBACK dfgDisconnect_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgDisconnect_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_Disconnect *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string srcPortPath;
    CStatus srcPortPathStatus =
      DecodeName( args, ai, srcPortPath );
    if ( srcPortPathStatus != CStatus::OK )
      return srcPortPathStatus;

    std::string dstPortPath;
    CStatus dstPortPathStatus =
      DecodeName( args, ai, dstPortPath );
    if ( dstPortPathStatus != CStatus::OK )
      return dstPortPathStatus;
    
    cmd = new FabricUI::DFG::DFGUICmd_Disconnect( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  srcPortPath.c_str(),
                                                  dstPortPath.c_str() );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgDisconnect_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgDisconnect_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgDisconnect_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());
  args.Add( srcPort.c_str() );
  args.Add( dstPort.c_str() );

  executeCommand(cmdName, args, CValue());
}

// ---
// command "dfgAddGraph"
// ---

SICALLBACK dfgAddGraph_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"title",  CString());
  oArgs.Add(L"xPos",   CString());
  oArgs.Add(L"yPos",   CString());

  return CStatus::OK;
}

SICALLBACK dfgAddGraph_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgAddGraph_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_AddGraph *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string title;
    CStatus titleStatus = DecodeString( args, ai, title );
    if ( titleStatus != CStatus::OK )
      return titleStatus;

    QPointF position;
    CStatus positionStatus = DecodePosition( args, ai, position );
    if ( positionStatus != CStatus::OK )
      return positionStatus;

    cmd = new FabricUI::DFG::DFGUICmd_AddGraph( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  title.c_str(),
                                                  position );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // store return value.
  CValue returnValue(CString(cmd->getActualNodeName().c_str()));
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue.GetAsText() + L"\"", siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgAddGraph_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgAddGraph_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgAddGraph_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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
  EncodePosition( pos, args );

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

// ---
// command "dfgAddFunc"
// ---

SICALLBACK dfgAddFunc_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"title",  CString());
  oArgs.Add(L"initialCode",  CString());
  oArgs.Add(L"xPos",   CString());
  oArgs.Add(L"yPos",   CString());

  return CStatus::OK;
}

SICALLBACK dfgAddFunc_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgAddFunc_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_AddFunc *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string title;
    CStatus titleStatus = DecodeString( args, ai, title );
    if ( titleStatus != CStatus::OK )
      return titleStatus;

    std::string initialCode;
    CStatus initialCodeStatus = DecodeString( args, ai, initialCode );
    if ( initialCodeStatus != CStatus::OK )
      return initialCodeStatus;

    QPointF position;
    CStatus positionStatus = DecodePosition( args, ai, position );
    if ( positionStatus != CStatus::OK )
      return positionStatus;

    cmd = new FabricUI::DFG::DFGUICmd_AddFunc( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  title.c_str(),
                                                  initialCode.c_str(),
                                                  position );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // store return value.
  CValue returnValue(CString(cmd->getActualNodeName().c_str()));
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue.GetAsText() + L"\"", siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgAddFunc_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgAddFunc_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgAddFunc_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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
  EncodePosition( pos, args );

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

// ---
// command "dfgInstPreset"
// ---

SICALLBACK dfgInstPreset_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"presetPath",  CString());
  oArgs.Add(L"xPos",   CString());
  oArgs.Add(L"yPos",   CString());

  return CStatus::OK;
}

SICALLBACK dfgInstPreset_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgInstPreset_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_InstPreset *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string presetPath;
    CStatus presetPathStatus = DecodeString( args, ai, presetPath );
    if ( presetPathStatus != CStatus::OK )
      return presetPathStatus;

    QPointF position;
    CStatus positionStatus = DecodePosition( args, ai, position );
    if ( positionStatus != CStatus::OK )
      return positionStatus;

    cmd = new FabricUI::DFG::DFGUICmd_InstPreset( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  presetPath.c_str(),
                                                  position );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // store return value.
  CValue returnValue(CString(cmd->getActualNodeName().c_str()));
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue.GetAsText() + L"\"", siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgInstPreset_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgInstPreset_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgInstPreset_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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
  args.Add(presetPath.c_str());                           // presetPath.
  EncodePosition( pos, args );

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

// ---
// command "dfgAddVar"
// ---

SICALLBACK dfgAddVar_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"desiredNodeName",  CString());
  oArgs.Add(L"dataType",  CString());
  oArgs.Add(L"extDep",  CString());
  oArgs.Add(L"xPos",   CString());
  oArgs.Add(L"yPos",   CString());

  return CStatus::OK;
}

SICALLBACK dfgAddVar_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgAddVar_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_AddVar *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string desiredNodeName;
    CStatus desiredNodeNameStatus = DecodeString( args, ai, desiredNodeName );
    if ( desiredNodeNameStatus != CStatus::OK )
      return desiredNodeNameStatus;

    std::string dataType;
    CStatus dataTypeStatus = DecodeString( args, ai, dataType );
    if ( dataTypeStatus != CStatus::OK )
      return dataTypeStatus;

    std::string extDep;
    CStatus extDepStatus = DecodeString( args, ai, extDep );
    if ( extDepStatus != CStatus::OK )
      return extDepStatus;

    QPointF position;
    CStatus positionStatus = DecodePosition( args, ai, position );
    if ( positionStatus != CStatus::OK )
      return positionStatus;

    cmd = new FabricUI::DFG::DFGUICmd_AddVar( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  desiredNodeName.c_str(),
                                                  dataType.c_str(),
                                                  extDep.c_str(),
                                                  position );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // store return value.
  CValue returnValue(CString(cmd->getActualNodeName().c_str()));
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue.GetAsText() + L"\"", siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgAddVar_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgAddVar_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgAddVar_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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
  EncodePosition( pos, args );

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

// ---
// command "dfgAddGet"
// ---

SICALLBACK dfgAddGet_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"desiredNodeName",  CString());
  oArgs.Add(L"varPath",  CString());
  oArgs.Add(L"xPos",   CString());
  oArgs.Add(L"yPos",   CString());

  return CStatus::OK;
}

SICALLBACK dfgAddGet_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgAddGet_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_AddGet *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string desiredNodeName;
    CStatus desiredNodeNameStatus = DecodeString( args, ai, desiredNodeName );
    if ( desiredNodeNameStatus != CStatus::OK )
      return desiredNodeNameStatus;

    std::string varPath;
    CStatus varPathStatus = DecodeString( args, ai, varPath );
    if ( varPathStatus != CStatus::OK )
      return varPathStatus;

    QPointF position;
    CStatus positionStatus = DecodePosition( args, ai, position );
    if ( positionStatus != CStatus::OK )
      return positionStatus;

    cmd = new FabricUI::DFG::DFGUICmd_AddGet( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  desiredNodeName.c_str(),
                                                  varPath.c_str(),
                                                  position );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // store return value.
  CValue returnValue(CString(cmd->getActualNodeName().c_str()));
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue.GetAsText() + L"\"", siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgAddGet_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgAddGet_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgAddGet_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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
  EncodePosition( pos, args );

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

// ---
// command "dfgAddSet"
// ---

SICALLBACK dfgAddSet_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"desiredNodeName",  CString());
  oArgs.Add(L"varPath",  CString());
  oArgs.Add(L"xPos",   CString());
  oArgs.Add(L"yPos",   CString());

  return CStatus::OK;
}

SICALLBACK dfgAddSet_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgAddSet_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_AddSet *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string desiredNodeName;
    CStatus desiredNodeNameStatus = DecodeString( args, ai, desiredNodeName );
    if ( desiredNodeNameStatus != CStatus::OK )
      return desiredNodeNameStatus;

    std::string varPath;
    CStatus varPathStatus = DecodeString( args, ai, varPath );
    if ( varPathStatus != CStatus::OK )
      return varPathStatus;

    QPointF position;
    CStatus positionStatus = DecodePosition( args, ai, position );
    if ( positionStatus != CStatus::OK )
      return positionStatus;

    cmd = new FabricUI::DFG::DFGUICmd_AddSet( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  desiredNodeName.c_str(),
                                                  varPath.c_str(),
                                                  position );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // store return value.
  CValue returnValue(CString(cmd->getActualNodeName().c_str()));
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue.GetAsText() + L"\"", siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgAddSet_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgAddSet_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgAddSet_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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
  EncodePosition( pos, args );

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
}

// ---
// command "dfgAddPort"
// ---

SICALLBACK dfgAddPort_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"desiredPortName",  CString());
  oArgs.Add(L"portType",  CString());
  oArgs.Add(L"typeSpec",   CString());
  oArgs.Add(L"portToConnect",   CString());

  return CStatus::OK;
}

SICALLBACK dfgAddPort_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgAddPort_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_AddPort *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string desiredPortName;
    CStatus desiredPortNameStatus = DecodeString( args, ai, desiredPortName );
    if ( desiredPortNameStatus != CStatus::OK )
      return desiredPortNameStatus;

    std::string portTypeString;
    CStatus portTypeStatus = DecodeString( args, ai, portTypeString );
    if ( portTypeStatus != CStatus::OK )
      return portTypeStatus;
    FabricCore::DFGPortType portType;
    if ( portTypeString == "In" || portTypeString == "in" )
      portType = FabricCore::DFGPortType_In;
    else if ( portTypeString == "IO" || portTypeString == "io" )
      portType = FabricCore::DFGPortType_IO;
    else if ( portTypeString == "Out" || portTypeString == "out" )
      portType = FabricCore::DFGPortType_Out;
    else
    {
      std::wstring msg;
      msg += L"[DFGUICmd] Unrecognize port type '";
      msg.insert( msg.end(), portTypeString.begin(), portTypeString.end() );
      msg += L'\'';
      Application().LogMessage( msg.c_str(), siErrorMsg );
      return CStatus::Fail;
    }

    std::string typeSpec;
    CStatus typeSpecStatus = DecodeString( args, ai, typeSpec );
    if ( typeSpecStatus != CStatus::OK )
      return typeSpecStatus;

    std::string portToConnectWith;
    CStatus portToConnectWithStatus = DecodeString( args, ai, portToConnectWith );
    if ( portToConnectWithStatus != CStatus::OK )
      return portToConnectWithStatus;

    cmd = new FabricUI::DFG::DFGUICmd_AddPort( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  desiredPortName.c_str(),
                                                  portType,
                                                  typeSpec.c_str(),
                                                  portToConnectWith.c_str() );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // store return value.
  CValue returnValue(CString(cmd->getActualPortName().c_str()));
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue.GetAsText() + L"\"", siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgAddPort_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgAddPort_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgAddPort_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());                             // execPath.
  args.Add(desiredPortName.c_str());                      // desiredNodeName.
  FTL::CStrRef portTypeStr;
  switch ( portType )
  {
    case FabricCore::DFGPortType_In:
      portTypeStr = FTL_STR("In");
      break;
    case FabricCore::DFGPortType_IO:
      portTypeStr = FTL_STR("IO");
      break;
    case FabricCore::DFGPortType_Out:
      portTypeStr = FTL_STR("Out");
      break;
  }
  args.Add(portTypeStr.c_str());                      // desiredNodeName.
  args.Add(typeSpec.c_str());                      // desiredNodeName.
  args.Add(portToConnect.c_str());                      // desiredNodeName.

  CValue result;
  executeCommand(cmdName, args, result);
  return result.GetAsText().GetAsciiString();
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

// ---
// command "dfgMoveNodes"
// ---

SICALLBACK dfgMoveNodes_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"nodeNames",  CString());
  oArgs.Add(L"xPoss",   CString());
  oArgs.Add(L"yPoss",   CString());

  return CStatus::OK;
}

SICALLBACK dfgMoveNodes_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgMoveNodes_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_MoveNodes *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string nodeNamesString;
    std::vector<FTL::StrRef> nodeNames;
    CStatus nodeNamesStatus =
      DecodeNames( args, ai, nodeNamesString, nodeNames );
    if ( nodeNamesStatus != CStatus::OK )
      return nodeNamesStatus;

    std::vector<QPointF> poss;
    std::string xPosString = CString(args[ai++]).GetAsciiString();
    FTL::StrRef xPosStr = xPosString;
    std::string yPosString = CString(args[ai++]).GetAsciiString();
    FTL::StrRef yPosStr = yPosString;
    while ( !xPosStr.empty() && !yPosStr.empty() )
    {
      QPointF pos;
      FTL::StrRef::Split xPosSplit = xPosStr.trimSplit('|');
      if ( !xPosSplit.first.empty() )
        pos.setX( xPosSplit.first.toFloat64() );
      xPosStr = xPosSplit.second;
      FTL::StrRef::Split yPosSplit = yPosStr.trimSplit('|');
      if ( !yPosSplit.first.empty() )
        pos.setY( yPosSplit.first.toFloat64() );
      yPosStr = yPosSplit.second;
      poss.push_back( pos );
    }

    cmd = new FabricUI::DFG::DFGUICmd_MoveNodes( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  nodeNames,
                                                  poss );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgMoveNodes_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgMoveNodes_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgMoveNodes_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
}

void DFGUICmdHandlerXSI::dfgDoMoveNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodeNames,
  FTL::ArrayRef<QPointF> poss
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_MoveNodes::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());
  args.Add( EncodeNames( nodeNames ).c_str() );
  args.Add( EncodeXPoss( poss ).c_str() );
  args.Add( EncodeYPoss( poss ).c_str() );

  executeCommand(cmdName, args, CValue());
}

// ---
// command "dfgResizeBackDrop"
// ---

SICALLBACK dfgResizeBackDrop_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"backDropName",   CString());
  oArgs.Add(L"xPos",   CString());
  oArgs.Add(L"yPos",   CString());
  oArgs.Add(L"width",   CString());
  oArgs.Add(L"height",   CString());

  return CStatus::OK;
}

SICALLBACK dfgResizeBackDrop_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgResizeBackDrop_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_ResizeBackDrop *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string backDropName;
    CStatus backDropNameStatus =
      DecodeName( args, ai, backDropName );
    if ( backDropNameStatus != CStatus::OK )
      return backDropNameStatus;

    QPointF position;
    CStatus positionStatus = DecodePosition( args, ai, position );
    if ( positionStatus != CStatus::OK )
      return positionStatus;

    QSizeF size;
    CStatus sizeStatus = DecodeSize( args, ai, size );
    if ( sizeStatus != CStatus::OK )
      return sizeStatus;

    cmd = new FabricUI::DFG::DFGUICmd_ResizeBackDrop( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  backDropName.c_str(),
                                                  position,
                                                  size );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgResizeBackDrop_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgResizeBackDrop_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgResizeBackDrop_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
}


void DFGUICmdHandlerXSI::dfgDoResizeBackDrop(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef backDropName,
  QPointF pos,
  QSizeF size
  )
{
  CString cmdName(FabricUI::DFG::DFGUICmd_ResizeBackDrop::CmdName().c_str());
  CValueArray args;

  args.Add(getOperatorNameFromBinding(binding).c_str());  // binding.
  args.Add(execPath.c_str());
  args.Add( backDropName.c_str() );
  EncodePosition( pos, args );
  EncodeSize( size, args );

  executeCommand(cmdName, args, CValue());
}

std::string DFGUICmdHandlerXSI::dfgDoImplodeNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodeNames,
  FTL::CStrRef desiredNodeName
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

// ---
// command "dfgAddBackDrop"
// ---

SICALLBACK dfgAddBackDrop_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"title",  CString());
  oArgs.Add(L"xPos",   CString());
  oArgs.Add(L"yPos",   CString());

  return CStatus::OK;
}

SICALLBACK dfgAddBackDrop_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgAddBackDrop_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_AddBackDrop *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string title;
    CStatus titleStatus = DecodeString( args, ai, title );
    if ( titleStatus != CStatus::OK )
      return titleStatus;

    QPointF position;
    CStatus positionStatus = DecodePosition( args, ai, position );
    if ( positionStatus != CStatus::OK )
      return positionStatus;

    cmd = new FabricUI::DFG::DFGUICmd_AddBackDrop( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  title.c_str(),
                                                  position );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // store return value.
  CValue returnValue(CString(cmd->getActualNodeName().c_str()));
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue.GetAsText() + L"\"", siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgAddBackDrop_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgAddBackDrop_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgAddBackDrop_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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
  EncodePosition( pos, args );

  executeCommand(cmdName, args, CValue());
}

// ---
// command "dfgSetNodeTitle"
// ---

SICALLBACK dfgSetNodeTitle_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"nodeName",   CString());
  oArgs.Add(L"title",   CString());

  return CStatus::OK;
}

SICALLBACK dfgSetNodeTitle_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgSetNodeTitle_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_SetNodeTitle *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string nodeName;
    CStatus nodeNameStatus =
      DecodeName( args, ai, nodeName );
    if ( nodeNameStatus != CStatus::OK )
      return nodeNameStatus;

    std::string title;
    CStatus titleStatus =
      DecodeName( args, ai, title );
    if ( titleStatus != CStatus::OK )
      return titleStatus;

    cmd = new FabricUI::DFG::DFGUICmd_SetNodeTitle( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  nodeName.c_str(),
                                                  title.c_str() );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgSetNodeTitle_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgSetNodeTitle_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgSetNodeTitle_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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

// ---
// command "dfgSetNodeComment"
// ---

SICALLBACK dfgSetNodeComment_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"nodeName",   CString());
  oArgs.Add(L"comment",   CString());

  return CStatus::OK;
}

SICALLBACK dfgSetNodeComment_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgSetNodeComment_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_SetNodeComment *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string nodeName;
    CStatus nodeNameStatus =
      DecodeName( args, ai, nodeName );
    if ( nodeNameStatus != CStatus::OK )
      return nodeNameStatus;

    std::string comment;
    CStatus commentStatus =
      DecodeName( args, ai, comment );
    if ( commentStatus != CStatus::OK )
      return commentStatus;

    cmd = new FabricUI::DFG::DFGUICmd_SetNodeComment( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  nodeName.c_str(),
                                                  comment.c_str() );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgSetNodeComment_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgSetNodeComment_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgSetNodeComment_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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

// ---
// command "dfgSetCode"
// ---

SICALLBACK dfgSetCode_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"code",   CString());

  return CStatus::OK;
}

SICALLBACK dfgSetCode_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgSetCode_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_SetCode *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string code;
    CStatus codeStatus =
      DecodeName( args, ai, code );
    if ( codeStatus != CStatus::OK )
      return codeStatus;

    cmd = new FabricUI::DFG::DFGUICmd_SetCode( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  code.c_str() );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgSetCode_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgSetCode_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgSetCode_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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

// ---
// command "dfgSetRefVarPath"
// ---

SICALLBACK dfgSetRefVarPath_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     CString());
  oArgs.Add(L"execPath",    CString());
  oArgs.Add(L"refName",   CString());
  oArgs.Add(L"varPath",   CString());

  return CStatus::OK;
}

SICALLBACK dfgSetRefVarPath_Execute(CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) Application().LogMessage(L"[DFGUICmd] calling dfgSetRefVarPath_Execute()", siCommentMsg);

  // init.
  Context ctxt(in_ctxt);
  CValueArray args = ctxt.GetAttribute(L"Arguments");

  // create the DFG command.
  FabricUI::DFG::DFGUICmd_SetRefVarPath *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    CStatus execStatus = DecodeExec( args, ai, binding, execPath, exec );
    if ( execStatus != CStatus::OK )
      return execStatus;

    std::string refName;
    CStatus refNameStatus =
      DecodeName( args, ai, refName );
    if ( refNameStatus != CStatus::OK )
      return refNameStatus;

    std::string varPath;
    CStatus varPathStatus =
      DecodeName( args, ai, varPath );
    if ( varPathStatus != CStatus::OK )
      return varPathStatus;

    cmd = new FabricUI::DFG::DFGUICmd_SetRefVarPath( binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  refName.c_str(),
                                                  varPath.c_str() );
  }

  // execute the DFG command.
  try
  {
    cmd->doit();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  return DFGUICmd_Finish( ctxt, cmd );
}

SICALLBACK dfgSetRefVarPath_Undo( CRef &ctxt )
{
  return DFGUICmd_Undo( ctxt );
}

SICALLBACK dfgSetRefVarPath_Redo( CRef &ctxt )
{
  return DFGUICmd_Redo( ctxt );
}

SICALLBACK dfgSetRefVarPath_TermUndoRedo( CRef &ctxt )
{
  return DFGUICmd_TermUndoRedo( ctxt );
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
