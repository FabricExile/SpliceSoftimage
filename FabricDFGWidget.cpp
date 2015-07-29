#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_desktop.h>
#include <xsi_status.h>

#include "FabricDFGPlugin.h"
#include "FabricDFGWidget.h"
#include "FabricDFGOperators.h"
#include "FabricDFGUICmdHandlerXSI.h"

#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QBoxLayout>
#include <QtGui/QPalette>

#ifdef _WIN32
  #include <windows.h>
  #include <io.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>

#include <DFG/DFGCombinedWidget.h>

using namespace XSI;

class FabricDFGWidget : public DFG::DFGCombinedWidget
{
 public:

  FabricDFGWidget(QWidget *parent) : DFGCombinedWidget(parent)
  {
  }

  virtual void onRecompilation()
  {
    std::string s = "Recompiling";
    log(NULL, s.c_str(), s.length());
  }

  static void log(void *userData, const char *message, unsigned int length)
  {
    feLog(userData, message, length);
  }
};

struct _windowData
{
  QBoxLayout      *qtLayout;
  FabricDFGWidget *qtDFGWidget;
  QDialog         *qtDialog;

  _windowData(void)
  {
    qtDialog      = NULL;
    qtDFGWidget   = NULL;
    qtLayout      = NULL;
  }
};

const char *GetOpenCanvasErrorDescription(OPENCANVAS_RETURN_VALS in_errID)
{
  static const char str_SUCCESS     [] = "success";
  static const char str_ALREADY_OPEN[] = "did not open Canvas, because there already is an open Canvas window";
  static const char str_NULL_POINTER[] = "failed to open Canvas: a pointer is NULL";
  static const char str_UNKNOWN     [] = "failed to open Canvas: unknown error";
  switch (in_errID)
  {
    case OPENCANVAS_RETURN_VALS::SUCCESS:       return str_SUCCESS;
    case OPENCANVAS_RETURN_VALS::ALREADY_OPEN:  return str_ALREADY_OPEN;
    case OPENCANVAS_RETURN_VALS::NULL_POINTER:  return str_NULL_POINTER;
    default:                                    return str_UNKNOWN;
  }
}

OPENCANVAS_RETURN_VALS OpenCanvas(_opUserData *pud, const char *winTitle)
{
  // static flag to ensure that not more than one Canvas is open at the same time.
  static bool s_canvasIsOpen = false;

  // check.
  if (s_canvasIsOpen)
  {
    Application().LogMessage(L"Not opening Canvas... reason: there already is an open Canvas window.", siWarningMsg);
    return OPENCANVAS_RETURN_VALS::ALREADY_OPEN;
  }

  // init Qt app, if necessary.
  if (!qApp)
  {
    Application().LogMessage(L"allocating an instance of QApplication", siCommentMsg);
    int argc = 0;
    new QApplication(argc, NULL);
  }

  // check.
  if (!qApp)                    return OPENCANVAS_RETURN_VALS::NULL_POINTER;
  if (!pud)                     return OPENCANVAS_RETURN_VALS::NULL_POINTER;
  if (!pud->GetBaseInterface()) return OPENCANVAS_RETURN_VALS::NULL_POINTER;

  // set flag to block any further canvas.
  s_canvasIsOpen = true;

  // declare and fill window data structure.
  _windowData winData;
  try
  {
    winData.qtDialog      = new QDialog         (NULL);
    winData.qtDFGWidget   = new FabricDFGWidget (winData.qtDialog);
    winData.qtLayout      = new QVBoxLayout     (winData.qtDialog);

    winData.qtDialog->setWindowTitle(winTitle ? winTitle : "Canvas");
    winData.qtLayout->addWidget(winData.qtDFGWidget); 
    winData.qtLayout->setContentsMargins(0, 0, 0, 0);

    // for some reason setting the qtDialog to modal doesn't work.
    /*winData.qtDialog->setWindowModality(Qt::WindowModal);*/

    // parenting the qtDialog to the Softimage main window results in a weird mouse position offset
    /*SetParent((HWND)winData.qtDialog->winId(), (HWND)Application().GetDesktop().GetApplicationWindowHandle());*/

    // init the DFG widget.
    DFG::DFGConfig config;
    float f = 0.8f / 255.0f;
    config.graphConfig.useOpenGL                = false;
    config.graphConfig.headerBackgroundColor    . setRgbF(f * 113, f * 112, f * 111);
    config.graphConfig.mainPanelBackgroundColor . setRgbF(f * 127, f * 127, f * 127);
    config.graphConfig.sidePanelBackgroundColor . setRgbF(f * 171, f * 168, f * 166);
    winData.qtDFGWidget->init(*pud->GetBaseInterface()->getClient(),
                               pud->GetBaseInterface()->getManager(),
                               pud->GetBaseInterface()->getHost(),
                               pud->GetBaseInterface()->getBinding(),
                               "",
                               pud->GetBaseInterface()->getBinding().getExec(),
                               pud->GetBaseInterface()->getCmdHandler(),
                               false,
                               config
                             );
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // show/execute Qt dialog.
  try
  {
    const bool useExec = true;

    // use the widget's exec() function.
    if (useExec)
    {
      winData.qtDialog->exec();
    }

    // use a manual loop.
    else
    {
      winData.qtDialog->show();
      while (winData.qtDialog->isVisible())
      {
        qApp->processEvents();
      }
    }
  }
  catch(std::exception &e)
  {
    feLogError(e.what() ? e.what() : "\"\"");
    winData.qtDialog->close();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // clean up.
  s_canvasIsOpen = false;
  try
  {
    //delete winData.qtDFGWidget;
    //delete winData.qtLayout;
    //delete winData.qtDialog;
    //delete qApp;
  }
  catch(std::exception &e)
  {
    feLogError(e.what() ? e.what() : "\"\"");
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // done.
  return OPENCANVAS_RETURN_VALS::SUCCESS;
}
