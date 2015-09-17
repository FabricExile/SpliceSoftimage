#include "FabricDFGUICmdHandlerDCC.h"
#include "FabricDFGBaseInterface.h"
#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"

#include "FabricSplicePlugin.h"

#ifdef USE_FABRICSPLICE__CLIENT
  #include "FabricSplice.h"
#endif

#include <algorithm>
#include <sstream>

FabricCore::Client                        BaseInterface::s_client;
FabricCore::DFGHost                       BaseInterface::s_host = NULL;
FabricServices::ASTWrapper::KLASTManager *BaseInterface::s_manager = NULL;
unsigned int                              BaseInterface::s_maxId = 0;
void (*BaseInterface::s_logFunc)(void *, const char *, unsigned int) = NULL;
void (*BaseInterface::s_logErrorFunc)(void *, const char *, unsigned int) = NULL;
std::map <unsigned int, BaseInterface*>   BaseInterface::s_instances;

BaseInterface::BaseInterface(void (*in_logFunc)     (void *, const char *, unsigned int),
                             void (*in_logErrorFunc)(void *, const char *, unsigned int))
{
  // init splice.
  xsiInitializeSplice();

  // set log functions.
  // (note: this is probably not necessary since the logging is done by the FabricSplice client).
  if (in_logFunc)       setLogFunc(in_logFunc);
  if (in_logErrorFunc)  setLogErrorFunc(in_logErrorFunc);

  //
  m_id = s_maxId++;
  std::string m;
  std::stringstream ssId;
  ssId << m_id;
  m  = "calling BaseInterface(), m_id = " + ssId.str();
  logFunc(NULL, m.c_str(), m.length());

  // construct the client
  if (s_instances.size() == 0)
  {
    try
    {
      // create a client
      #ifdef USE_FABRICSPLICE__CLIENT
      {
        const int guarded = 1;
        s_client = FabricSplice::ConstructClient(guarded);
      }
      #else
      {
        FabricCore::Client::CreateOptions options;
        memset(&options, 0, sizeof(options));
        options.guarded = 1;
        options.optimizationType = FabricCore::ClientOptimizationType_Background;
        s_client = FabricCore::Client(&logFunc, NULL, &options);
      }
      #endif

      // load basic extensions
      s_client.loadExtension("Math",     "", false);
      s_client.loadExtension("Geometry", "", false);
      s_client.loadExtension("FileIO",   "", false);

      // create a host for Canvas
      s_host = s_client.getDFGHost();

      // create KL AST manager
      s_manager = new FabricServices::ASTWrapper::KLASTManager(&s_client);
      s_manager->loadAllExtensionsFromExtsPath();
    }
    catch (FabricCore::Exception e)
    {
      logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    }
  }

  // insert in map.
  s_instances.insert(std::pair<unsigned int, BaseInterface*>(m_id, this));

  //
  try
  {
    // create an empty binding
    m_binding = s_host.createBindingToNewGraph();
    m_binding.setNotificationCallback(bindingNotificationCallback, this);

    // command handler.
    m_cmdHandler = new DFGUICmdHandlerDCC(this);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

BaseInterface::~BaseInterface()
{
  std::string m;
  std::stringstream ssId;
  ssId << m_id;
  m  = "calling ~BaseInterface(), m_id = " + ssId.str();
  logFunc(NULL, m.c_str(), m.length());

  std::map<unsigned int, BaseInterface*>::iterator it = s_instances.find(m_id);

  // Release cached values and variables, for example InlineDrawingHandle
  if( m_binding )
    m_binding.deallocValues();

  m_binding = FabricCore::DFGBinding();

  delete m_cmdHandler;

  if (it != s_instances.end())
  {
    s_instances.erase(it);
    if (s_instances.size() == 0)
    {
      try
      {
        printf("Destructing client...\n");
        delete(s_manager);
        s_host = FabricCore::DFGHost();
        #ifdef USE_FABRICSPLICE__CLIENT
        {
          FabricSplice::DestroyClient();
          s_client.invalidate();
        }
        #else
        {
          s_client = FabricCore::Client();
        }
        #endif
      }
      catch (FabricCore::Exception e)
      {
        logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
      }
    }
  }
}

unsigned int BaseInterface::getId()
{
  return m_id;
}

BaseInterface *BaseInterface::getFromId(unsigned int id)
{
  std::map<unsigned int, BaseInterface*>::iterator it = s_instances.find(id);
  if (it == s_instances.end())
    return NULL;
  return it->second;
}

FabricCore::Client *BaseInterface::getClient()
{
  return &s_client;
}

FabricCore::DFGHost BaseInterface::getHost()
{
  return s_host;
}

FabricCore::DFGBinding BaseInterface::getBinding()
{
  return m_binding;
}

FabricServices::ASTWrapper::KLASTManager *BaseInterface::getManager()
{
  return s_manager;
}

DFGUICmdHandlerDCC *BaseInterface::getCmdHandler()
{
  return m_cmdHandler;
}

std::string BaseInterface::getJSON()
{
  try
  {
    return m_binding.exportJSON().getCString();
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return "";
  }
}

void BaseInterface::setFromJSON(const std::string &json)
{
  try
  {
    m_binding = s_host.createBindingFromJSON(json.c_str());
    m_binding.setNotificationCallback(bindingNotificationCallback, this);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::setLogFunc(void (*in_logFunc)(void *, const char *, unsigned int))
{
  s_logFunc = in_logFunc;
}

void BaseInterface::setLogErrorFunc(void (*in_logErrorFunc)(void *, const char *, unsigned int))
{
  s_logErrorFunc = in_logErrorFunc;
}

void BaseInterface::bindingNotificationCallback(void *userData, char const *jsonCString, uint32_t jsonLength)
{
  // check pointers.
  if (!userData || !jsonCString)
    return;

  //
  // NOTHING HERE.
  //
  // we ignore the notifications, because Canvas in Softimage is (pseudo) modal dialog.
  //
}

void BaseInterface::logFunc(void *userData, const char *message, unsigned int length)
{
  if (s_logFunc)
  {
    if (message)
      s_logFunc(userData, message, length);
  }
  else
  {
    printf("BaseInterface: %s\n", message ? message : "");
  }
}

void BaseInterface::logErrorFunc(void *userData, const char *message, unsigned int length)
{
  if (s_logErrorFunc)
  {
    if (message)
      s_logErrorFunc(userData, message, length);
  }
  else
  {
    printf("BaseInterface: error: %s\n", message ? message : "");
  }
}

bool BaseInterface::HasPort(const char *in_portName, const bool testForInput)
{
  try
  {
    // check/init.
    if (!in_portName || in_portName[0] == '\0')  return false;
    const FabricCore::DFGPortType portType = (testForInput ? FabricCore::DFGPortType_In : FabricCore::DFGPortType_Out);

    // get the graph.
    FabricCore::DFGExec graph = m_binding.getExec();
    if (!graph.isValid())
    {
      std::string s = "BaseInterface::HasPort(): failed to get graph!";
      logErrorFunc(NULL, s.c_str(), s.length());
      return false;
    }

    // return result.
    return (graph.haveExecPort(in_portName) && graph.getExecPortType(in_portName) == portType);
  }
  catch (FabricCore::Exception e)
  {
    //std::string s = std::string("BaseInterface::HasPort(): ") + (e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
    //logErrorFunc(NULL, s.c_str(), s.length());
    return false;
  }
}

bool BaseInterface::HasInputPort(const char *portName)
{
  return HasPort(portName, true);
}

bool BaseInterface::HasInputPort(const std::string &portName)
{
  return HasPort(portName.c_str(), true);
}

bool BaseInterface::HasOutputPort(const char *portName)
{
  return HasPort(portName, false);
}

bool BaseInterface::HasOutputPort(const std::string &portName)
{
  return HasPort(portName.c_str(), false);
}

int BaseInterface::GetArgValueBoolean(FabricCore::DFGBinding &binding, char const * argName, bool &out, bool strict)
{
  // init output.
  out = false;

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)    return -1;

    else if (resolvedType == "Boolean")     out = rtval.getBoolean();

    else if (!strict)
    {
      if      (resolvedType == "Scalar")    out = (0 != rtval.getFloat32());
      else if (resolvedType == "Float32")   out = (0 != rtval.getFloat32());
      else if (resolvedType == "Float64")   out = (0 != rtval.getFloat64());

      else if (resolvedType == "Integer")   out = (0 != rtval.getSInt32());
      else if (resolvedType == "SInt8")     out = (0 != rtval.getSInt8());
      else if (resolvedType == "SInt16")    out = (0 != rtval.getSInt16());
      else if (resolvedType == "SInt32")    out = (0 != rtval.getSInt32());
      else if (resolvedType == "SInt64")    out = (0 != rtval.getSInt64());

      else if (resolvedType == "Byte")      out = (0 != rtval.getUInt8());
      else if (resolvedType == "UInt8")     out = (0 != rtval.getUInt8());
      else if (resolvedType == "UInt16")    out = (0 != rtval.getUInt16());
      else if (resolvedType == "Count")     out = (0 != rtval.getUInt32());
      else if (resolvedType == "Index")     out = (0 != rtval.getUInt32());
      else if (resolvedType == "Size")      out = (0 != rtval.getUInt32());
      else if (resolvedType == "UInt32")    out = (0 != rtval.getUInt32());
      else if (resolvedType == "DataSize")  out = (0 != rtval.getUInt64());
      else if (resolvedType == "UInt64")    out = (0 != rtval.getUInt64());

      else return -1;
    }
    else return -1;
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return -4;
  }

  // done.
  return 0;
}

int BaseInterface::GetArgValueInteger(FabricCore::DFGBinding &binding, char const * argName, int &out, bool strict)
{
  // init output.
  out = 0;

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)    return -1;

    else if (resolvedType == "Integer")     out = (int)rtval.getSInt32();
    else if (resolvedType == "SInt8")       out = (int)rtval.getSInt8();
    else if (resolvedType == "SInt16")      out = (int)rtval.getSInt16();
    else if (resolvedType == "SInt32")      out = (int)rtval.getSInt32();
    else if (resolvedType == "SInt64")      out = (int)rtval.getSInt64();

    else if (resolvedType == "Byte")        out = (int)rtval.getUInt8();
    else if (resolvedType == "UInt8")       out = (int)rtval.getUInt8();
    else if (resolvedType == "UInt16")      out = (int)rtval.getUInt16();
    else if (resolvedType == "Count")       out = (int)rtval.getUInt32();
    else if (resolvedType == "Index")       out = (int)rtval.getUInt32();
    else if (resolvedType == "Size")        out = (int)rtval.getUInt32();
    else if (resolvedType == "UInt32")      out = (int)rtval.getUInt32();
    else if (resolvedType == "DataSize")    out = (int)rtval.getUInt64();
    else if (resolvedType == "UInt64")      out = (int)rtval.getUInt64();

    else if (!strict)
    {
      if      (resolvedType == "Boolean")   out = (int)rtval.getBoolean();

      else if (resolvedType == "Scalar")    out = (int)rtval.getFloat32();
      else if (resolvedType == "Float32")   out = (int)rtval.getFloat32();
      else if (resolvedType == "Float64")   out = (int)rtval.getFloat64();

      else return -1;
    }
    else return -1;
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return -4;
  }

  // done.
  return 0;
}

