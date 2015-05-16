#include "FabricDFGBaseInterface.h"

FabricCore::Client BaseInterface::s_client;
FabricServices::DFGWrapper::Host * BaseInterface::s_host = NULL;
FabricServices::ASTWrapper::KLASTManager * BaseInterface::s_manager = NULL;
FabricServices::Commands::CommandStack BaseInterface::s_stack;
unsigned int BaseInterface::s_maxId = 0;
void (*BaseInterface::s_logFunc)(void *, const char *, unsigned int) = NULL;
void (*BaseInterface::s_logErrorFunc)(void *, const char *, unsigned int) = NULL;
std::map<unsigned int, BaseInterface*> BaseInterface::s_instances;

BaseInterface::BaseInterface(void (*in_logFunc)     (void *, const char *, unsigned int),
                             void (*in_logErrorFunc)(void *, const char *, unsigned int))
{
  //
  if (in_logFunc)       setLogFunc(in_logFunc);
  if (in_logErrorFunc)  setLogErrorFunc(in_logErrorFunc);

  //
  m_id = s_maxId++;
  std::string m;
  m  = "calling BaseInterface(), m_id = " + std::to_string((long long)m_id);
  logFunc(NULL, m.c_str(), m.length());

  // construct the client
  if (s_instances.size() == 0)
  {
    try
    {
      // create a client
      FabricCore::Client::CreateOptions options;
      memset(&options, 0, sizeof(options));
      options.guarded = 1;
      options.optimizationType = FabricCore::ClientOptimizationType_Background;
      s_client = FabricCore::Client(&logFunc, NULL, &options);

      // load basic extensions
      s_client.loadExtension("Math",     "", false);
      s_client.loadExtension("Geometry", "", false);
      s_client.loadExtension("FileIO",   "", false);

      // create a host for Canvas
      s_host = new FabricServices::DFGWrapper::Host(s_client);

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
    m_binding = s_host->createBindingToNewGraph();
    m_binding.setNotificationCallback(bindingNotificationCallback, this);

    // set the graph on the view
    FabricServices::DFGWrapper::View::setGraph(FabricServices::DFGWrapper::GraphExecutablePtr::StaticCast(m_binding.getExecutable()));
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
  }
}

BaseInterface::~BaseInterface()
{
  std::string m;
  m  = "calling ~BaseInterface(), m_id = " + std::to_string((long long)m_id);
  logFunc(NULL, m.c_str(), m.length());

  std::map<unsigned int, BaseInterface*>::iterator it = s_instances.find(m_id);

  m_binding = FabricServices::DFGWrapper::Binding();

  if (it != s_instances.end())
  {
    s_instances.erase(it);
    if (s_instances.size() == 0)
    {
      try
      {
        printf("Destructing client...\n");
        s_stack.clear();
        delete(s_manager);
        delete(s_host);
        s_client = FabricCore::Client();
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

FabricServices::DFGWrapper::Host *BaseInterface::getHost()
{
  return s_host;
}

FabricServices::DFGWrapper::Binding *BaseInterface::getBinding()
{
  return &m_binding;
}

FabricServices::ASTWrapper::KLASTManager *BaseInterface::getManager()
{
  return s_manager;
}

FabricServices::Commands::CommandStack *BaseInterface::getStack()
{
  return &s_stack;
}

std::string BaseInterface::getJSON()
{
  try
  {
    return m_binding.exportJSON();
  }
  catch (FabricCore::Exception e)
  {
    logErrorFunc(NULL, e.getDesc_cstr(), e.getDescLength());
    return "";
  }
}

void BaseInterface::setFromJSON(const std::string & json)
{
  try
  {
    m_binding = s_host->createBindingFromJSON(json.c_str());
    m_binding.setNotificationCallback(bindingNotificationCallback, this);
    FabricServices::DFGWrapper::View::setGraph(FabricServices::DFGWrapper::GraphExecutablePtr::StaticCast(m_binding.getExecutable()));
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

  // wip.
  // NOTHING HERE YET.
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
    FabricServices::DFGWrapper::GraphExecutablePtr graph = FabricServices::DFGWrapper::GraphExecutablePtr::StaticCast(m_binding.getExecutable());
    if (graph.isNull())
    {
      std::string s = "BaseInterface::HasPort(): m_binding.getExecutable() returned NULL pointer.";
      logErrorFunc(NULL, s.c_str(), s.length());
      return false;
    }

    // get the port.
    FabricServices::DFGWrapper::PortPtr port = graph->getPort(in_portName);

    // return result.
    return (   !port.isNull()
            &&  port->isValid()
            &&  port->getPortType() == portType);
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





