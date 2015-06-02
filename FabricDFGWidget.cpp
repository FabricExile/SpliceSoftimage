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
  QDialog         *qtDialog;
  FabricDFGWidget *qtDFGWidget;

  FabricCore::Client                             *client;
  FabricServices::ASTWrapper::KLASTManager       *manager;
  FabricServices::DFGWrapper::Host               *host;
  FabricServices::DFGWrapper::Binding            *binding;
  FabricServices::DFGWrapper::GraphExecutablePtr  graph;
  FabricServices::Commands::CommandStack         *stack;

  _windowData(void)
  {
    qtApp         = NULL;
    qtDialog      = NULL;
    qtDFGWidget   = NULL;

    client        = NULL;
    manager       = NULL;
    host          = NULL;
    binding       = NULL;
    stack         = NULL;
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
    winData.client   = baseInterface->getClient();
    winData.manager  = baseInterface->getManager();
    winData.host     = baseInterface->getHost();
    winData.binding  = baseInterface->getBinding();
    winData.graph    = baseInterface->getGraph();
    winData.stack    = baseInterface->getStack();
  }
  catch(FabricCore::Exception e)
  {
    feLogError(e.getDesc_cstr() ? e.getDesc_cstr() : "\"\"");
  }

  // show canvas.
  {
    _windowData *wd = &winData;
    if (wd->qtApp == NULL)
    {
      int argc = 0;
      wd->qtApp         = new QApplication(argc, NULL);
      wd->qtDialog      = new QDialog();
      wd->qtDFGWidget   = new FabricDFGWidget(NULL/*wd->qtDialog*/);

QVBoxLayout *layout = new QVBoxLayout();
wd->qtDialog->setLayout(layout);
layout->addWidget(wd->qtDFGWidget); 


      wd->qtDialog->setWindowTitle(winTitle ? winTitle : "Canvas");

      //wd->qtDialog->setModal(true);

      wd->qtDialog->setWindowModality(Qt::WindowModal);

      wd->qtDialog->setAttribute(Qt::WA_DeleteOnClose, true);


      // init.
      DFG::DFGConfig config;
      config.graphConfig.useOpenGL = false;
      wd->qtDFGWidget->init(wd->client,
                            wd->manager,
                            wd->host,
                           *wd->binding,
                            wd->graph,
                            wd->stack,
                            true,
                            config
                           );

      // use Windows function SetParent() to parent the Qt dialog to XSI's main window.
      SetParent((HWND)wd->qtDialog->winId(), (HWND)Application().GetDesktop().GetApplicationWindowHandle());

      // show/execute Qt dialog.
      wd->qtDialog->exec();
      //if (!wd->qtDialog->isVisible())
      //{
      //  wd->qtDialog->show();
      //  while (wd->qtDialog->isVisible())
      //  {
      //    wd->qtApp->processEvents();
      //  }
      //}
    }


  }

  // done.
  g_canvasIsOpen = false;
}