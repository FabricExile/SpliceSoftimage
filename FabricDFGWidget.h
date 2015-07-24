#ifndef __FabricDFGWidget_H_
#define __FabricDFGWidget_H_

// constants
enum OPENCANVAS_RETURN_VALS // return values of OpenCanvas().
{
  SUCCESS       = 0,  // success.
  ALREADY_OPEN,       // there already is an open canvas window.
  NULL_POINTER,       // some pointer is NULL.
};

// forward declarations.
struct _opUserData;
const char *GetOpenCanvasErrorDescription(OPENCANVAS_RETURN_VALS in_errID);
OPENCANVAS_RETURN_VALS OpenCanvas(_opUserData *pud, const char *winTitle);

#endif
