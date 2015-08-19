#include "FabricDFGUICmdHandlerDCC.h"
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



/*----------------
  helper functions.
*/



bool execCmd(const std::string &in_cmdName, const std::vector<std::string> &in_args, std::string &io_result)
{
  /*
    executes a DCC command.

    return values: on success: true and io_result contains the command's return value.
                   on failure: false and io_result is empty.
  */

  // init result.
  io_result = "";

  // log.
  if (DFGUICmdHandlerLOG)
  {
    feLog("[DFGUICmd] about to execute \"" + in_cmdName + "\" with the following array of arguments:");
    for (LONG i=0;i<in_args.size();i++)
      feLog("[DFGUICmd]     \"" + in_args[i] + "\"");
  }

  // execute DCC command.
  bool ret = false;
  {
    // do it.
    XSI::CValue result;
    XSI::CValueArray args;
    for (int i=0;i<in_args.size();i++)
      args.Add(XSI::CString(in_args[i].c_str()));
    ret = (XSI::Application().ExecuteCommand(XSI::CString(in_cmdName.c_str()), args, result) == XSI::CStatus::OK);

    // store result in io_result.
    if (ret)  io_result = XSI::CString(result).GetAsciiString();
  }

  // failed?
  if (!ret)
  {
    // log some info about this command.
    feLogError("failed to execute \"" + in_cmdName + "\" with the following array of arguments:");
    for (LONG i=0;i<in_args.size();i++)
      feLogError("    \"" + in_args[i] + "\"");
  }

  // done.
  return ret;
}

static inline bool HandleFabricCoreException(FabricCore::Exception const &e)
{
  std::string msg = "[DFGUICmd] Fabric Core exception: ";
  FTL::CStrRef desc = e.getDesc_cstr();
  msg.insert(msg.end(), desc.begin(), desc.end());
  feLogError(msg);
  return false;
}

static inline std::string EncodeNames(FTL::ArrayRef<FTL::CStrRef> names)
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

static inline std::string EncodeXPoss(FTL::ArrayRef<QPointF> poss)
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

static inline std::string EncodeYPoss(FTL::ArrayRef<QPointF> poss)
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

static inline void EncodePosition(QPointF const &position, std::vector<std::string> &args)
{
  {
    std::stringstream ss;
    ss << position.x();
    args.push_back(ss.str());
  }
  {
    std::stringstream ss;
    ss << position.y();
    args.push_back(ss.str());
  }
}

static inline void EncodeSize(QSizeF const &size, std::vector<std::string> &args)
{
  {
    std::stringstream ss;
    ss << size.width();
    args.push_back(ss.str());
  }
  {
    std::stringstream ss;
    ss << size.height();
    args.push_back(ss.str());
  }
}

static inline bool DecodeString(std::vector<std::string> const &args, unsigned &ai, std::string &value)
{
  value = args[ai++];
  return true;
}

static inline bool DecodePosition(std::vector<std::string> const &args, unsigned &ai, QPointF &position)
{
  position.setX(FTL::CStrRef(args[ai++]).toFloat64());
  position.setY(FTL::CStrRef(args[ai++]).toFloat64());
  return true;
}

static inline bool DecodeSize(std::vector<std::string> const &args, unsigned &ai, QSizeF &size)
{
  size.setWidth (FTL::CStrRef(args[ai++]).toFloat64());
  size.setHeight(FTL::CStrRef(args[ai++]).toFloat64());
  return true;
}

static inline bool DecodeBinding(std::vector<std::string> const &args, unsigned &ai, FabricCore::DFGBinding &binding)
{
  binding = DFGUICmdHandlerDCC::getBindingFromOperatorName(args[ai++]);
  if (!binding.isValid())
  {
    feLogError("[DFGUICmd] invalid binding!");
    return false;
  }
  return true;
}

static inline bool DecodeExec(std::vector<std::string> const &args, unsigned &ai, FabricCore::DFGBinding &binding, std::string &execPath, FabricCore::DFGExec &exec)
{
  if (!DecodeBinding(args, ai, binding))
    return false;

  if (!DecodeString(args, ai, execPath))
    return false;
  
  try
  {
    exec = binding.getExec().getSubExec(execPath.c_str());
  }
  catch (FabricCore::Exception e)
  {
    return HandleFabricCoreException(e);
  }

  return true;
}

static inline bool DecodeName(std::vector<std::string> const &args, unsigned &ai, std::string &value)
{
  return DecodeString(args, ai, value);
}

static inline bool DecodeNames(std::vector<std::string> const &args, unsigned &ai, std::string &namesString, std::vector<FTL::StrRef> &names)
{
  if (!DecodeString(args, ai, namesString))
    return false;
  
  FTL::StrRef namesStr = namesString;
  while (!namesStr.empty())
  {
    FTL::StrRef::Split split = namesStr.trimSplit('|');
    if (!split.first.empty())
      names.push_back(split.first);
    namesStr = split.second;
  }

  return true;
}

FabricUI::DFG::DFGUICmd_RemoveNodes *createAndExecuteDFGCommand_RemoveNodes(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_RemoveNodes *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string nodeNamesString;
    std::vector<FTL::StrRef> nodeNames;
    if (!DecodeNames(args, ai, nodeNamesString, nodeNames))
      return cmd;
    
    cmd = new FabricUI::DFG::DFGUICmd_RemoveNodes(binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  nodeNames);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }
  return cmd;
}

FabricUI::DFG::DFGUICmd_Connect *createAndExecuteDFGCommand_Connect(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_Connect *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string srcPortPath;
    if (!DecodeName(args, ai, srcPortPath))
      return cmd;

    std::string dstPortPath;
    if (!DecodeName(args, ai, dstPortPath))
      return cmd;
    
    cmd = new FabricUI::DFG::DFGUICmd_Connect(binding,
                                              execPath.c_str(),
                                              exec,
                                              srcPortPath.c_str(),
                                              dstPortPath.c_str());
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }
  return cmd;
}

