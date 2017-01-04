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
    XSI::CValueArray args;
    XSI::CValue val;
    XSI::Application().ExecuteCommand(L"Undo", args, val);
  }

  virtual void onRedo()
  {
    XSI::CValueArray args;
    XSI::CValue val;
    XSI::Application().ExecuteCommand(L"Redo", args, val);
  }

  virtual void onSelectCanvasNodeInDCC()
  {
  }

  virtual void onImportGraphInDCC()
  {
  }

  virtual void onExportGraphInDCC()
  {
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
        case Qt::Key_Y:  // redo.
        {
          if (keyEvent->modifiers().testFlag(Qt::ControlModifier))
          {
            XSI::CValueArray args;
            XSI::CValue val;
            XSI::Application().ExecuteCommand(L"Redo", args, val);
            return true;
          }
        } break;
        case Qt::Key_Z:  // undo.
        {
          if (keyEvent->modifiers().testFlag(Qt::ControlModifier))
          {
            XSI::CValueArray args;
            XSI::CValue val;
            XSI::Application().ExecuteCommand(L"Undo", args, val);
            return true;
          }
        } break;
        default:
        {
          // the Canvas window in Softimage is a modal
          // dialog, so we accept all key press events.
          return true;
        }
      }
    }
    return false;
  }
};

void FabricInitQt();

#endif
