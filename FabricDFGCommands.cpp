#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>
#include <xsi_command.h>
#include <xsi_argument.h>

#include "FabricDFGBaseInterface.h"

using namespace XSI;

SICALLBACK dfgLogVersion_Init(CRef &in_ctxt)
{
  Context ctxt(in_ctxt);
  Command oCmd;
  oCmd = ctxt.GetSource();
  oCmd.PutDescription(L"logs the plugin/Fabric version in the history log.");
  oCmd.SetFlag(siNoLogging, false);
  oCmd.EnableReturnValue(true) ;
  ArgumentArray oArgs = oCmd.GetArguments();
  return CStatus::OK;
}

SICALLBACK dfgLogVersion_Execute(CRef & in_ctxt)
{
  CString s = L"   Fabric Engine Plugin, Fabric Core v. " + CString(FabricCore::GetVersionStr()) + L"   ";
  CString t;
  for (int i=0;i<s.Length();i++)
    t += L"-";
  Application().LogMessage(t, siInfoMsg);
  Application().LogMessage(s, siInfoMsg);
  Application().LogMessage(t, siInfoMsg);
  
  return CStatus::OK;
}