FabricUI::DFG::DFGUICmd_Disconnect *createAndExecuteDFGCommand_Disconnect(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_Disconnect *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string srcPortPath;
    if (!DecodeName(args, ai, srcPortPath))
      return cmd;

    std::string dstPortPath;
    if (!DecodeName(args, ai, dstPortPath))
      return cmd;
    
    cmd = new FabricUI::DFG::DFGUICmd_Disconnect(binding,
                                                 execPath.c_str(),
                                                 exec,
                                                 srcPortPath.c_str(),
                                                 dstPortPath.c_str());
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }
  return cmd;
}

FabricUI::DFG::DFGUICmd_AddGraph *createAndExecuteDFGCommand_AddGraph(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_AddGraph *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string title;
    if (!DecodeString(args, ai, title))
      return cmd;

    QPointF position;
    if (!DecodePosition(args, ai, position))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_AddGraph(binding,
                                               execPath.c_str(),
                                               exec,
                                               title.c_str(),
                                               position);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_AddFunc *createAndExecuteDFGCommand_AddFunc(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_AddFunc *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string title;
    if (!DecodeString(args, ai, title))
      return cmd;

    std::string initialCode;
    if (!DecodeString(args, ai, initialCode))
      return cmd;

    QPointF position;
    if (!DecodePosition(args, ai, position))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_AddFunc(binding,
                                              execPath.c_str(),
                                              exec,
                                              title.c_str(),
                                              initialCode.c_str(),
                                              position);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }
  return cmd;
}

