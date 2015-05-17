#ifndef __FabricDFGOperators_H_
#define __FabricDFGOperators_H_

#include "FabricDFGBaseInterface.h"

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

#endif
