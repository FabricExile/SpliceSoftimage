#include <xsi_application.h>

#include "FabricDFGBaseInterface.h"
#include "FabricDFGPlugin.h"
#include <DFG/DFGCombinedWidget.h>

// log system.
void feLog(void *userData, const char *s, unsigned int length)
{
  const char *p = (s != NULL ? s : "s == NULL");
  XSI::Application().LogMessage(L"[FABRIC] " + XSI::CString(p), XSI::siVerboseMsg);
  FabricUI::DFG::DFGLogWidget::log(p);
}
void feLog(void *userData, const std::string &s)
{
  feLog(userData, s.c_str(), s.length());
}
void feLog(const std::string &s)
{
  feLog(NULL, s.c_str(), s.length());
}
void feLogError(void *userData, const char *s, unsigned int length)
{
  const char *p = (s != NULL ? s : "s == NULL");
  XSI::Application().LogMessage(L"[FABRIC ERROR] " + XSI::CString(p), XSI::siErrorMsg);
  std::string t = p;
  t = "Error: " + t;
  FabricUI::DFG::DFGLogWidget::log(t.c_str());
}
void feLogError(void *userData, const std::string &s)
{
  feLogError(userData, s.c_str(), s.length());
}
void feLogError(const std::string &s)
{
  feLogError(NULL, s.c_str(), s.length());
}


