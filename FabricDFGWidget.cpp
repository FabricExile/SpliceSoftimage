#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_desktop.h>
#include <xsi_status.h>

#include "FabricDFGPlugin.h"
#include "FabricDFGWidget.h"
#include "FabricDFGOperators.h"

#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/qboxlayout.h>

#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>

#include <DFGWrapper/DFGWrapper.h>
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
  QApplication    *qtApp;
  QBoxLayout      *qtLayout;
  FabricDFGWidget *qtDFGWidget;
  QDialog         *qtDialog;

  _windowData(void)
  {
    qtApp         = NULL;
    qtDialog      = NULL;
    qtDFGWidget   = NULL;
    qtLayout      = NULL;
  }

  ~_windowData()
  {
    if (qtApp)  delete qtApp;
  }
};

bool g_canvasIsOpen = false;  // global flag to ensure that not more than one Canvas is open.

void OpenCanvas(_opUserData *pud, const char *winTitle)
{
  // canvas already open?
  if (g_canvasIsOpen)   return;

  // check.
  if (!pud)                     return;
  if (!pud->GetBaseInterface()) return;

  // set global flag to block any further canvases.
  g_canvasIsOpen = true;

  // declare and fill window data structure.
  _windowData winData;
  try
  {
    if (winData.qtApp == NULL)
    {
      int argc = 0;
      winData.qtApp         = new QApplication    (argc, NULL);
      winData.qtDialog      = new QDialog         (NULL);
      winData.qtDFGWidget   = new FabricDFGWidget (winData.qtDialog);
      winData.qtLayout      = new QVBoxLayout     (winData.qtDialog);

      winData.qtDialog->setWindowTitle(winTitle ? winTitle : "Canvas");
      winData.qtLayout->addWidget(winData.qtDFGWidget); 
      winData.qtLayout->setContentsMargins(0, 0, 0, 0);

      // for some reason setting the qtDialog to modal doesn't work.
      /*winData.qtDialog->setWindowModality(Qt::WindowModal);*/

      // parenting the qtDialog to the Softimage main window results in a weird mouse position offset
      // => as a temporary workaround, we set the qtDialog to top most.
      winData.qtDialog->setWindowFlags(Qt::WindowStaysOnTopHint); // SetParent((HWND)winData.qtDialog->winId(), (HWND)Application().GetDesktop().GetApplicationWindowHandle());

      // init.
      DFG::DFGConfig config;
      config.graphConfig.useOpenGL = false;
      winData.qtDFGWidget->init(pud->GetBaseInterface()->getClient(),
                                pud->GetBaseInterface()->getManager(),
                                pud->GetBaseInterface()->getHost(),
                               *pud->GetBaseInterface()->getBinding(),
                                pud->GetBaseInterface()->getGraph(),
                                pud->GetBaseInterface()->getStack(),
                                true,
                                config
                               );

      // show/execute Qt dialog.
      winData.qtDialog->exec();
    }
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // done.
  g_canvasIsOpen = false;
}