int BaseInterface::GetArgValueFloat(FabricCore::DFGBinding &binding, char const * argName, double &out, bool strict)
{
  // init output.
  out = 0;

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)    return -1;

    else if (resolvedType == "Scalar")      out = (double)rtval.getFloat32();
    else if (resolvedType == "Float32")     out = (double)rtval.getFloat32();
    else if (resolvedType == "Float64")     out = (double)rtval.getFloat64();

    else if (!strict)
    {
      if      (resolvedType == "Boolean")   out = (double)rtval.getBoolean();

      else if (resolvedType == "Integer")   out = (double)rtval.getSInt32();
      else if (resolvedType == "SInt8")     out = (double)rtval.getSInt8();
      else if (resolvedType == "SInt16")    out = (double)rtval.getSInt16();
      else if (resolvedType == "SInt32")    out = (double)rtval.getSInt32();
      else if (resolvedType == "SInt64")    out = (double)rtval.getSInt64();

      else if (resolvedType == "Byte")      out = (double)rtval.getUInt8();
      else if (resolvedType == "UInt8")     out = (double)rtval.getUInt8();
      else if (resolvedType == "UInt16")    out = (double)rtval.getUInt16();
      else if (resolvedType == "Count")     out = (double)rtval.getUInt32();
      else if (resolvedType == "Index")     out = (double)rtval.getUInt32();
      else if (resolvedType == "Size")      out = (double)rtval.getUInt32();
      else if (resolvedType == "UInt32")    out = (double)rtval.getUInt32();
      else if (resolvedType == "DataSize")  out = (double)rtval.getUInt64();
      else if (resolvedType == "UInt64")    out = (double)rtval.getUInt64();

      else return -1;
    }
    else return -1;
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return -4;
  }

  // done.
  return 0;
}

