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
  QObject::connect(fileNewAction, SIGNAL(triggered()), this, SLOT(OnFileNew()));

  QAction* const fileOpenAction = new QAction(tr("Open..."), this) << pqSetName("fileOpenAction");
  QObject::connect(fileOpenAction, SIGNAL(triggered()), this, SLOT(OnFileOpen()));

  QAction* const fileOpenServerStateAction = new QAction(tr("Open Server State"), this) << pqSetName("fileOpenServerStateAction");
  QObject::connect(fileOpenServerStateAction, SIGNAL(triggered()), this, SLOT(OnFileOpenServerState()));

  QAction* const fileSaveServerStateAction = new QAction(tr("Save Server State"), this) << pqSetName("fileSaveServerStateAction");
  QObject::connect(fileSaveServerStateAction, SIGNAL(triggered()), this, SLOT(OnFileSaveServerState()));

  QAction* const fileQuitAction = new QAction(tr("Quit"), this) << pqSetName("fileQuitAction");
  QObject::connect(fileQuitAction, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));

  QAction* const serverConnectAction = new QAction(tr("Connect..."), this) << pqSetName("serverConnectAction");
  QObject::connect(serverConnectAction, SIGNAL(triggered()), this, SLOT(OnServerConnect()));

  this->ServerDisconnectAction = new QAction(tr("Disconnect"), this) << pqSetName("serverDisconnectAction");
  QObject::connect(this->ServerDisconnectAction, SIGNAL(triggered()), this, SLOT(OnServerDisconnect()));

  this->menuBar() << pqSetName("menuBar");

  QMenu* const fileMenu = this->menuBar()->addMenu(tr("File")) << pqSetName("fileMenu");
  fileMenu->addAction(fileNewAction);
  fileMenu->addAction(fileOpenAction);
  fileMenu->addAction(fileSaveServerStateAction);
  fileMenu->addAction(fileQuitAction);
  
  QMenu* const serverMenu = this->menuBar()->addMenu(tr("Server")) << pqSetName("serverMenu");
  serverMenu->addAction(serverConnectAction);
  serverMenu->addAction(this->ServerDisconnectAction);

  this->SourcesMenu = this->menuBar()->addMenu(tr("Sources")) << pqSetName("sourcesMenu");
  QObject::connect(this, SIGNAL(ServerChanged()), SLOT(OnUpdateSourcesMenu()));
  
  // keep help last
  QMenu* const helpMenu = this->menuBar()->addMenu(tr("Help")) << pqSetName("helpMenu");
  QAction* aboutAction = new QAction(tr("About") + " " + tr("ParaQ") + " " + QT_CLIENT_VERSION, this) << pqSetName("aboutAction");
  QObject::connect(aboutAction, SIGNAL(triggered()), this, SLOT(OnHelpAbout()));
  helpMenu->addAction(aboutAction);
  
  QObject::connect(&pqCommandDispatcherManager::Instance(), SIGNAL(UpdateWindows()), this, SLOT(OnUpdateWindows()));
 
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

  this->SetServer(0);
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

void pqMainWindow::SetServer(pqServer* Server)
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
  emit ServerChanged();

}

void pqMainWindow::OnFileNew()
{
  SetServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  QObject::connect(server_browser, SIGNAL(ServerConnected(pqServer*)), this, SLOT(OnFileNew(pqServer*)));
  server_browser->show();
}

void pqMainWindow::OnFileNew(pqServer* Server)
{
  SetServer(Server);
}

void pqMainWindow::OnFileOpen()
{
  SetServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  QObject::connect(server_browser, SIGNAL(ServerConnected(pqServer*)), this, SLOT(OnFileOpen(pqServer*)));
  server_browser->show();
}

void pqMainWindow::OnFileOpen(pqServer* Server)
{
  SetServer(Server);

  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(this->CurrentServer->GetProcessModule()), tr("Open File:"), this, "fileOpenDialog");
  QObject::connect(file_dialog, SIGNAL(FilesSelected(const QStringList&)), this, SLOT(OnFileOpen(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::OnFileOpen(const QStringList& Files)
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

void pqMainWindow::OnFileOpenServerState()
{
  SetServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  QObject::connect(server_browser, SIGNAL(ServerConnected(pqServer*)), this, SLOT(OnFileOpenServerState(pqServer*)));
  server_browser->show();
}

void pqMainWindow::OnFileOpenServerState(pqServer* Server)
{
  SetServer(Server);

  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(this->CurrentServer->GetProcessModule()), tr("Open Server State File:"), this, "fileOpenDialog");
  QObject::connect(file_dialog, SIGNAL(FilesSelected(const QStringList&)), this, SLOT(OnFileOpenServerState(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::OnFileOpenServerState(const QStringList& Files)
{
}

void pqMainWindow::OnFileSaveServerState()
{
  if(!this->CurrentServer)
    {
    QMessageBox::critical(this, tr("Dump Server State:"), tr("No server connections to serialize"));
    return;
    }

  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Save Server State:"), this, "fileSaveDialog");
  QObject::connect(file_dialog, SIGNAL(FilesSelected(const QStringList&)), this, SLOT(OnFileSaveServerState(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::OnFileSaveServerState(const QStringList& Files)
{
  for(int i = 0; i != Files.size(); ++i)
    {
    ofstream file(Files[i].toAscii().data());
    file << "<ServerState>" << "\n";
    this->CurrentServer->GetProxyManager()->SaveState("test", &file, 0);
    file << "</ServerState>" << "\n";
    }
}

void pqMainWindow::OnServerConnect()
{
  SetServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  QObject::connect(server_browser, SIGNAL(ServerConnected(pqServer*)), this, SLOT(OnServerConnect(pqServer*)));
  server_browser->show();
}

void pqMainWindow::OnServerConnect(pqServer* Server)
{
  SetServer(Server);
}

void pqMainWindow::OnServerDisconnect()
{
  SetServer(0);
}

void pqMainWindow::OnUpdateWindows()
{
  if(this->CurrentServer)
    this->CurrentServer->GetRenderModule()->StillRender();
}

void pqMainWindow::OnUpdateSourcesMenu()
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

void pqMainWindow::OnHelpAbout()
{
  QDialog about(this);
  Ui::pqAboutDialog ui;
  ui.setupUi(&about);
  about.exec();
}

