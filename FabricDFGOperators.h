#ifndef __FabricDFGOperators_H_
#define __FabricDFGOperators_H_

#include <xsi_string.h>
#include <xsi_customoperator.h>

#include "FabricDFGBaseInterface.h"

// constants (port mapping).
enum DFG_PORT_TYPE
{
  UNDEFINED = 0,
  IN,
  OUT,
};
enum DFG_PORT_MAPTYPE
{
  INTERNAL =  0,
  XSI_PARAMETER,
  XSI_PORT,
  XSI_ICE_PORT,
};

// ___________________________
// structure for port mapping.
struct _portMapping
{
  // DFG port.
  XSI::CString  dfgPortName;       // port name.
  DFG_PORT_TYPE dfgPortType;       // port type (one of DFG_PORT_TYPE_*).
  XSI::CString  dfgPortDataType;   // data type ("SInt32", "Vec3", "PolygonMesh", etc.).

  // mapping.
  DFG_PORT_MAPTYPE mapType;       // specifies how the DFG port is to be mapped/exposed.
  XSI::CString mapTarget;         // if mapType == "XSIPort" then the full name of the target (or L"" for no target).

  void clear(void)
  {
    dfgPortName     . Clear();
    dfgPortType     = DFG_PORT_TYPE::UNDEFINED;
    dfgPortDataType . Clear();
    mapType          = DFG_PORT_MAPTYPE::INTERNAL;
    mapTarget       .Clear();
  }
};

// _____________________________________
// dfgSoftimageOp's user data structure.
struct _opUserData
{
 private:
 
  BaseInterface *m_baseInterface;
  static std::map <unsigned int, _opUserData *> s_instances;

 public:

  // constructor.
  _opUserData(unsigned int operatorObjectID)
  {
    // clear everything.
    memset(this, NULL, sizeof(*this));

    // create base interface.
    m_baseInterface = new BaseInterface(feLog, feLogError);

    // insert this in map.
    s_instances.insert(std::pair<unsigned int, _opUserData *>(operatorObjectID, this));

  }

  // destructor.
  ~_opUserData()
  {
    if (m_baseInterface)
    {
      // delete the base interface.
      delete m_baseInterface;

      // remove this from map.
      for (std::map<unsigned int, _opUserData *>::iterator it=s_instances.begin();it!=s_instances.end();it++)
        if (it->second == this)
        {
          s_instances.erase(it);
          break;
        }
    }
  }

  // return pointer at base interface.
  BaseInterface *GetBaseInterface(void)
  {
    return m_baseInterface;
  }

  // return pointer at _opUserData for a given operator's ObjectID.
  static _opUserData *GetUserData(unsigned int operatorObjectID)
  {
    std::map <unsigned int, _opUserData *>::iterator it = s_instances.find(operatorObjectID);
    if (it != s_instances.end())  return it->second;
    else                          return NULL;
  }

  // return pointer at base interface for a given operator's ObjectID.
  static BaseInterface *GetBaseInterface(unsigned int operatorObjectID)
  {
    _opUserData *pud = GetUserData(operatorObjectID);
    if (pud)  return pud->GetBaseInterface();
    else      return NULL;
  }

  // return amount of _opUserData instances.
  static int GetNumOpUserData(void)
  {
    if (s_instances.empty())  return 0;
    else                      return (int)s_instances.size();
  }
};

// forward declarations.
bool DefinePortMapping(_portMapping *&io_pm, int &in_numPm);

#endif