int BaseInterface::GetArgValueString(FabricCore::DFGBinding &binding, char const * argName, std::string &out, bool strict)
{
  // init output.
  out = "";

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)    return -1;

    else if (resolvedType == "String")      out = rtval.getStringCString();

    else if (!strict)
    {
      char    s[64];
      bool    b;
      int     i;
      double  f;

      if (GetArgValueBoolean(binding, argName, b, true) == 0)
      {
        out = (b ? "true" : "false");
        return 0;
      }

      if (GetArgValueInteger(binding, argName, i, true) == 0)
      {
        #ifdef _WIN32
          sprintf_s(s, sizeof(s), "%ld", i);
        #else
          snprintf(s, sizeof(s), "%ld", i);
        #endif
        out = s;
        return 0;
      }

      if (GetArgValueFloat(binding, argName, f, true) == 0)
      {
        #ifdef _WIN32
          sprintf_s(s, sizeof(s), "%f", f);
        #else
          snprintf(s, sizeof(s), "%f", f);
        #endif
        out = s;
        return 0;
      }

      return -1;
    }
    else
      return -1;
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return -4;
  }

  // done.
  return 0;
}

int BaseInterface::GetArgValueVec2(FabricCore::DFGBinding &binding, char const * argName, std::vector <double> &out, bool strict)
{
  // init output.
  out.clear();

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)      return -1;

    else if (resolvedType == "Vec2")        {
                                              out.push_back(rtval.maybeGetMember("x").getFloat32());
                                              out.push_back(rtval.maybeGetMember("y").getFloat32());
                                            }
    else
      return -1;
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return -4;
  }

  // done.
  return 0;
}

int BaseInterface::GetArgValueVec3(FabricCore::DFGBinding &binding, char const * argName, std::vector <double> &out, bool strict)
{
  // init output.
  out.clear();

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)      return -1;

    else if (resolvedType == "Vec3")        {
                                              out.push_back(rtval.maybeGetMember("x").getFloat32());
                                              out.push_back(rtval.maybeGetMember("y").getFloat32());
                                              out.push_back(rtval.maybeGetMember("z").getFloat32());
                                            }
    else if (!strict)
    {
        if      (resolvedType == "Color")   {
                                              out.push_back(rtval.maybeGetMember("r").getFloat32());
                                              out.push_back(rtval.maybeGetMember("g").getFloat32());
                                              out.push_back(rtval.maybeGetMember("b").getFloat32());
                                            }
        else if (resolvedType == "Vec4")    {
                                              out.push_back(rtval.maybeGetMember("x").getFloat32());
                                              out.push_back(rtval.maybeGetMember("y").getFloat32());
                                              out.push_back(rtval.maybeGetMember("z").getFloat32());
                                            }
        else
          return -1;
    }
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return -4;
  }

  // done.
  return 0;
}

