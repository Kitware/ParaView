/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCommandDispatcher.h"
#include "pqCommandDispatcherManager.h"
#include "pqFileDialog.h"
#include "pqLocalFileDialogModel.h"
#include "pqMainWindow.h"
#include "pqObjectInspector.h"
#include "pqParts.h"
#include "pqRefreshToolbar.h"
#include "pqRenderViewProxy.h"
#include "pqServer.h"
#include "pqServerBrowser.h"
#include "pqServerFileDialogModel.h"
#include "pqSMAdaptor.h"
#include "ui_pqAbout.h"
#include "pqConfig.h"
#ifdef PARAQ_BUILD_TESTING
#  include "pqTesting.h"
#endif

#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QHeaderView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QTreeView>

#include <vtkRenderWindow.h>
#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMXMLParser.h>
#include <vtkPVGenericRenderWindowInteractor.h>

#include <QVTKWidget.h>

namespace
{

struct pqSetName
{
  pqSetName(const vtkstd::string& Name) : name(Name) {}
  const vtkstd::string name;
};

template<typename T>
T* operator<<(T* LHS, const pqSetName& RHS)
{
  LHS->setObjectName(RHS.name.c_str());
  return LHS;
}

} // namespace

pqMainWindow::pqMainWindow() :
  base(),
  CurrentServer(0),
  RefreshToolbar(0),
  PropertyToolbar(0),
  Window(0),
  ServerDisconnectAction(0),
  Inspector(0),
  InspectorDock(0),
  InspectorView(0)
{
  this->setObjectName("mainWindow");
  this->setWindowTitle(QByteArray("ParaQ Client") + QByteArray(" ") + QByteArray(QT_CLIENT_VERSION));

  QAction* const fileNewAction = new QAction(tr("New..."), this) << pqSetName("fileNewAction");
  QObject::connect(fileNewAction, SIGNAL(triggered()), this, SLOT(onFileNew()));

  QAction* const fileOpenAction = new QAction(tr("Open..."), this) << pqSetName("fileOpenAction");
  QObject::connect(fileOpenAction, SIGNAL(triggered()), this, SLOT(onFileOpen()));

  QAction* const fileOpenServerStateAction = new QAction(tr("Open Server State"), this) << pqSetName("fileOpenServerStateAction");
  QObject::connect(fileOpenServerStateAction, SIGNAL(triggered()), this, SLOT(onFileOpenServerState()));

  QAction* const fileSaveServerStateAction = new QAction(tr("Save Server State"), this) << pqSetName("fileSaveServerStateAction");
  QObject::connect(fileSaveServerStateAction, SIGNAL(triggered()), this, SLOT(onFileSaveServerState()));

  QAction* const fileQuitAction = new QAction(tr("Quit"), this) << pqSetName("fileQuitAction");
  QObject::connect(fileQuitAction, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));

  QAction* const serverConnectAction = new QAction(tr("Connect..."), this) << pqSetName("serverConnectAction");
  QObject::connect(serverConnectAction, SIGNAL(triggered()), this, SLOT(onServerConnect()));

  this->ServerDisconnectAction = new QAction(tr("Disconnect"), this) << pqSetName("serverDisconnectAction");
  QObject::connect(this->ServerDisconnectAction, SIGNAL(triggered()), this, SLOT(onServerDisconnect()));

  QAction* const debugOpenLocalFilesAction = new QAction(tr("Open Local Files"), this) << pqSetName("debugOpenLocalFilesAction");
  QObject::connect(debugOpenLocalFilesAction, SIGNAL(triggered()), this, SLOT(onDebugOpenLocalFiles()));

  QAction* const debugDumpQtHierarchyAction = new QAction(tr("Dump Qt Hierarchy"), this) << pqSetName("debugDumpQtHierarchyAction");
  QObject::connect(debugDumpQtHierarchyAction, SIGNAL(triggered()), this, SLOT(onDebugDumpQtHierarchy()));

  QAction* const testsRunAction = new QAction(tr("Run"), this) << pqSetName("testsRunAction");
  QObject::connect(testsRunAction, SIGNAL(triggered()), this, SLOT(onTestsRun()));

  this->menuBar() << pqSetName("menuBar");

  QMenu* const fileMenu = this->menuBar()->addMenu(tr("File")) << pqSetName("fileMenu");
  fileMenu->addAction(fileNewAction);
  fileMenu->addAction(fileOpenAction);
