#ifndef _FabricDFGBaseInterface_H_
#define _FabricDFGBaseInterface_H_

// disable some annoying VS warnings.
#pragma warning(disable : 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc.
#pragma warning(disable : 4800)  // forcing value to bool 'true' or 'false'.
#pragma warning(disable : 4806)  // unsafe operation: no value of type 'bool' promoted to type ...etc.

// includes.
#include <DFGWrapper/DFGWrapper.h>
#include <ASTWrapper/KLASTManager.h>
#include <Commands/CommandStack.h>
#include <map>

// a management class for client and host
class BaseInterface : public FabricServices::DFGWrapper::View
{
 public:

  BaseInterface(void (*in_logFunc)     (void *, const char *, unsigned int) = NULL,
                void (*in_logErrorFunc)(void *, const char *, unsigned int) = NULL);
  ~BaseInterface();

  // instance management
  // right now there are no locks in place,
  // assuming that the DCC will only access
  // these things from the main thread.
  unsigned int getId();
  static BaseInterface *getFromId(unsigned int id);

  // accessors
  static FabricCore::Client                       *getClient();
  static FabricServices::DFGWrapper::Host         *getHost();
  FabricServices::DFGWrapper::Binding             *getBinding();
  static FabricServices::ASTWrapper::KLASTManager *getManager();
  static FabricServices::Commands::CommandStack   *getStack();

  // persistence
  std::string getJSON();
  void setFromJSON(const std::string & json);

  // logging.
  static void setLogFunc(void (*in_logFunc)(void *, const char *, unsigned int));
  static void setLogErrorFunc(void (*in_logErrorFunc)(void *, const char *, unsigned int));

  // notifications
  virtual void onNotification(char const * json)                                                                                  {}
  virtual void onNodeInserted(FabricServices::DFGWrapper::NodePtr node)                                                           {}
  virtual void onNodeRemoved(FabricServices::DFGWrapper::NodePtr node)                                                            {}
  virtual void onPinInserted(FabricServices::DFGWrapper::PinPtr pin)                                                              {}
  virtual void onPinRemoved(FabricServices::DFGWrapper::PinPtr pin)                                                               {}
  virtual void onPortInserted(FabricServices::DFGWrapper::PortPtr port)                                                           {}
  virtual void onPortRemoved(FabricServices::DFGWrapper::PortPtr port)                                                            {}
  virtual void onEndPointsConnected(FabricServices::DFGWrapper::EndPointPtr src, FabricServices::DFGWrapper::EndPointPtr dst)     {}
  virtual void onEndPointsDisconnected(FabricServices::DFGWrapper::EndPointPtr src, FabricServices::DFGWrapper::EndPointPtr dst)  {}
  virtual void onNodeMetadataChanged(FabricServices::DFGWrapper::NodePtr node, const char * key, const char * metadata)           {}
  virtual void onNodeTitleChanged(FabricServices::DFGWrapper::NodePtr node, const char * title)                                   {}
  virtual void onPortRenamed(FabricServices::DFGWrapper::PortPtr port, const char * oldName)                                      {}
  virtual void onPinRenamed(FabricServices::DFGWrapper::PinPtr pin, const char * oldName)                                         {}
  virtual void onExecMetadataChanged(FabricServices::DFGWrapper::ExecutablePtr exec, const char * key, const char * metadata)     {}
  virtual void onExtDepAdded(const char * extension, const char * version)                                                        {}
  virtual void onExtDepRemoved(const char * extension, const char * version)                                                      {}
  virtual void onNodeCacheRuleChanged(const char * path, const char * rule)                                                       {}
  virtual void onExecCacheRuleChanged(const char * path, const char * rule)                                                       {}
  virtual void onPortResolvedTypeChanged(FabricServices::DFGWrapper::PortPtr port, const char * resolvedType)                     {}
  virtual void onPortTypeSpecChanged(FabricServices::DFGWrapper::PortPtr port, const char * typeSpec)                             {}
  virtual void onPinResolvedTypeChanged(FabricServices::DFGWrapper::PinPtr pin, const char * resolvedType)                        {}
  virtual void onPortMetadataChanged(FabricServices::DFGWrapper::PortPtr port, const char * key, const char * metadata)           {}
  virtual void onPinMetadataChanged(FabricServices::DFGWrapper::PinPtr pin, const char * key, const char * metadata)              {}

  // binding notifications.
  static void bindingNotificationCallback(void *userData, char const *jsonCString, uint32_t jsonLength);

 private:

  // logging.
  static void logFunc(void *userData, const char *message, unsigned int length);
  static void logErrorFunc(void * userData, const char * message, unsigned int length);
  static void (*s_logFunc)(void *, const char *, unsigned int);
  static void (*s_logErrorFunc)(void *, const char *, unsigned int);

  // member vars.
  unsigned int        m_id;
  static unsigned int s_maxId;
  static FabricCore::Client                        s_client;
  static FabricServices::DFGWrapper::Host         *s_host;
  static FabricServices::ASTWrapper::KLASTManager *s_manager;
  static FabricServices::Commands::CommandStack    s_stack;
  FabricServices::DFGWrapper::Binding              m_binding;
  static std::map<unsigned int, BaseInterface*>    s_instances;

  // returns true if the binding's executable has a port called portName that matches the port type (input/output).
  // params:  in_portName     name of the port.
  //          testForInput    true: look for input port, else for output port.
  bool HasPort(const char *in_portName, const bool testForInput);

 public:

  // returns the amount of base interfaces.
  static int GetNumBaseInterfaces(void)  {  return s_instances.size();  }

  // returns true if the binding's executable has an input port called portName.
  bool HasInputPort(const char *portName);
  bool HasInputPort(const std::string &portName);

  // returns true if the binding's executable has an output port called portName.
  bool HasOutputPort(const char *portName);
  bool HasOutputPort(const std::string &portName);
};

#endif
