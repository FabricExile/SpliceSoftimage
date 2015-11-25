#ifndef __FabricDFGWidget_H_
#define __FabricDFGWidget_H_

#include "FabricDFGPlugin.h"
#include <DFG/DFGCombinedWidget.h>

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

class FabricDFGWidget : public FabricUI::DFG::DFGCombinedWidget
{
 public:

  FabricDFGWidget(QWidget *parent) : FabricUI::DFG::DFGCombinedWidget(parent)
  {
  }

  virtual void onUndo()
  {
    XSI::Application().ExecuteCommand(L"Undo", XSI::CValueArray(), XSI::CValue());
  }

  virtual void onRedo()
  {
    XSI::Application().ExecuteCommand(L"Redo", XSI::CValueArray(), XSI::CValue());
  }

  static void log(void *userData, const char *message, unsigned int length)
  {
    std::string mess = std::string("[CANVAS] ") + (message ? message : "");
    feLog(userData, mess.c_str(), mess.length());
  }

  bool eventFilter(QObject *watched, QEvent *event)
  {
    if (event->type() == QEvent::KeyPress)
    {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
      switch (keyEvent->key())
      {
        case 89:  // redo.
        {
          XSI::Application().ExecuteCommand(L"Redo", XSI::CValueArray(), XSI::CValue());
          return true;
        }
        case 90:  // undo.
        {
          XSI::Application().ExecuteCommand(L"Undo", XSI::CValueArray(), XSI::CValue());
          return true;
        }
        default:
        {
          break;
        }
      }
    }
    return false;
  }
};

void FabricInitQt();

#endif