int BaseInterface::GetArgValueVec4(FabricCore::DFGBinding &binding, char const * argName, std::vector <double> &out, bool strict)
{
  // init output.
  out.clear();

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)      return -1;

    else if (resolvedType == "Vec4")        {
                                              out.push_back(rtval.maybeGetMember("x").getFloat32());
                                              out.push_back(rtval.maybeGetMember("y").getFloat32());
                                              out.push_back(rtval.maybeGetMember("z").getFloat32());
                                              out.push_back(rtval.maybeGetMember("t").getFloat32());
                                            }
    else if (!strict)
    {
        if      (resolvedType == "Color")   {
                                              out.push_back(rtval.maybeGetMember("r").getFloat32());
                                              out.push_back(rtval.maybeGetMember("g").getFloat32());
                                              out.push_back(rtval.maybeGetMember("b").getFloat32());
                                              out.push_back(rtval.maybeGetMember("a").getFloat32());
                                            }
        else
          return -1;
    }
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return -4;
  }

  // done.
  return 0;
}

int BaseInterface::GetArgValueColor(FabricCore::DFGBinding &binding, char const * argName, std::vector <double> &out, bool strict)
{
  // init output.
  out.clear();

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)      return -1;

    else if (resolvedType == "Color")      {
                                              out.push_back(rtval.maybeGetMember("r").getFloat32());
                                              out.push_back(rtval.maybeGetMember("g").getFloat32());
                                              out.push_back(rtval.maybeGetMember("b").getFloat32());
                                              out.push_back(rtval.maybeGetMember("a").getFloat32());
                                           }
    else if (!strict)
    {
        if      (resolvedType == "Vec4")   {
                                              out.push_back(rtval.maybeGetMember("x").getFloat32());
                                              out.push_back(rtval.maybeGetMember("y").getFloat32());
                                              out.push_back(rtval.maybeGetMember("z").getFloat32());
                                              out.push_back(rtval.maybeGetMember("t").getFloat32());
                                            }
        else if (resolvedType == "RGB")     {
                                              out.push_back(rtval.maybeGetMember("r").getUInt8() / 255.0);
                                              out.push_back(rtval.maybeGetMember("g").getUInt8() / 255.0);
                                              out.push_back(rtval.maybeGetMember("b").getUInt8() / 255.0);
                                              out.push_back(1);
                                            }
        else if (resolvedType == "RGBA")    {
                                              out.push_back(rtval.maybeGetMember("r").getUInt8() / 255.0);
                                              out.push_back(rtval.maybeGetMember("g").getUInt8() / 255.0);
                                              out.push_back(rtval.maybeGetMember("b").getUInt8() / 255.0);
                                              out.push_back(rtval.maybeGetMember("a").getUInt8() / 255.0);
                                            }
        else
          return -1;
    }
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return -4;
  }

  // done.
  return 0;
}

int BaseInterface::GetArgValueRGB(FabricCore::DFGBinding &binding, char const * argName, std::vector <double> &out, bool strict)
{
  // init output.
  out.clear();

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)    return -1;

    else if (resolvedType == "RGB")       {
                                            out.push_back(rtval.maybeGetMember("r").getUInt8() / 255.0);
                                            out.push_back(rtval.maybeGetMember("g").getUInt8() / 255.0);
                                            out.push_back(rtval.maybeGetMember("b").getUInt8() / 255.0);
                                          }
    else if (!strict)
    {
      if      (resolvedType == "RGBA")    {
                                            out.push_back(rtval.maybeGetMember("r").getUInt8() / 255.0);
                                            out.push_back(rtval.maybeGetMember("g").getUInt8() / 255.0);
                                            out.push_back(rtval.maybeGetMember("b").getUInt8() / 255.0);
                                          }
      else if (resolvedType == "Color")   {
                                            out.push_back(rtval.maybeGetMember("r").getFloat32());
                                            out.push_back(rtval.maybeGetMember("g").getFloat32());
                                            out.push_back(rtval.maybeGetMember("b").getFloat32());
                                          }
      else
        return -1;
    }
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return -4;
  }

  // done.
  return 0;
}