//  fileMenu->addAction(fileOpenServerStateAction);
  fileMenu->addAction(fileSaveServerStateAction);
  fileMenu->addAction(fileQuitAction);
  
  QMenu* const serverMenu = this->menuBar()->addMenu(tr("Server")) << pqSetName("serverMenu");
  serverMenu->addAction(serverConnectAction);
  serverMenu->addAction(this->ServerDisconnectAction);

  this->SourcesMenu = this->menuBar()->addMenu(tr("Sources")) << pqSetName("sourcesMenu");
  QObject::connect(this, SIGNAL(serverChanged()), SLOT(updateSourcesMenu()));
  
  QMenu* const debugMenu = this->menuBar()->addMenu(tr("Debug")) << pqSetName("debugMenu");
  debugMenu->addAction(debugOpenLocalFilesAction);
  debugMenu->addAction(debugDumpQtHierarchyAction);
  
  QMenu* const testMenu = this->menuBar()->addMenu(tr("Tests")) << pqSetName("testMenu");
  testMenu->addAction(testsRunAction);

  // keep help last
  QMenu* const helpMenu = this->menuBar()->addMenu(tr("Help")) << pqSetName("helpMenu");
  QAction* aboutAction = new QAction(tr("About") + " " + tr("ParaQ") + " " + QT_CLIENT_VERSION, this) << pqSetName("aboutAction");
  QObject::connect(aboutAction, SIGNAL(triggered()), this, SLOT(onAbout()));
  helpMenu->addAction(aboutAction);
  
  QObject::connect(&pqCommandDispatcherManager::instance(), SIGNAL(dispatcherChanged()), this, SLOT(onDispatcherChanged()));
 
  this->RefreshToolbar = new pqRefreshToolbar(this);
  this->addToolBar(this->RefreshToolbar);

  // Create the object inspector model.
  this->Inspector = new pqObjectInspector(this);
  if(this->Inspector)
    this->Inspector->setObjectName("Inspector");

  // Add the object inspector dock window.
  this->InspectorDock = new QDockWidget("Object Inspector", this);
  if(this->InspectorDock)
    {
    this->InspectorDock->setObjectName("InspectorDock");
    this->InspectorDock->setAllowedAreas(
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->InspectorDock->setFeatures(QDockWidget::AllDockWidgetFeatures);
    this->InspectorView = new QTreeView(this->InspectorDock);
    if(this->InspectorView)
      {
      this->InspectorView->setObjectName("InspectorView");
      this->InspectorView->setAlternatingRowColors(true);
      this->InspectorView->header()->hide();
      this->InspectorDock->setWidget(this->InspectorView);
      this->InspectorView->setModel(this->Inspector);
      }

    this->addDockWidget(Qt::LeftDockWidgetArea, this->InspectorDock);
    }

  this->setServer(0);
  this->Adaptor = new pqSMAdaptor;  // should go in pqServer?
}

pqMainWindow::~pqMainWindow()
{
  // Clean up the model before deleting the adaptor.
  if(this->Inspector)
  {
    if(this->InspectorView)
      this->InspectorView->setModel(0);
    delete this->Inspector;
  }

  delete this->Window;
  delete this->PropertyToolbar;
  delete this->RefreshToolbar;
  delete this->CurrentServer;
  delete this->Adaptor;
}

void pqMainWindow::setServer(pqServer* Server)
{
  delete this->Window;
  this->Window = 0;

  delete this->PropertyToolbar;
  this->PropertyToolbar = 0;

  delete this->CurrentServer;
  this->CurrentServer = 0;

  if(Server)
    {
    this->Window = new QVTKWidget(this);
    this->setCentralWidget(this->Window);

    vtkRenderWindow* const rw = Server->GetRenderModule()->GetRenderWindow();
    this->Window->SetRenderWindow(rw);
    this->Window->update();

    pqRenderViewProxy* proxy = pqRenderViewProxy::New();
    proxy->SetRenderModule(Server->GetRenderModule());
    vtkPVGenericRenderWindowInteractor* interactor = vtkPVGenericRenderWindowInteractor::SafeDownCast(Server->GetRenderModule()->GetInteractor());
    interactor->SetPVRenderView(proxy);
    proxy->Delete();
    interactor->Enable();
    
    this->CurrentServer = Server;
    }
  
  this->ServerDisconnectAction->setEnabled(this->CurrentServer);
  emit serverChanged();

}

