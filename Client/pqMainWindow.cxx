/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCamera.h"
#include "pqFileDialog.h"
#include "pqLocalFileDialogModel.h"
#include "pqMainWindow.h"
#include "pqParts.h"
#include "pqProperties.h"
#include "pqRenderViewProxy.h"
#include "pqServer.h"
#include "pqServerBrowser.h"
#include "pqServerFileDialogModel.h"
#include "pqTesting.h"

#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>

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

void pqDumpQtHierarchy(ostream& Stream, QObject& Object, unsigned long Indent = 0)
{
  Stream << vtkstd::string(Indent, '\t') << Object.name("[unspecified]") << endl;
  
  QList<QObject*> children = Object.findChildren<QObject*>();
  for(QList<QObject*>::iterator child = children.begin(); child != children.end(); ++child)
    pqDumpQtHierarchy(Stream, **child, Indent+1);
}

struct pqSetName
{
  pqSetName(const vtkstd::string& Name) : name(Name) {}
  const vtkstd::string name;
};

template<typename T>
T* operator<<(T* LHS, const pqSetName& RHS)
{
  LHS->setName(RHS.name.c_str());
  return LHS;
}

} // namespace

pqMainWindow::pqMainWindow(QApplication& Application) :
  base(),
  currentServer(0),
  window(0),
  serverDisconnectAction(0)
{
  this->setName("mainWindow");
  this->setWindowTitle("ParaQ Client");

  QAction* const fileNewAction = new QAction(tr("New..."), this) << pqSetName("fileNewAction");
  connect(fileNewAction, SIGNAL(activated()), this, SLOT(onFileNew()));

  QAction* const fileOpenAction = new QAction(tr("Open..."), this) << pqSetName("fileOpenAction");
  connect(fileOpenAction, SIGNAL(activated()), this, SLOT(onFileOpen()));

  QAction* const fileOpenServerStateAction = new QAction(tr("Open Server State"), this) << pqSetName("fileOpenServerStateAction");
  connect(fileOpenServerStateAction, SIGNAL(activated()), this, SLOT(onFileOpenServerState()));

  QAction* const fileSaveServerStateAction = new QAction(tr("Save Server State"), this) << pqSetName("fileSaveServerStateAction");
  connect(fileSaveServerStateAction, SIGNAL(activated()), this, SLOT(onFileSaveServerState()));

  QAction* const fileQuitAction = new QAction(tr("Quit"), this) << pqSetName("fileQuitAction");
  connect(fileQuitAction, SIGNAL(activated()), &Application, SLOT(quit()));

  QAction* const serverConnectAction = new QAction(tr("Connect..."), this) << pqSetName("serverConnectAction");
  connect(serverConnectAction, SIGNAL(activated()), this, SLOT(onServerConnect()));

  this->serverDisconnectAction = new QAction(tr("Disconnect"), this) << pqSetName("serverDisconnectAction");
  connect(this->serverDisconnectAction, SIGNAL(activated()), this, SLOT(onServerDisconnect()));

  QAction* const debugOpenLocalFilesAction = new QAction(tr("Open Local Files"), this) << pqSetName("debugOpenLocalFilesAction");
  connect(debugOpenLocalFilesAction, SIGNAL(activated()), this, SLOT(onDebugOpenLocalFiles()));

  QAction* const debugDumpQtHierarchyAction = new QAction(tr("Dump Qt Hierarchy"), this) << pqSetName("debugDumpQtHierarchyAction");
  connect(debugDumpQtHierarchyAction, SIGNAL(activated()), this, SLOT(onDebugDumpQtHierarchy()));

  QAction* const testsRunAction = new QAction(tr("Run"), this) << pqSetName("testsRunAction");
  connect(testsRunAction, SIGNAL(activated()), this, SLOT(onTestsRun()));

  this->menuBar() << pqSetName("menuBar");

  QMenu* const fileMenu = this->menuBar()->addMenu(tr("File")) << pqSetName("fileMenu");
  fileMenu->addAction(fileNewAction);
  fileMenu->addAction(fileOpenAction);
//  fileMenu->addAction(fileOpenServerStateAction);
  fileMenu->addAction(fileSaveServerStateAction);
  fileMenu->addAction(fileQuitAction);
  
  QMenu* const serverMenu = this->menuBar()->addMenu(tr("Server")) << pqSetName("serverMenu");
  serverMenu->addAction(serverConnectAction);
  serverMenu->addAction(this->serverDisconnectAction);
  
  QMenu* const debugMenu = this->menuBar()->addMenu(tr("Debug")) << pqSetName("debugMenu");
  debugMenu->addAction(debugOpenLocalFilesAction);
  debugMenu->addAction(debugDumpQtHierarchyAction);
  
  QMenu* const testMenu = this->menuBar()->addMenu(tr("Tests")) << pqSetName("testMenu");
  testMenu->addAction(testsRunAction);
  
  this->setServer(0);
}