int BaseInterface::GetArgValueRGBA(FabricCore::DFGBinding &binding, char const * argName, std::vector <double> &out, bool strict)
{
  // init output.
  out.clear();

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

      std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
      FabricCore::RTVal rtval  = binding.getArgValue(argName);

      if      (resolvedType.length() == 0)      return -1;

      else if (resolvedType == "RGBA")        {
                                                out.push_back(rtval.maybeGetMember("r").getUInt8() / 255.0);
                                                out.push_back(rtval.maybeGetMember("g").getUInt8() / 255.0);
                                                out.push_back(rtval.maybeGetMember("b").getUInt8() / 255.0);
                                                out.push_back(rtval.maybeGetMember("a").getUInt8() / 255.0);
                                              }
      else if (!strict)
      {
          if      (resolvedType == "RGB")     {
                                                out.push_back(rtval.maybeGetMember("r").getUInt8() / 255.0);
                                                out.push_back(rtval.maybeGetMember("g").getUInt8() / 255.0);
                                                out.push_back(rtval.maybeGetMember("b").getUInt8() / 255.0);
                                                out.push_back(1);
                                              }
          else if (resolvedType == "Color")   {
                                                out.push_back(rtval.maybeGetMember("r").getFloat32());
                                                out.push_back(rtval.maybeGetMember("g").getFloat32());
                                                out.push_back(rtval.maybeGetMember("b").getFloat32());
                                                out.push_back(rtval.maybeGetMember("a").getFloat32());
                                              }
          else
            return -1;
      }
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return -4;
  }

  // done.
  return 0;
}

int BaseInterface::GetArgValueQuat(FabricCore::DFGBinding &binding, char const * argName, std::vector <double> &out, bool strict)
{
  // init output.
  out.clear();

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)      return -1;

    else if (resolvedType == "Quat")        {
                                              FabricCore::RTVal v;
                                              v = rtval.maybeGetMember("v");
                                              out.push_back(v.    maybeGetMember("x").getFloat32());
                                              out.push_back(v.    maybeGetMember("y").getFloat32());
                                              out.push_back(v.    maybeGetMember("z").getFloat32());
                                              out.push_back(rtval.maybeGetMember("w").getFloat32());
                                            }
    else
      return -1;
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return -4;
  }

  // done.
  return 0;
}

int BaseInterface::GetArgValueMat44(FabricCore::DFGBinding &binding, char const * argName, std::vector <double> &out, bool strict)
{
  // init output.
  out.clear();

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)      return -1;

    else if (resolvedType == "Mat44")       {
                                              char member[32];
                                              FabricCore::RTVal rtRow;
                                              for (int i = 0; i < 4; i++)
                                              {
                                                #ifdef _WIN32
                                                  sprintf_s(member, sizeof(member), "row%ld", i);
                                                #else
                                                  snprintf(member, sizeof(member), "row%ld", i);
                                                #endif
                                                rtRow = rtval.maybeGetMember(member);
                                                out.push_back(rtRow.maybeGetMember("x").getFloat32());
                                                out.push_back(rtRow.maybeGetMember("y").getFloat32());
                                                out.push_back(rtRow.maybeGetMember("z").getFloat32());
                                                out.push_back(rtRow.maybeGetMember("t").getFloat32());
                                              }
                                            }
    else
      return -1;
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return -4;
  }

  // done.
  return 0;
}

