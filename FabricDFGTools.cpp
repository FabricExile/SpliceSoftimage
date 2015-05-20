#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_status.h>
#include <xsi_command.h>
#include <xsi_argument.h>
#include <xsi_uitoolkit.h>
#include <xsi_selection.h>
#include <xsi_customoperator.h>
#include <xsi_operatorcontext.h>
#include <xsi_port.h>
#include <xsi_inputport.h>
#include <xsi_outputport.h>
#include <xsi_portgroup.h>
#include <xsi_inputport.h>
#include <xsi_factory.h>
#include <xsi_model.h>
#include <xsi_x3dobject.h>
#include <xsi_kinematics.h>

#include "FabricDFGPlugin.h"
#include "FabricDFGOperators.h"
#include "FabricDFGTools.h"

using namespace XSI;

// for a given X3DObject fill the CRef array out_refs with all refs
// at "dfgSoftimageOp" custom operators that belong to the X3DObject.
// return value: the amount of custom operators that were found.
int dfgTool_GetRefsAtOps(XSI::X3DObject &in_obj, XSI::CRefArray out_refs)
{
  // init output.
  out_refs.Clear();

  // fill out_refs.
  if (in_obj.IsValid())
  {
    CRefArray refs = Application().FindObjects(siCustomOperatorID);
    for (int i=0;i<refs.GetCount();i++)
    {
      CustomOperator op(refs[i]);
      if (   op.GetType() == "dfgSoftimageOp"
          && op.GetParent3DObject().GetUniqueName() == in_obj.GetUniqueName())
      {
        out_refs.Add(refs[i]);
      }
    }
  }

  // done.
  return out_refs.GetCount();
}