void pqMainWindow::onFileNew()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onFileNew(pqServer*)));
  server_browser->show();
}

void pqMainWindow::onFileNew(pqServer* Server)
{
  setServer(Server);
}

void pqMainWindow::onFileOpen()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onFileOpen(pqServer*)));
  server_browser->show();
}

void pqMainWindow::onFileOpen(pqServer* Server)
{
  setServer(Server);

  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(this->CurrentServer->GetProcessModule()), tr("Open File:"), this, "fileOpenDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileOpen(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileOpen(const QStringList& Files)
{
  for(int i = 0; i != Files.size(); ++i)
    {
    QString file = Files[i];
    
    vtkSMProxy* const source = this->CurrentServer->GetProxyManager()->NewProxy("sources", "ExodusReader");
    this->CurrentServer->GetProxyManager()->RegisterProxy("paraq", "source1", source);
    source->Delete();
    Adaptor->setProperty(source->GetProperty("FileName"), file);
    Adaptor->setProperty(source->GetProperty("FilePrefix"), file);
    Adaptor->setProperty(source->GetProperty("FilePattern"), "%s");
    source->UpdateVTKObjects();
    
    pqAddPart(this->CurrentServer, vtkSMSourceProxy::SafeDownCast(source));
    }

  this->CurrentServer->GetRenderModule()->ResetCamera();
  this->Window->update();
}

void pqMainWindow::onFileOpenServerState()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onFileOpenServerState(pqServer*)));
  server_browser->show();
}

void pqMainWindow::onFileOpenServerState(pqServer* Server)
{
  setServer(Server);

  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(this->CurrentServer->GetProcessModule()), tr("Open Server State File:"), this, "fileOpenDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileOpenServerState(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileOpenServerState(const QStringList& Files)
{
}

void pqMainWindow::onFileSaveServerState()
{
  if(!this->CurrentServer)
    {
    QMessageBox::critical(this, tr("Dump Server State:"), tr("No server connections to serialize"));
    return;
    }

  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Save Server State:"), this, "fileSaveDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileSaveServerState(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileSaveServerState(const QStringList& Files)
{
  for(int i = 0; i != Files.size(); ++i)
    {
    ofstream file(Files[i].toAscii().data());
    file << "<ServerState>" << "\n";
    this->CurrentServer->GetProxyManager()->SaveState("test", &file, 0);
    file << "</ServerState>" << "\n";
    }
}

void pqMainWindow::onServerConnect()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onServerConnect(pqServer*)));
  server_browser->show();
}

void pqMainWindow::onServerConnect(pqServer* Server)
{
  setServer(Server);
}

void pqMainWindow::onServerDisconnect()
{
  setServer(0);
}

void pqMainWindow::onDebugOpenLocalFiles()
{
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Open File:"), this, "fileOpenDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onDebugOpenLocalFiles(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onDebugOpenLocalFiles(const QStringList& Files)
{
  for(int i = 0; i != Files.size(); ++i)
    {
    cerr << "File: " << Files[i].toAscii().data() << endl;
    }
}

void pqMainWindow::onDebugDumpQtHierarchy()
{
  dumpObjectTree();
}

void pqMainWindow::onTestsRun()
{
#ifdef PARAQ_BUILD_TESTING
  pqRunRegressionTests(this);
#endif
}

void pqMainWindow::onDispatcherChanged()
{
  QObject::connect(&pqCommandDispatcherManager::instance().getDispatcher(), SIGNAL(updateWindow()), this, SLOT(onRedrawWindows()));
}

void pqMainWindow::onRedrawWindows()
{
  if(this->CurrentServer)
    this->CurrentServer->GetRenderModule()->StillRender();
}

void pqMainWindow::updateSourcesMenu()
{
  this->SourcesMenu->clear();

  if(this->CurrentServer)
    {
    vtkSMProxyManager* manager = this->CurrentServer->GetProxyManager();
    int numSources = manager->GetNumberOfProxies("sources");
    for(int i=0; i<numSources; i++)
      {
      this->SourcesMenu->addAction(tr(manager->GetProxyName("sources", i)));
      }
    }
}

void pqMainWindow::onAbout()
{
  QDialog about(this);
  Ui::pqAboutDialog ui;
  ui.setupUi(&about);
  about.exec();
}