int BaseInterface::GetArgValuePolygonMesh(FabricCore::DFGBinding &binding,
                                           char const * argName,
                                           unsigned int                            &out_numVertices,
                                           unsigned int                            &out_numPolygons,
                                           unsigned int                            &out_numSamples,
                                           std::vector <float>                     *out_positions,
                                           std::vector <uint32_t>                  *out_polygonNumVertices,
                                           std::vector <uint32_t>                  *out_polygonVertices,
                                           std::vector <float>                     *out_polygonNodeNormals,
                                           std::vector <float>                     *out_polygonNodeUVWs,
                                           std::vector <float>                     *out_polygonNodeColors,
                                           bool                                     strict)
{
  // init output.
  out_numVertices = 0;
  out_numPolygons = 0;
  out_numSamples  = 0;
  if (out_positions)          out_positions          -> clear();
  if (out_polygonNumVertices) out_polygonNumVertices -> clear();
  if (out_polygonVertices)    out_polygonVertices    -> clear();
  if (out_polygonNodeNormals) out_polygonNodeNormals -> clear();
  if (out_polygonNodeUVWs)    out_polygonNodeUVWs    -> clear();
  if (out_polygonNodeColors)  out_polygonNodeColors  -> clear();

  // set out from port value.
  int errID = 0;
  try
  {
    // port doesn't exist?
    if(!binding.getExec().haveExecPort(argName))
      return -2;

    // check type.
    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    if (   resolvedType.length() == 0
        || resolvedType != "PolygonMesh")
      return -1;

    // RTVal of the polygon mesh.
    FabricCore::RTVal rtMesh = binding.getArgValue(argName);

    // get amount of points, polys, etc.
    out_numVertices = rtMesh.callMethod("UInt64", "pointCount",         0, 0).getUInt64();
    out_numPolygons = rtMesh.callMethod("UInt64", "polygonCount",       0, 0).getUInt64();
    out_numSamples  = rtMesh.callMethod("UInt64", "polygonPointsCount", 0, 0).getUInt64();
    if (   out_numVertices == UINT_MAX
        || out_numPolygons == UINT_MAX
        || out_numSamples  == UINT_MAX)
    {
      out_numVertices = 0;
      out_numPolygons = 0;
      out_numSamples  = 0;
      return errID;
    }

    // get the rest.
    do
    {
      // get vertex positions.
      if (   out_positions  != NULL
          && out_numVertices > 0  )
      {
        std::vector <float> &data = *out_positions;

        // resize output array(s).
            data.   resize(3 * out_numVertices);
        if (data.size() != 3 * out_numVertices)
        { errID = -3;
          break;  }

        // fill output array(s).
        std::vector <FabricCore::RTVal> args(2);
        args[0] = FabricCore::RTVal::ConstructExternalArray(*getClient(), "Float32", data.size(), (void *)data.data());
        args[1] = FabricCore::RTVal::ConstructUInt32(*getClient(), 3);
        rtMesh.callMethod("", "getPointsAsExternalArray", 2, &args[0]);
      }

      // get polygonal description.
      if (  (   out_polygonNumVertices != NULL
             || out_polygonVertices    != NULL)
          && out_numPolygons            > 0
          && out_numSamples             > 0  )
      {
        std::vector <uint32_t> tmpNum;
        std::vector <uint32_t> tmpIdx;
        std::vector <uint32_t> &dataNum = (out_polygonNumVertices ? *out_polygonNumVertices : tmpNum);
        std::vector <uint32_t> &dataIdx = (out_polygonVertices    ? *out_polygonVertices    : tmpIdx);

        // resize output array(s).
            dataNum.   resize(out_numPolygons);
        if (dataNum.size() != out_numPolygons)
        { errID = -3;
          break;  }
            dataIdx.   resize(out_numSamples);
        if (dataIdx.size() != out_numSamples)
        { errID = -3;
          break;  }

        // fill output array(s).
        std::vector <FabricCore::RTVal> args(2);
        args[0] = FabricCore::RTVal::ConstructExternalArray(*getClient(), "UInt32", dataNum.size(), (void *)dataNum.data());
        args[1] = FabricCore::RTVal::ConstructExternalArray(*getClient(), "UInt32", dataIdx.size(), (void *)dataIdx.data());
        rtMesh.callMethod("", "getTopologyAsCountsIndicesExternalArrays", 2, &args[0]);
      }

      // get polygon node normals.
      if (   out_polygonNodeNormals != NULL
          && out_numPolygons         > 0
          && out_numSamples          > 0  )
      {
        std::vector <float> &data = *out_polygonNodeNormals;

        // resize output array(s).
            data.   resize(3 * out_numSamples);
        if (data.size() != 3 * out_numSamples)
        { errID = -3;
          break;  }

        // fill output array(s).
        std::vector <FabricCore::RTVal> args(1);
        args[0] = FabricCore::RTVal::ConstructExternalArray(*getClient(), "Float32", data.size(), (void *)data.data());
        rtMesh.callMethod("", "getNormalsAsExternalArray", 1, &args[0]);
      }

      // get polygon node UVWs.
      if (   out_polygonNodeUVWs    != NULL
          && out_numPolygons         > 0
          && out_numSamples          > 0  )
      {
        if (rtMesh.callMethod("Boolean", "hasUVs", 0, NULL).getBoolean())
        {
          std::vector <float> &data = *out_polygonNodeUVWs;

          // resize output array(s).
              data.   resize(3 * out_numSamples);
          if (data.size() != 3 * out_numSamples)
          { errID = -3;
            break;  }

          // fill output array(s).
          std::vector <FabricCore::RTVal> args(2);
          args[0] = FabricCore::RTVal::ConstructExternalArray(*getClient(), "Float32", data.size(), (void *)data.data());
          args[1] = FabricCore::RTVal::ConstructUInt32       (*getClient(), 3);  
          rtMesh.callMethod("", "getUVsAsExternalArray", 2, &args[0]);
        }
      }

      // get polygon node colors.
      if (   out_polygonNodeColors  != NULL
          && out_numPolygons         > 0
          && out_numSamples          > 0  )
      {
        if (rtMesh.callMethod("Boolean", "hasVertexColors", 0, NULL).getBoolean())
        {
          std::vector <float> &data = *out_polygonNodeColors;

          // resize output array(s).
              data.   resize(4 * out_numSamples);
          if (data.size() != 4 * out_numSamples)
          { errID = -3;
            break;  }

          // fill output array(s).
          std::vector <FabricCore::RTVal> args(2);
          args[0] = FabricCore::RTVal::ConstructExternalArray(*getClient(), "Float32", data.size(), (void *)data.data());
          args[1] = FabricCore::RTVal::ConstructUInt32       (*getClient(), 4);  
          rtMesh.callMethod("", "getVertexColorsAsExternalArray", 2, &args[0]);
        }
      }
    } while (false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    errID = -4;
  }

  // if an error occurred then clear all outputs.
  if (errID)
  {
    if (errID == -3)
    { std::string s = "memory error: failed to resize std::vector<>";
      logErrorFunc(NULL, s.c_str(), s.length());  }
    out_numVertices = 0;
    out_numPolygons = 0;
    out_numSamples  = 0;
    if (out_positions)          out_positions          -> clear();
    if (out_polygonNumVertices) out_polygonNumVertices -> clear();
    if (out_polygonVertices)    out_polygonVertices    -> clear();
    if (out_polygonNodeNormals) out_polygonNodeNormals -> clear();
    if (out_polygonNodeUVWs)    out_polygonNodeUVWs    -> clear();
    if (out_polygonNodeColors)  out_polygonNodeColors  -> clear();
  }

  // done.
  return errID;
}

void BaseInterface::SetValueOfArgBoolean(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const bool val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgBoolean(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    FabricCore::RTVal rtval;
    rtval = FabricCore::RTVal::ConstructBoolean(client, val);
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgSInt(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const int32_t val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgSInt(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    FabricCore::RTVal rtval;
    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    if      (resolvedType == "Integer") rtval = FabricCore::RTVal::ConstructSInt32(client, val);
    else if (resolvedType == "SInt8")   rtval = FabricCore::RTVal::ConstructSInt8 (client, val);
    else if (resolvedType == "SInt16")  rtval = FabricCore::RTVal::ConstructSInt16(client, val);
    else if (resolvedType == "SInt32")  rtval = FabricCore::RTVal::ConstructSInt32(client, val);
    else if (resolvedType == "SInt64")  rtval = FabricCore::RTVal::ConstructSInt64(client, val);
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgUInt(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const uint32_t val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgUInt(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    FabricCore::RTVal rtval;
    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    if      (resolvedType == "Byte")      rtval = FabricCore::RTVal::ConstructUInt8 (client, val);
    else if (resolvedType == "UInt8")     rtval = FabricCore::RTVal::ConstructUInt16(client, val);
    else if (resolvedType == "UInt16")    rtval = FabricCore::RTVal::ConstructUInt16(client, val);
    else if (resolvedType == "Count")     rtval = FabricCore::RTVal::ConstructUInt32(client, val);
    else if (resolvedType == "Index")     rtval = FabricCore::RTVal::ConstructUInt32(client, val);
    else if (resolvedType == "Size")      rtval = FabricCore::RTVal::ConstructUInt32(client, val);
    else if (resolvedType == "UInt32")    rtval = FabricCore::RTVal::ConstructUInt32(client, val);
    else if (resolvedType == "DataSize")  rtval = FabricCore::RTVal::ConstructUInt64(client, val);
    else if (resolvedType == "UInt64")    rtval = FabricCore::RTVal::ConstructUInt64(client, val);
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgFloat(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const double val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgFloat(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    FabricCore::RTVal rtval;
    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    if      (resolvedType == "Scalar")  rtval = FabricCore::RTVal::ConstructFloat32(client, val);
    else if (resolvedType == "Float32") rtval = FabricCore::RTVal::ConstructFloat32(client, val);
    else if (resolvedType == "Float64") rtval = FabricCore::RTVal::ConstructFloat64(client, val);
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgString(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const std::string &val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgString(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    FabricCore::RTVal rtval;
    rtval = FabricCore::RTVal::ConstructString(client, val.c_str());
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgVec2(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const std::vector <double> &val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgVec2(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    const int  N        =     2;
    const char name[16] = "Vec2";
    FabricCore::RTVal rtval;
    FabricCore::RTVal v[N];
    const bool valIsValid  = (val.size() >= N);
    for (int i = 0; i < N; i++)
      v[i] = FabricCore::RTVal::ConstructFloat32(client, valIsValid ? val[i] : 0);
    rtval  = FabricCore::RTVal::Construct(client, name, N, v);
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgVec3(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const std::vector <double> &val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgVec3(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    const int  N        =     3;
    const char name[16] = "Vec3";
    FabricCore::RTVal rtval;
    FabricCore::RTVal v[N];
    const bool valIsValid  = (val.size() >= N);
    for (int i = 0; i < N; i++)
      v[i] = FabricCore::RTVal::ConstructFloat32(client, valIsValid ? val[i] : 0);
    rtval  = FabricCore::RTVal::Construct(client, name, N, v);
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgVec4(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const std::vector <double> &val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgVec4(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    const int  N        =     4;
    const char name[16] = "Vec4";
    FabricCore::RTVal rtval;
    FabricCore::RTVal v[N];
    const bool valIsValid  = (val.size() >= N);
    for (int i = 0; i < N; i++)
      v[i] = FabricCore::RTVal::ConstructFloat32(client, valIsValid ? val[i] : 0);
    rtval  = FabricCore::RTVal::Construct(client, name, N, v);
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgColor(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const std::vector <double> &val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgColor(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    const int  N        =      4;
    const char name[16] = "Color";
    FabricCore::RTVal rtval;
    FabricCore::RTVal v[N];
    const bool valIsValid  = (val.size() >= N);
    for (int i = 0; i < N; i++)
      v[i] = FabricCore::RTVal::ConstructFloat32(client, valIsValid ? val[i] : 0);
    rtval  = FabricCore::RTVal::Construct(client, name, N, v);
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgRGB(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const std::vector <double> &val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgRGB(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    const int  N        =    3;
    const char name[16] = "RGB";
    FabricCore::RTVal rtval;
    FabricCore::RTVal v[N];
    const bool valIsValid  = (val.size() >= N);
    for (int i = 0; i < N; i++)
      v[i] = FabricCore::RTVal::ConstructUInt8(client, valIsValid ? (uint8_t)std::max(0.0, std::min(255.0, 255.0 * val[i])) : 0);
    rtval  = FabricCore::RTVal::Construct(client, name, N, v);
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgRGBA(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const std::vector <double> &val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgRGBA(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    const int  N        =     4;
    const char name[16] = "RGBA";
    FabricCore::RTVal rtval;
    FabricCore::RTVal v[N];
    const bool valIsValid  = (val.size() >= N);
    for (int i = 0; i < N; i++)
      v[i] = FabricCore::RTVal::ConstructUInt8(client, valIsValid ? (uint8_t)std::max(0.0, std::min(255.0, 255.0 * val[i])) : 0);
    rtval  = FabricCore::RTVal::Construct(client, name, N, v);
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgQuat(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const std::vector <double> &val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgQuat(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    FabricCore::RTVal rtval;
    FabricCore::RTVal xyz[3], v[2];
    const bool valIsValid = (val.size() >= 4);
    xyz[0] = FabricCore::RTVal::ConstructFloat32(client, valIsValid ? val[0] : 0);
    xyz[1] = FabricCore::RTVal::ConstructFloat32(client, valIsValid ? val[1] : 0);
    xyz[2] = FabricCore::RTVal::ConstructFloat32(client, valIsValid ? val[2] : 0);
    v[0]   = FabricCore::RTVal::Construct(client, "Vec3", 3, xyz);
    v[1]   = FabricCore::RTVal::ConstructFloat32(client, valIsValid ? val[3] : 0);
    rtval  = FabricCore::RTVal::Construct(client, "Quat", 2, v);
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgMat44(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const std::vector <double> &val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgMat44(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    FabricCore::RTVal rtval;
    FabricCore::RTVal xyzt[4], v[4];
    const bool valIsValid = (val.size() >= 16);
    for (int i = 0; i < 4; i++)
    {
      int offset = i * 4;
      xyzt[0] = FabricCore::RTVal::ConstructFloat32(client, valIsValid ? val[offset + 0] : 0);
      xyzt[1] = FabricCore::RTVal::ConstructFloat32(client, valIsValid ? val[offset + 1] : 0);
      xyzt[2] = FabricCore::RTVal::ConstructFloat32(client, valIsValid ? val[offset + 2] : 0);
      xyzt[3] = FabricCore::RTVal::ConstructFloat32(client, valIsValid ? val[offset + 3] : 0);
      v[i]    = FabricCore::RTVal::Construct(client, "Vec4", 4, xyzt);
    }
    rtval = FabricCore::RTVal::Construct(client, "Mat44", 4, v);
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

void BaseInterface::SetValueOfArgPolygonMesh(FabricCore::Client &client, FabricCore::DFGBinding &binding, char const * argName, const _polymesh &val)
{
  if (!binding.getExec().haveExecPort(argName))
  {
    std::string s = "BaseInterface::SetValueOfArgPolygonMesh(): port not found.";
    logErrorFunc(NULL, s.c_str(), s.length());
    return;
  }

  try
  {
    FabricCore::RTVal rtval;
    rtval = FabricSplice::constructObjectRTVal("PolygonMesh");
    rtval.callMethod("", "clear", 0, NULL);
    if (val.isValid() && !val.isEmpty())
    {
      std::vector <FabricCore::RTVal> args(2);

      // vertices.
      args[0] = FabricCore::RTVal::ConstructExternalArray(client, "Float32", val.vertPositions.size(), (void *)val.vertPositions.data());
      args[1] = FabricCore::RTVal::ConstructUInt32(client, 3);
      rtval.callMethod("", "setPointsFromExternalArray", 2, &args[0]);

      // polygonal description.
      args[0] = FabricCore::RTVal::ConstructExternalArray(client, "UInt32", val.polyNumVertices.size(), (void *)val.polyNumVertices.data());
      args[1] = FabricCore::RTVal::ConstructExternalArray(client, "UInt32", val.polyVertices   .size(), (void *)val.polyVertices   .data());
      rtval.callMethod("", "setTopologyFromCountsIndicesExternalArrays", 2, &args[0]);

      // normals.
      args[0] = FabricCore::RTVal::ConstructExternalArray(client, "Float32", val.polyNodeNormals.size(), (void *)val.polyNodeNormals.data());
      rtval.callMethod("", "setNormalsFromExternalArray", 1, &args[0]);

      // UVWs.
      if (val.hasUVWs())
      {
        args[0] = FabricCore::RTVal::ConstructExternalArray(client, "Float32", val.polyNodeUVWs.size(), (void *)val.polyNodeUVWs.data());
        args[1] = FabricCore::RTVal::ConstructUInt32(client, 3);
        rtval.callMethod("", "setUVsFromExternalArray", 2, &args[0]);
      }

      // colors.
      if (val.hasColors())
      {
        args[0] = FabricCore::RTVal::ConstructExternalArray(client, "Float32", val.polyNodeColors.size(), (void *)val.polyNodeColors.data());
        args[1] = FabricCore::RTVal::ConstructUInt32(client, 4);
        rtval.callMethod("", "setVertexColorsFromExternalArray", 2, &args[0]);
      }
    }
    binding.setArgValue(argName, rtval, false);
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}