FabricUI::DFG::DFGUICmd_InstPreset *createAndExecuteDFGCommand_InstPreset(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_InstPreset *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string presetPath;
    if (!DecodeString(args, ai, presetPath))
      return cmd;

    QPointF position;
    if (!DecodePosition(args, ai, position))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_InstPreset(binding,
                                                 execPath.c_str(),
                                                 exec,
                                                 presetPath.c_str(),
                                                 position);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_AddVar *createAndExecuteDFGCommand_AddVar(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_AddVar *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string desiredNodeName;
    if (!DecodeString(args, ai, desiredNodeName))
      return cmd;

    std::string dataType;
    if (!DecodeString(args, ai, dataType))
      return cmd;

    std::string extDep;
    if (!DecodeString(args, ai, extDep))
      return cmd;

    QPointF position;
    if (!DecodePosition(args, ai, position))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_AddVar(binding,
                                             execPath.c_str(),
                                             exec,
                                             desiredNodeName.c_str(),
                                             dataType.c_str(),
                                             extDep.c_str(),
                                             position);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_AddGet *createAndExecuteDFGCommand_AddGet(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_AddGet *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string desiredNodeName;
    if (!DecodeString(args, ai, desiredNodeName))
      return cmd;

    std::string varPath;
    if (!DecodeString(args, ai, varPath))
      return cmd;

    QPointF position;
    if (!DecodePosition(args, ai, position))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_AddGet(binding,
                                             execPath.c_str(),
                                             exec,
                                             desiredNodeName.c_str(),
                                             varPath.c_str(),
                                             position);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_AddSet *createAndExecuteDFGCommand_AddSet(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_AddSet *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string desiredNodeName;
    if (!DecodeString(args, ai, desiredNodeName))
      return cmd;

    std::string varPath;
    if (!DecodeString(args, ai, varPath))
      return cmd;

    QPointF position;
    if (!DecodePosition(args, ai, position))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_AddSet(binding,
                                             execPath.c_str(),
                                             exec,
                                             desiredNodeName.c_str(),
                                             varPath.c_str(),
                                             position);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_AddPort *createAndExecuteDFGCommand_AddPort(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_AddPort *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string desiredPortName;
    if (!DecodeString(args, ai, desiredPortName))
      return cmd;

    std::string portTypeString;
    if (!DecodeString(args, ai, portTypeString))
      return cmd;
    FabricCore::DFGPortType portType;
    if      (portTypeString == "In"  || portTypeString == "in" )  portType = FabricCore::DFGPortType_In;
    else if (portTypeString == "IO"  || portTypeString == "io" )  portType = FabricCore::DFGPortType_IO;
    else if (portTypeString == "Out" || portTypeString == "out")  portType = FabricCore::DFGPortType_Out;
    else
    {
      std::string msg;
      msg += "[DFGUICmd] Unrecognize port type \"";
      msg.insert( msg.end(), portTypeString.begin(), portTypeString.end() );
      msg += "\"";
      feLogError(msg);
      return cmd;
    }

    std::string typeSpec;
    if (!DecodeString(args, ai, typeSpec))
      return cmd;

    std::string portToConnectWith;
    if (!DecodeString(args, ai, portToConnectWith))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_AddPort(binding,
                                              execPath.c_str(),
                                              exec,
                                              desiredPortName.c_str(),
                                              portType,
                                              typeSpec.c_str(),
                                              portToConnectWith.c_str());
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_RemovePort *createAndExecuteDFGCommand_RemovePort(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_RemovePort *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string portName;
    if (!DecodeName(args, ai, portName))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_RemovePort(binding,
                                                 execPath.c_str(),
                                                 exec,
                                                 portName.c_str());
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_MoveNodes *createAndExecuteDFGCommand_MoveNodes(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_MoveNodes *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string nodeNamesString;
    std::vector<FTL::StrRef> nodeNames;
    if (!DecodeNames(args, ai, nodeNamesString, nodeNames))
      return cmd;

    std::vector<QPointF> poss;
    std::string xPosString = args[ai++];
    FTL::StrRef xPosStr = xPosString;
    std::string yPosString = args[ai++];
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

    cmd = new FabricUI::DFG::DFGUICmd_MoveNodes(binding,
                                                execPath.c_str(),
                                                exec,
                                                nodeNames,
                                                poss);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_ResizeBackDrop *createAndExecuteDFGCommand_ResizeBackDrop(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_ResizeBackDrop *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string backDropName;
    if (!DecodeName(args, ai, backDropName))
      return cmd;

    QPointF position;
    if (!DecodePosition(args, ai, position))
      return cmd;

    QSizeF size;
    if (!DecodeSize(args, ai, size))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_ResizeBackDrop(binding,
                                                     execPath.c_str(),
                                                     exec,
                                                     backDropName.c_str(),
                                                     position,
                                                     size);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_ImplodeNodes *createAndExecuteDFGCommand_ImplodeNodes(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_ImplodeNodes *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string nodeNamesString;
    std::vector<FTL::StrRef> nodeNames;
    if (!DecodeNames(args, ai, nodeNamesString, nodeNames))
      return cmd;

    std::string desiredImplodedNodeName;
    if (!DecodeString(args, ai, desiredImplodedNodeName))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_ImplodeNodes(binding,
                                                   execPath.c_str(),
                                                   exec,
                                                   nodeNames,
                                                   desiredImplodedNodeName.c_str());
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_ExplodeNode *createAndExecuteDFGCommand_ExplodeNode(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_ExplodeNode *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string nodeName;
    if (!DecodeName(args, ai, nodeName))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_ExplodeNode(binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  nodeName.c_str());
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_AddBackDrop *createAndExecuteDFGCommand_AddBackDrop(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_AddBackDrop *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string title;
    if (!DecodeString(args, ai, title))
      return cmd;

    QPointF position;
    if (!DecodePosition(args, ai, position))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_AddBackDrop(binding,
                                                  execPath.c_str(),
                                                  exec,
                                                  title.c_str(),
                                                  position);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_SetNodeTitle *createAndExecuteDFGCommand_SetNodeTitle(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_SetNodeTitle *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string nodeName;
    if (!DecodeName(args, ai, nodeName))
      return cmd;

    std::string title;
    if (!DecodeName(args, ai, title))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_SetNodeTitle(binding,
                                                   execPath.c_str(),
                                                   exec,
                                                   nodeName.c_str(),
                                                   title.c_str());
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_SetNodeComment *createAndExecuteDFGCommand_SetNodeComment(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_SetNodeComment *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string nodeName;
    if (!DecodeName(args, ai, nodeName))
      return cmd;

    std::string comment;
    if (!DecodeName(args, ai, comment))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_SetNodeComment(binding,
                                                     execPath.c_str(),
                                                     exec,
                                                     nodeName.c_str(),
                                                     comment.c_str());
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_SetCode *createAndExecuteDFGCommand_SetCode(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_SetCode *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string code;
    if (!DecodeName(args, ai, code))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_SetCode(binding,
                                              execPath.c_str(),
                                              exec,
                                              code.c_str());
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_RenamePort *createAndExecuteDFGCommand_RenamePort(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_RenamePort *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string oldPortName;
    if (!DecodeString(args, ai, oldPortName))
      return cmd;

    std::string desiredNewPortName;
    if (!DecodeString(args, ai, desiredNewPortName))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_RenamePort(binding,
                                                 execPath.c_str(),
                                                 exec,
                                                 oldPortName.c_str(),
                                                 desiredNewPortName.c_str());
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_Paste *createAndExecuteDFGCommand_Paste(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_Paste *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string text;
    if (!DecodeName(args, ai, text))
      return cmd;

    QPointF position;
    if (!DecodePosition(args, ai, position))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_Paste(binding,
                                            execPath.c_str(),
                                            exec,
                                            text.c_str(),
                                            position);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_SetArgType *createAndExecuteDFGCommand_SetArgType(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_SetArgType *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    if (!DecodeBinding(args, ai, binding))
      return cmd;

    std::string argName;
    if (!DecodeName(args, ai, argName))
      return cmd;

    std::string typeName;
    if (!DecodeName(args, ai, typeName))
      return cmd;
    
    cmd = new FabricUI::DFG::DFGUICmd_SetArgType(binding,
                                                 argName.c_str(),
                                                 typeName.c_str());
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_SetArgValue *createAndExecuteDFGCommand_SetArgValue(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_SetArgValue *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    if (!DecodeBinding(args, ai, binding))
      return cmd;

    std::string argName;
    if (!DecodeName(args, ai, argName))
      return cmd;

    std::string typeName;
    if (!DecodeName(args, ai, typeName))
      return cmd;
  
    std::string valueJSON;
    if (!DecodeString(args, ai, valueJSON))
      return cmd;
    FabricCore::DFGHost host    = binding.getHost();
    FabricCore::Context context = host.getContext();
    FabricCore::RTVal   value   = FabricCore::RTVal::Construct( context, typeName.c_str(), 0, NULL );
    value.setJSON(valueJSON.c_str());

    cmd = new FabricUI::DFG::DFGUICmd_SetArgValue(binding,
                                                  argName.c_str(),
                                                  value);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_SetPortDefaultValue *createAndExecuteDFGCommand_SetPortDefaultValue(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_SetPortDefaultValue *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string portPath;
    if (!DecodeName(args, ai, portPath))
      return cmd;

    std::string typeName;
    if (!DecodeName(args, ai, typeName))
      return cmd;
  
    std::string valueJSON;
    if (!DecodeString(args, ai, valueJSON))
      return cmd;
    FabricCore::DFGHost host    = binding.getHost();
    FabricCore::Context context = host.getContext();
    FabricCore::RTVal value     = FabricCore::RTVal::Construct( context, typeName.c_str(), 0, NULL );
    value.setJSON(valueJSON.c_str());

    cmd = new FabricUI::DFG::DFGUICmd_SetPortDefaultValue(binding,
                                                          execPath.c_str(),
                                                          exec,
                                                          portPath.c_str(),
                                                          value);
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}

FabricUI::DFG::DFGUICmd_SetRefVarPath *createAndExecuteDFGCommand_SetRefVarPath(std::vector<std::string> &args)
{
  FabricUI::DFG::DFGUICmd_SetRefVarPath *cmd = NULL;
  {
    unsigned int ai = 0;

    FabricCore::DFGBinding binding;
    std::string execPath;
    FabricCore::DFGExec exec;
    if (!DecodeExec(args, ai, binding, execPath, exec))
      return cmd;

    std::string refName;
    if (!DecodeName(args, ai, refName))
      return cmd;

    std::string varPath;
    if (!DecodeName(args, ai, varPath))
      return cmd;

    cmd = new FabricUI::DFG::DFGUICmd_SetRefVarPath(binding,
                                                    execPath.c_str(),
                                                    exec,
                                                    refName.c_str(),
                                                    varPath.c_str());
    try
    {
      cmd->doit();
    }
    catch(FabricCore::Exception e)
    {
      feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    }
  }

  return cmd;
}



/*---------------------------------------------------
  implementation of DFGUICmdHandler member functions.
*/



void DFGUICmdHandlerDCC::dfgDoRemoveNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodeNames
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_RemoveNodes::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(EncodeNames(nodeNames));

  execCmd(cmdName, args, std::string());
}

void DFGUICmdHandlerDCC::dfgDoConnect(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef srcPort, 
  FTL::CStrRef dstPort
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_Connect::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(srcPort);
  args.push_back(dstPort);

  execCmd(cmdName, args, std::string());
}

void DFGUICmdHandlerDCC::dfgDoDisconnect(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef srcPort, 
  FTL::CStrRef dstPort
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_Disconnect::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(srcPort);
  args.push_back(dstPort);

  execCmd(cmdName, args, std::string());
}

std::string DFGUICmdHandlerDCC::dfgDoAddGraph(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  QPointF pos
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_AddGraph::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(title);
  EncodePosition(pos, args);

  std::string result;
  execCmd(cmdName, args, result);
  return result;
}

std::string DFGUICmdHandlerDCC::dfgDoAddFunc(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  FTL::CStrRef initialCode,
  QPointF pos
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_AddFunc::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(title);
  args.push_back(initialCode);
  EncodePosition(pos, args);

  std::string result;
  execCmd(cmdName, args, result);
  return result;
}

std::string DFGUICmdHandlerDCC::dfgDoInstPreset(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef presetPath,
  QPointF pos
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_InstPreset::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(presetPath);
  EncodePosition(pos, args);

  std::string result;
  execCmd(cmdName, args, result);
  return result;
}

std::string DFGUICmdHandlerDCC::dfgDoAddVar(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredNodeName,
  FTL::CStrRef dataType,
  FTL::CStrRef extDep,
  QPointF pos
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_AddVar::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(desiredNodeName);
  args.push_back(dataType);
  args.push_back(extDep);
  EncodePosition(pos, args);

  std::string result;
  execCmd(cmdName, args, result);
  return result;
}

std::string DFGUICmdHandlerDCC::dfgDoAddGet(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredNodeName,
  FTL::CStrRef varPath,
  QPointF pos
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_AddGet::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(desiredNodeName);
  args.push_back(varPath);
  EncodePosition(pos, args);

  std::string result;
  execCmd(cmdName, args, result);
  return result;
}

std::string DFGUICmdHandlerDCC::dfgDoAddSet(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredNodeName,
  FTL::CStrRef varPath,
  QPointF pos
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_AddSet::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(desiredNodeName);
  args.push_back(varPath);
  EncodePosition(pos, args);

  std::string result;
  execCmd(cmdName, args, result);
  return result;
}

std::string DFGUICmdHandlerDCC::dfgDoAddPort(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredPortName,
  FabricCore::DFGPortType portType,
  FTL::CStrRef typeSpec,
  FTL::CStrRef portToConnect
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_AddPort::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(desiredPortName);
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
  args.push_back(portTypeStr);
  args.push_back(typeSpec);
  args.push_back(portToConnect);

  std::string result;
  execCmd(cmdName, args, result);
  return result;
}

void DFGUICmdHandlerDCC::dfgDoRemovePort(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef portName
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_RemovePort::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(portName);

  execCmd(cmdName, args, std::string());
}

void DFGUICmdHandlerDCC::dfgDoMoveNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodeNames,
  FTL::ArrayRef<QPointF> poss
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_MoveNodes::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(EncodeNames(nodeNames));
  args.push_back(EncodeXPoss(poss));
  args.push_back(EncodeYPoss(poss));

  execCmd(cmdName, args, std::string());
}

void DFGUICmdHandlerDCC::dfgDoResizeBackDrop(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef backDropName,
  QPointF pos,
  QSizeF size
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_ResizeBackDrop::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(backDropName);
  EncodePosition(pos, args);
  EncodeSize(size, args);

  execCmd(cmdName, args, std::string());
}

std::string DFGUICmdHandlerDCC::dfgDoImplodeNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodeNames,
  FTL::CStrRef desiredNodeName
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_ImplodeNodes::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(EncodeNames(nodeNames));
  args.push_back(desiredNodeName);

  std::string result;
  execCmd(cmdName, args, result);
  return result;
}

std::vector<std::string> DFGUICmdHandlerDCC::dfgDoExplodeNode(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef nodeName
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_ExplodeNode::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(nodeName);

  std::string resultValue;
  execCmd(cmdName, args, resultValue);
  FTL::StrRef explodedNodeNamesStr = resultValue;
  std::vector<std::string> result;
  while (!explodedNodeNamesStr.empty())
  {
    FTL::StrRef::Split split = explodedNodeNamesStr.split('|');
    result.push_back(split.first);
    explodedNodeNamesStr = split.second;
  }
  return result;
}

void DFGUICmdHandlerDCC::dfgDoAddBackDrop(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  QPointF pos
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_AddBackDrop::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(title);
  EncodePosition(pos, args);

  execCmd(cmdName, args, std::string());
}

void DFGUICmdHandlerDCC::dfgDoSetNodeTitle(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef nodeName,
  FTL::CStrRef title
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_SetNodeTitle::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(nodeName);
  args.push_back(title);

  execCmd(cmdName, args, std::string());
}

void DFGUICmdHandlerDCC::dfgDoSetNodeComment(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef nodeName,
  FTL::CStrRef comment
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_SetNodeComment::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(nodeName);
  args.push_back(comment);

  execCmd(cmdName, args, std::string());
}

void DFGUICmdHandlerDCC::dfgDoSetCode(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef code
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_SetCode::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(code);

  execCmd(cmdName, args, std::string());
}

std::string DFGUICmdHandlerDCC::dfgDoRenamePort(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef oldPortName,
  FTL::CStrRef desiredNewPortName
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_RenamePort::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(oldPortName);
  args.push_back(desiredNewPortName);

  std::string result;
  execCmd(cmdName, args, result);
  return result;
}

std::vector<std::string> DFGUICmdHandlerDCC::dfgDoPaste(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef text,
  QPointF cursorPos
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_Paste::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(text);
  EncodePosition(cursorPos, args);

  std::string resultValue;
  execCmd(cmdName, args, resultValue);
  FTL::StrRef pastedNodeNamesStr = resultValue;
  std::vector<std::string> result;
  while (!pastedNodeNamesStr.empty())
  {
    FTL::StrRef::Split split = pastedNodeNamesStr.split('|');
    result.push_back(split.first);
    pastedNodeNamesStr = split.second;
  }
  return result;
}

void DFGUICmdHandlerDCC::dfgDoSetArgType(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef argName,
  FTL::CStrRef typeName
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_SetArgType::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(argName);
  args.push_back(typeName);

  execCmd(cmdName, args, std::string());
}

void DFGUICmdHandlerDCC::dfgDoSetArgValue(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef argName,
  FabricCore::RTVal const &value
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_SetArgValue::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(argName);
  args.push_back(value.getTypeNameCStr());
  args.push_back(value.getJSON().getStringCString());

  execCmd(cmdName, args, std::string());
}

void DFGUICmdHandlerDCC::dfgDoSetPortDefaultValue(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef portPath,
  FabricCore::RTVal const &value
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_SetPortDefaultValue::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(portPath);
  args.push_back(value.getTypeNameCStr());
  args.push_back(value.getJSON().getStringCString());

  execCmd(cmdName, args, std::string());
}

void DFGUICmdHandlerDCC::dfgDoSetRefVarPath(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef refName,
  FTL::CStrRef varPath
  )
{
  std::string cmdName(FabricUI::DFG::DFGUICmd_SetRefVarPath::CmdName());
  std::vector<std::string> args;

  args.push_back(getOperatorNameFromBinding(binding));
  args.push_back(execPath);
  args.push_back(refName);
  args.push_back(varPath);

  execCmd(cmdName, args, std::string());
}

std::string DFGUICmdHandlerDCC::getOperatorNameFromBinding(FabricCore::DFGBinding const &binding)
{
  // try to get the operator's name for this binding.
  if (m_parentBaseInterface && m_parentBaseInterface->getBinding() == binding)
  {
    unsigned int opObjID = _opUserData::GetOperatorObjectID(m_parentBaseInterface);
    if (opObjID != UINT_MAX)
    {
      XSI::CRef opRef = XSI::Application().GetObjectFromID(opObjID).GetRef();
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

FabricCore::DFGBinding DFGUICmdHandlerDCC::getBindingFromOperatorName(std::string operatorName)
{
  // try to get the binding from the operator's name.
  XSI::CRef opRef;
  opRef.Set(XSI::CString(operatorName.c_str()));
  XSI::CustomOperator op(opRef);
  BaseInterface *baseInterface = _opUserData::GetBaseInterface(op.GetObjectID());
  if (baseInterface)
    return baseInterface->getBinding();

  // not found.
  return FabricCore::DFGBinding();
}



/*-------------------------------
  implementation of XSI commands.
*/



void CValueArrayToStdVector(XSI::CValueArray &inArgs, std::vector<std::string> &outArgs)
{
  outArgs.clear();
  for (int i=0;i<inArgs.GetCount();i++)
  {
    outArgs.push_back(XSI::CString(inArgs[i]).GetAsciiString());
    if (inArgs[i].m_t != XSI::CValue::siString)
      XSI::Application().LogMessage(L"[DFGUICmd] argsToStdArgs(): inArgs[i] is not of type CValue::siString.", XSI::siWarningMsg);
  }
}

void DFGUICmd_Finish(XSI::Context &ctxt, FabricUI::DFG::DFGUICmd *cmd)
{
  // store or delete the DFG command, depending on XSI's current undo preferences.
  if ((bool)ctxt.GetAttribute(L"UndoRequired") == true)
  {
    if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] Finish, store cmd in UndoRedoData", XSI::siCommentMsg);
    ctxt.PutAttribute(L"UndoRedoData", (XSI::CValue::siPtrType)cmd);
  }
  else if (cmd)
  {
    if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] Finish, delete cmd", XSI::siCommentMsg);
    delete cmd;
  }
}

void DFGUICmd_Undo(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] Undo", XSI::siCommentMsg);
  FabricUI::DFG::DFGUICmd *cmd = (FabricUI::DFG::DFGUICmd *)(XSI::CValue::siPtrType)ctxt.GetAttribute(L"UndoRedoData");
  if (cmd)
  {
    if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] cmd->undo() \"" + XSI::CString(cmd->getDesc().c_str()) + L"\"", XSI::siCommentMsg);
    cmd->undo();
  }
}

void DFGUICmd_Redo(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] Redo", XSI::siCommentMsg);
  FabricUI::DFG::DFGUICmd *cmd = (FabricUI::DFG::DFGUICmd *)(XSI::CValue::siPtrType)ctxt.GetAttribute(L"UndoRedoData");
  if (cmd)
  {
    if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] cmd->redo() \"" + XSI::CString(cmd->getDesc().c_str()) + L"\"", XSI::siCommentMsg);
    cmd->redo();
  }
}

void DFGUICmd_TermUndoRedo(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] TermUndoRedo", XSI::siCommentMsg);
  FabricUI::DFG::DFGUICmd *cmd = (FabricUI::DFG::DFGUICmd *)(XSI::CValue::siPtrType)ctxt.GetAttribute(L"UndoRedoData");
  if (cmd)
  {
    if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] delete cmd", XSI::siCommentMsg);
    delete cmd;
  }
}

//        "dfgRemoveNodes"

SICALLBACK dfgRemoveNodes_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",   XSI::CString());
  oArgs.Add(L"execPath",  XSI::CString());
  oArgs.Add(L"nodeNames", XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgRemoveNodes_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgRemoveNodes_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_RemoveNodes *cmd = createAndExecuteDFGCommand_RemoveNodes(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgRemoveNodes_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgRemoveNodes_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgRemoveNodes_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgConnect"

SICALLBACK dfgConnect_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     XSI::CString());
  oArgs.Add(L"execPath",    XSI::CString());
  oArgs.Add(L"srcPortPath", XSI::CString());
  oArgs.Add(L"dstPortPath", XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgConnect_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgConnect_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_Connect *cmd = createAndExecuteDFGCommand_Connect(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgConnect_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgConnect_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgConnect_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgDisconnect"

SICALLBACK dfgDisconnect_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     XSI::CString());
  oArgs.Add(L"execPath",    XSI::CString());
  oArgs.Add(L"srcPortPath", XSI::CString());
  oArgs.Add(L"dstPortPath", XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgDisconnect_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgDisconnect_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_Disconnect *cmd = createAndExecuteDFGCommand_Disconnect(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgDisconnect_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgDisconnect_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgDisconnect_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgAddGraph"

SICALLBACK dfgAddGraph_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",  XSI::CString());
  oArgs.Add(L"execPath", XSI::CString());
  oArgs.Add(L"title",    XSI::CString());
  oArgs.Add(L"xPos",     XSI::CString());
  oArgs.Add(L"yPos",     XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgAddGraph_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgAddGraph_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_AddGraph *cmd = createAndExecuteDFGCommand_AddGraph(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // store return value.
  XSI::CString returnValue = cmd->getActualNodeName().c_str();
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue + L"\"", XSI::siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddGraph_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddGraph_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddGraph_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgAddFunc"

SICALLBACK dfgAddFunc_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",     XSI::CString());
  oArgs.Add(L"execPath",    XSI::CString());
  oArgs.Add(L"title",       XSI::CString());
  oArgs.Add(L"initialCode", XSI::CString());
  oArgs.Add(L"xPos",        XSI::CString());
  oArgs.Add(L"yPos",        XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgAddFunc_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgAddFunc_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_AddFunc *cmd = createAndExecuteDFGCommand_AddFunc(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // store return value.
  XSI::CString returnValue = cmd->getActualNodeName().c_str();
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue + L"\"", XSI::siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddFunc_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddFunc_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddFunc_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgInstPreset"

SICALLBACK dfgInstPreset_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",    XSI::CString());
  oArgs.Add(L"execPath",   XSI::CString());
  oArgs.Add(L"presetPath", XSI::CString());
  oArgs.Add(L"xPos",       XSI::CString());
  oArgs.Add(L"yPos",       XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgInstPreset_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgInstPreset_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_InstPreset *cmd = createAndExecuteDFGCommand_InstPreset(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // store return value.
  XSI::CString returnValue = cmd->getActualNodeName().c_str();
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue + L"\"", XSI::siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgInstPreset_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgInstPreset_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgInstPreset_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgAddVar"

SICALLBACK dfgAddVar_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",         XSI::CString());
  oArgs.Add(L"execPath",        XSI::CString());
  oArgs.Add(L"desiredNodeName", XSI::CString());
  oArgs.Add(L"dataType",        XSI::CString());
  oArgs.Add(L"extDep",          XSI::CString());
  oArgs.Add(L"xPos",            XSI::CString());
  oArgs.Add(L"yPos",            XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgAddVar_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgAddVar_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_AddVar *cmd = createAndExecuteDFGCommand_AddVar(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // store return value.
  XSI::CString returnValue = cmd->getActualNodeName().c_str();
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue + L"\"", XSI::siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddVar_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddVar_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddVar_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgAddGet"

SICALLBACK dfgAddGet_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",         XSI::CString());
  oArgs.Add(L"execPath",        XSI::CString());
  oArgs.Add(L"desiredNodeName", XSI::CString());
  oArgs.Add(L"varPath",         XSI::CString());
  oArgs.Add(L"xPos",            XSI::CString());
  oArgs.Add(L"yPos",            XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgAddGet_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgAddGet_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_AddGet *cmd = createAndExecuteDFGCommand_AddGet(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // store return value.
  XSI::CString returnValue = cmd->getActualNodeName().c_str();
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue + L"\"", XSI::siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddGet_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddGet_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddGet_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgAddSet"

SICALLBACK dfgAddSet_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",         XSI::CString());
  oArgs.Add(L"execPath",        XSI::CString());
  oArgs.Add(L"desiredNodeName", XSI::CString());
  oArgs.Add(L"varPath",         XSI::CString());
  oArgs.Add(L"xPos",            XSI::CString());
  oArgs.Add(L"yPos",            XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgAddSet_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgAddSet_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_AddSet *cmd = createAndExecuteDFGCommand_AddSet(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // store return value.
  XSI::CString returnValue = cmd->getActualNodeName().c_str();
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue + L"\"", XSI::siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddSet_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddSet_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddSet_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgAddPort"

SICALLBACK dfgAddPort_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",         XSI::CString());
  oArgs.Add(L"execPath",        XSI::CString());
  oArgs.Add(L"desiredPortName", XSI::CString());
  oArgs.Add(L"portType",        XSI::CString());
  oArgs.Add(L"typeSpec",        XSI::CString());
  oArgs.Add(L"portToConnect",   XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgAddPort_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgAddPort_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_AddPort *cmd = createAndExecuteDFGCommand_AddPort(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // store return value.
  XSI::CString returnValue = cmd->getActualPortName().c_str();
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue + L"\"", XSI::siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddPort_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddPort_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddPort_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgRemovePort"

SICALLBACK dfgRemovePort_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",  XSI::CString());
  oArgs.Add(L"execPath", XSI::CString());
  oArgs.Add(L"portName", XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgRemovePort_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgRemovePort_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_RemovePort *cmd = createAndExecuteDFGCommand_RemovePort(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgRemovePort_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgRemovePort_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgRemovePort_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgMoveNodes"

SICALLBACK dfgMoveNodes_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",   XSI::CString());
  oArgs.Add(L"execPath",  XSI::CString());
  oArgs.Add(L"nodeNames", XSI::CString());
  oArgs.Add(L"xPoss",     XSI::CString());
  oArgs.Add(L"yPoss",     XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgMoveNodes_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgMoveNodes_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_MoveNodes *cmd = createAndExecuteDFGCommand_MoveNodes(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgMoveNodes_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgMoveNodes_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgMoveNodes_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgResizeBackDrop"

SICALLBACK dfgResizeBackDrop_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",      XSI::CString());
  oArgs.Add(L"execPath",     XSI::CString());
  oArgs.Add(L"backDropName", XSI::CString());
  oArgs.Add(L"xPos",         XSI::CString());
  oArgs.Add(L"yPos",         XSI::CString());
  oArgs.Add(L"width",        XSI::CString());
  oArgs.Add(L"height",       XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgResizeBackDrop_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgResizeBackDrop_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_ResizeBackDrop *cmd = createAndExecuteDFGCommand_ResizeBackDrop(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgResizeBackDrop_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgResizeBackDrop_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgResizeBackDrop_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgImplodeNodes"

SICALLBACK dfgImplodeNodes_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",                 XSI::CString());
  oArgs.Add(L"execPath",                XSI::CString());
  oArgs.Add(L"nodeNames",               XSI::CString());
  oArgs.Add(L"desiredImplodedNodeName", XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgImplodeNodes_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgImplodeNodes_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_ImplodeNodes *cmd = createAndExecuteDFGCommand_ImplodeNodes(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // store return value.
  XSI::CString returnValue = cmd->getActualImplodedNodeName().c_str();
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue + L"\"", XSI::siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgImplodeNodes_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgImplodeNodes_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgImplodeNodes_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgExplodeNode"

SICALLBACK dfgExplodeNode_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",  XSI::CString());
  oArgs.Add(L"execPath", XSI::CString());
  oArgs.Add(L"nodeName", XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgExplodeNode_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgExplodeNode_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_ExplodeNode *cmd = createAndExecuteDFGCommand_ExplodeNode(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // store return value.
  FTL::ArrayRef<std::string> explodedNodeNames = cmd->getExplodedNodeNames();
  std::string explodedNodeNamesString;
  for ( FTL::ArrayRef<std::string>::IT it = explodedNodeNames.begin();
    it != explodedNodeNames.end(); ++it )
  {
    if ( it != explodedNodeNames.begin() )
      explodedNodeNamesString += '|';
    explodedNodeNamesString += *it;
  }

  XSI::CString returnValue = explodedNodeNamesString.c_str();
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue + L"\"", XSI::siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgExplodeNode_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgExplodeNode_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgExplodeNode_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgAddBackDrop"

SICALLBACK dfgAddBackDrop_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",  XSI::CString());
  oArgs.Add(L"execPath", XSI::CString());
  oArgs.Add(L"title",    XSI::CString());
  oArgs.Add(L"xPos",     XSI::CString());
  oArgs.Add(L"yPos",     XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgAddBackDrop_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgAddBackDrop_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_AddBackDrop *cmd = createAndExecuteDFGCommand_AddBackDrop(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // store return value.
  XSI::CString returnValue = cmd->getActualNodeName().c_str();
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue + L"\"", XSI::siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddBackDrop_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddBackDrop_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgAddBackDrop_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgSetNodeTitle"

SICALLBACK dfgSetNodeTitle_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",  XSI::CString());
  oArgs.Add(L"execPath", XSI::CString());
  oArgs.Add(L"nodeName", XSI::CString());
  oArgs.Add(L"title",    XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgSetNodeTitle_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgSetNodeTitle_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_SetNodeTitle *cmd = createAndExecuteDFGCommand_SetNodeTitle(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetNodeTitle_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetNodeTitle_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetNodeTitle_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgSetNodeComment"

SICALLBACK dfgSetNodeComment_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",  XSI::CString());
  oArgs.Add(L"execPath", XSI::CString());
  oArgs.Add(L"nodeName", XSI::CString());
  oArgs.Add(L"comment",  XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgSetNodeComment_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgSetNodeComment_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_SetNodeComment *cmd = createAndExecuteDFGCommand_SetNodeComment(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetNodeComment_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetNodeComment_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetNodeComment_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgSetCode"

SICALLBACK dfgSetCode_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",  XSI::CString());
  oArgs.Add(L"execPath", XSI::CString());
  oArgs.Add(L"code",     XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgSetCode_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgSetCode_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_SetCode *cmd = createAndExecuteDFGCommand_SetCode(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetCode_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetCode_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetCode_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgRenamePort"

SICALLBACK dfgRenamePort_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",            XSI::CString());
  oArgs.Add(L"execPath",           XSI::CString());
  oArgs.Add(L"oldPortName",        XSI::CString());
  oArgs.Add(L"desiredNewPortName", XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgRenamePort_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgRenamePort_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_RenamePort *cmd = createAndExecuteDFGCommand_RenamePort(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // store return value.
  XSI::CString returnValue = cmd->getActualNewPortName().c_str();
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue + L"\"", XSI::siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgRenamePort_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgRenamePort_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgRenamePort_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgPaste"

SICALLBACK dfgPaste_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(true);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",  XSI::CString());
  oArgs.Add(L"execPath", XSI::CString());
  oArgs.Add(L"text",     XSI::CString());
  oArgs.Add(L"xPos",     XSI::CString());
  oArgs.Add(L"yPos",     XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgPaste_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgPaste_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_Paste *cmd = createAndExecuteDFGCommand_Paste(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // store return value.
  FTL::ArrayRef<std::string> pastedNodeNames =
    cmd->getPastedNodeNames();
  std::string pastedNodeNamesString;
  for ( FTL::ArrayRef<std::string>::IT it = pastedNodeNames.begin();
    it != pastedNodeNames.end(); ++it )
  {
    if ( it != pastedNodeNames.begin() )
      pastedNodeNamesString += '|';
    pastedNodeNamesString += *it;
  }

  XSI::CString returnValue = pastedNodeNamesString.c_str();
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] storing return value \"" + returnValue + L"\"", XSI::siCommentMsg);
  ctxt.PutAttribute(L"ReturnValue", returnValue);

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgPaste_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgPaste_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgPaste_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgSetArgType"

SICALLBACK dfgSetArgType_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",  XSI::CString());
  oArgs.Add(L"argName",  XSI::CString());
  oArgs.Add(L"typeName", XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgSetArgType_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgSetArgType_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_SetArgType *cmd = createAndExecuteDFGCommand_SetArgType(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetArgType_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetArgType_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetArgType_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgSetArgValue"

SICALLBACK dfgSetArgValue_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",   XSI::CString());
  oArgs.Add(L"argName",   XSI::CString());
  oArgs.Add(L"typeName",  XSI::CString());
  oArgs.Add(L"valueJSON", XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgSetArgValue_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgSetArgValue_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_SetArgValue *cmd = createAndExecuteDFGCommand_SetArgValue(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetArgValue_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetArgValue_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetArgValue_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgSetPortDefaultValue"

SICALLBACK dfgSetPortDefaultValue_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",   XSI::CString());
  oArgs.Add(L"execPath",  XSI::CString());
  oArgs.Add(L"portPath",  XSI::CString());
  oArgs.Add(L"typeName",  XSI::CString());
  oArgs.Add(L"valueJSON", XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgSetPortDefaultValue_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgSetPortDefaultValue_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_SetPortDefaultValue *cmd = createAndExecuteDFGCommand_SetPortDefaultValue(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetPortDefaultValue_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetPortDefaultValue_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetPortDefaultValue_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

//        "dfgSetRefVarPath"

SICALLBACK dfgSetRefVarPath_Init(XSI::CRef &in_ctxt)
{
  XSI::Context ctxt(in_ctxt);
  XSI::Command oCmd;

  oCmd = ctxt.GetSource();
  oCmd.EnableReturnValue(false);

  XSI::ArgumentArray oArgs = oCmd.GetArguments();
  oArgs.Add(L"binding",  XSI::CString());
  oArgs.Add(L"execPath", XSI::CString());
  oArgs.Add(L"refName",  XSI::CString());
  oArgs.Add(L"varPath",  XSI::CString());

  return XSI::CStatus::OK;
}

SICALLBACK dfgSetRefVarPath_Execute(XSI::CRef &in_ctxt)
{
  if (DFGUICmdHandlerLOG) XSI::Application().LogMessage(L"[DFGUICmd] calling dfgSetRefVarPath_Execute()", XSI::siCommentMsg);

  // init.
  XSI::Context     ctxt(in_ctxt);
  XSI::CValueArray tmp = ctxt.GetAttribute(L"Arguments");
  std::vector<std::string> args;
  CValueArrayToStdVector(tmp, args);

  // create and execute the DFG command.
  FabricUI::DFG::DFGUICmd_SetRefVarPath *cmd = createAndExecuteDFGCommand_SetRefVarPath(args);
  if (!cmd)
    return XSI::CStatus::Fail;

  // done.
  DFGUICmd_Finish(ctxt, cmd);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetRefVarPath_Undo(XSI::CRef &ctxt)
{
  DFGUICmd_Undo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetRefVarPath_Redo(XSI::CRef &ctxt)
{
  DFGUICmd_Redo(ctxt);
  return XSI::CStatus::OK;
}

SICALLBACK dfgSetRefVarPath_TermUndoRedo(XSI::CRef &ctxt)
{
  DFGUICmd_TermUndoRedo(ctxt);
  return XSI::CStatus::OK;
}