pqMainWindow::~pqMainWindow()
{
  delete this->window;
  delete this->currentServer;
}

void pqMainWindow::setServer(pqServer* Server)
{
  delete this->window;
  this->window = 0;

  delete this->currentServer;
  this->currentServer = 0;
  
  if(Server)
    {
    this->window = new QVTKWidget(this);
    this->setCentralWidget(this->window);

    vtkRenderWindow* const render_window = Server->GetRenderModule()->GetRenderWindow();
    this->window->SetRenderWindow(render_window);
    this->window->update();

    pqRenderViewProxy* proxy = pqRenderViewProxy::New();
    proxy->SetRenderWindow(this->window);
    vtkPVGenericRenderWindowInteractor* interactor = vtkPVGenericRenderWindowInteractor::SafeDownCast(Server->GetRenderModule()->GetInteractor());
    interactor->SetPVRenderView(proxy);
    proxy->Delete();
    interactor->Enable();
    
    this->currentServer = Server;
    }
  
  serverDisconnectAction->setEnabled(this->currentServer);
}

void pqMainWindow::onFileNew()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this, "serverBrowser");
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onFileNew(pqServer*)));
  server_browser->show();
}

void pqMainWindow::onFileNew(pqServer* Server)
{
  setServer(Server);

  // Create a source ... see ParaView/Servers/ServerManager/Resources/sources.xml
  vtkSMProxy* const source = this->currentServer->GetProxyManager()->NewProxy("sources", "CylinderSource");
  this->currentServer->GetProxyManager()->RegisterProxy("paraq", "source1", source);
  source->Delete();
  pqSetProperty(source, "Resolution", 64);
  pqSetProperty(source, "Radius", 0.1);
  pqSetProperty(source, "foo", "bar");
  pqSetProperty(source, "Radius", "ten");
  source->UpdateVTKObjects();
  
  pqAddPart(currentServer, vtkSMSourceProxy::SafeDownCast(source));

  pqResetCamera(this->currentServer->GetRenderModule());
  pqRedrawCamera(this->currentServer->GetRenderModule());
}

void pqMainWindow::onFileOpen()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this, "serverBrowser");
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onFileOpen(pqServer*)));
  server_browser->show();
}

void pqMainWindow::onFileOpen(pqServer* Server)
{
  setServer(Server);

  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(this->currentServer->GetProcessModule()), tr("Open File:"), this, "fileOpenDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileOpen(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileOpen(const QStringList& Files)
{
  for(int i = 0; i != Files.size(); ++i)
    {
    QString file = Files[i];
    
    vtkSMProxy* const source = this->currentServer->GetProxyManager()->NewProxy("sources", "ExodusReader");
    this->currentServer->GetProxyManager()->RegisterProxy("paraq", "source1", source);
    source->Delete();
    pqSetProperty(source, "FileName", file);
    pqSetProperty(source, "FilePrefix", file);
    pqSetProperty(source, "FilePattern", "%s");
    source->UpdateVTKObjects();
    
    pqAddPart(currentServer, vtkSMSourceProxy::SafeDownCast(source));
    }

  pqResetCamera(this->currentServer->GetRenderModule());
  pqRedrawCamera(this->currentServer->GetRenderModule());
}

void pqMainWindow::onFileOpenServerState()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this, "serverBrowser");
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onFileOpenServerState(pqServer*)));
  server_browser->show();
}

void pqMainWindow::onFileOpenServerState(pqServer* Server)
{
  setServer(Server);

  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(this->currentServer->GetProcessModule()), tr("Open Server State File:"), this, "fileOpenDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileOpenServerState(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileOpenServerState(const QStringList& Files)
{
}

void pqMainWindow::onFileSaveServerState()
{
  if(!this->currentServer)
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
    ofstream file(Files[i]);
    file << "<ServerState>" << "\n";
    this->currentServer->GetProxyManager()->SaveState("test", &file, 0);
    file << "</ServerState>" << "\n";
    }
}

void pqMainWindow::onServerConnect()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this, "serverBrowser");
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
    cerr << "File: " << Files[i].ascii() << endl;
    }
}

void pqMainWindow::onDebugDumpQtHierarchy()
{
  pqDumpQtHierarchy(cerr, *this);
}

void pqMainWindow::onTestsRun()
{
  pqRunRegressionTests();
  pqRunRegressionTests(*this);
}


