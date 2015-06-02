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
    log(0, "Recompiling", 12);
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

  FabricCore::Client                             *client;
  FabricServices::ASTWrapper::KLASTManager       *manager;
  FabricServices::DFGWrapper::Host               *host;
  FabricServices::DFGWrapper::Binding             binding;
  FabricServices::DFGWrapper::GraphExecutablePtr  graph;
  FabricServices::Commands  ::CommandStack        stack;

  _windowData(void)
  {
    qtApp         = NULL;
    qtDialog      = NULL;
    qtDFGWidget   = NULL;
    qtLayout      = NULL;

    client        = NULL;
    manager       = NULL;
    host          = NULL;
  }

  ~_windowData()
  {
    if (qtApp)  delete qtApp;
  }
};

bool g_canvasIsOpen = false;
void OpenCanvas(_opUserData *pud, const char *winTitle)
{
  // check.
  if (!pud || g_canvasIsOpen)  return;
  BaseInterface *baseInterface = pud->GetBaseInterface();
  if (!baseInterface)         return;

  // set global flag to block any further canvases.
  g_canvasIsOpen = true;

  // declare and fill window data structure.
  _windowData winData;
  try
  {
    winData.client   =  baseInterface->getClient();
    winData.manager  =  baseInterface->getManager();
    winData.host     =  baseInterface->getHost();
    winData.binding  = *baseInterface->getBinding();
    winData.graph    =  baseInterface->getGraph();
    winData.stack    = *baseInterface->getStack();

    _windowData *wd = &winData;
    if (wd->qtApp == NULL)
    {
      int argc = 0;
      wd->qtApp         = new QApplication    (argc, NULL);
      wd->qtDialog      = new QDialog         (NULL);
      wd->qtDFGWidget   = new FabricDFGWidget (wd->qtDialog);
      wd->qtLayout      = new QVBoxLayout     (wd->qtDialog);

      wd->qtDialog->setWindowTitle(winTitle ? winTitle : "Canvas");
      wd->qtLayout->addWidget(wd->qtDFGWidget); 
      wd->qtLayout->setContentsMargins(0, 0, 0, 0);
      wd->qtDialog->setWindowModality(Qt::WindowModal); // <- for some reason this doesn't work.

      // init.
      DFG::DFGConfig config;
      config.graphConfig.useOpenGL = false;
      wd->qtDFGWidget->init(wd->client,
                            wd->manager,
                            wd->host,
                            wd->binding,
                            wd->graph,
                           &wd->stack,
                            true,
                            config
                           );

      // use Windows function SetParent() to parent the Qt dialog to XSI's main window.
      SetParent((HWND)wd->qtDialog->winId(), (HWND)Application().GetDesktop().GetApplicationWindowHandle());

      // show/execute Qt dialog.
      wd->qtDialog->exec();
    }
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // done.
  g_canvasIsOpen = false;
}