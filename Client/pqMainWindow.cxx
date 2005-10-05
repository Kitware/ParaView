/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCamera.h"
#include "pqMainWindow.h"
#include "pqParts.h"
#include "pqServer.h"
#include "pqRenderViewProxy.h"
#include "pqServerFileBrowser.h"
#include "pqServerBrowser.h"
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
#include <vtkPVGenericRenderWindowInteractor.h>

#include <QVTKWidget.h>

namespace
{

void pqDumpHierarchy(ostream& Stream, QObject& Object, unsigned long Indent = 0)
{
  Stream << vtkstd::string(Indent, '\t') << Object.name("[unspecified]") << endl;
  
  QList<QObject*> children = Object.findChildren<QObject*>();
  for(QList<QObject*>::iterator child = children.begin(); child != children.end(); ++child)
    pqDumpHierarchy(Stream, **child, Indent+1);
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
  currentServer(0),
  window(0),
  fileOpenAction(0),
  serverConnectAction(0),
  serverDisconnectAction(0)
{
  this->setName("mainWindow");
  this->setWindowTitle("ParaQ Client");

  this->fileOpenAction = new QAction(tr("Open..."), this) << pqSetName("fileOpenAction");
  connect(this->fileOpenAction, SIGNAL(activated()), this, SLOT(onFileOpen()));

  QAction* const fileQuitAction = new QAction(tr("Quit"), this) << pqSetName("fileQuitAction");
  connect(fileQuitAction, SIGNAL(activated()), &Application, SLOT(quit()));

  this->serverConnectAction = new QAction(tr("Connect..."), this) << pqSetName("serverConnectAction");
  connect(this->serverConnectAction, SIGNAL(activated()), this, SLOT(onServerConnect()));

  this->serverDisconnectAction = new QAction(tr("Disconnect"), this) << pqSetName("serverDisconnectAction");
  connect(this->serverDisconnectAction, SIGNAL(activated()), this, SLOT(onServerDisconnect()));

  QAction* const debugHierarchyAction = new QAction(tr("Dump Hierarchy"), this) << pqSetName("debugHierarchyAction");
  connect(debugHierarchyAction, SIGNAL(activated()), this, SLOT(onDebugHierarchy()));

  QAction* const testsRunAction = new QAction(tr("Run"), this) << pqSetName("testsRunAction");
  connect(testsRunAction, SIGNAL(activated()), this, SLOT(onTestsRun()));

  this->menuBar() << pqSetName("menuBar");

  QMenu* const fileMenu = this->menuBar()->addMenu(tr("File")) << pqSetName("fileMenu");
  fileMenu->addAction(fileOpenAction);
  fileMenu->addAction(fileQuitAction);
  
  QMenu* const serverMenu = this->menuBar()->addMenu(tr("Server")) << pqSetName("serverMenu");
  serverMenu->addAction(serverConnectAction);
  serverMenu->addAction(serverDisconnectAction);
  
  QMenu* const debugMenu = this->menuBar()->addMenu(tr("Debug")) << pqSetName("debugMenu");
  debugMenu->addAction(debugHierarchyAction);
  
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
    this->currentServer = Server;
    }
  
  fileOpenAction->setEnabled(this->currentServer);
  serverConnectAction->setEnabled(!this->currentServer);
  serverDisconnectAction->setEnabled(this->currentServer);
}

void pqMainWindow::onServerConnect()
{
  pqServerBrowser server_browser(this, "serverBrowser");
  if(server_browser.exec() != QDialog::Accepted)
    return;
    
  if(server_browser.ui.serverType->currentIndex() == 0)
    {
    pqServer* const server = pqServer::Standalone();
    if(server)
      {
      this->setServer(server);
      }
    else
      {
      QMessageBox::critical(this, tr("Pick Server:"), tr("Error connecting to server"));
      }
    }
  else if(server_browser.ui.serverType->currentIndex() == 1)
    {
    pqServer* const server = pqServer::Connect(server_browser.ui.hostName->text().ascii(), server_browser.ui.portNumber->value());
    if(server)
      {
      this->setServer(server);
      }
    else
      {
      QMessageBox::critical(this, tr("Pick Server:"), tr("Error connecting to server"));
      }
    }
  else
    {
    QMessageBox::critical(this, tr("Pick Server:"), tr("Unknown server type"));
    }
}

void pqMainWindow::onServerDisconnect()
{
  setServer(0);
}

void pqMainWindow::onFileOpen()
{
  if(!this->currentServer)
    {
    return;
    }
  pqServerFileBrowser* const file_browser = new pqServerFileBrowser(*this->currentServer, this, "fileOpenBrowser");
  file_browser->show();
  QObject::connect(file_browser, SIGNAL(fileSelected(const QString&)), this, SLOT(onFileOpen(const QString&)));
}

void pqMainWindow::onFileOpen(const QString& File)
{
  // Create a source ... see ParaView/Servers/ServerManager/Resources/sources.xml
  vtkSMProxy* const source = this->currentServer->GetProxyManager()->NewProxy("sources", "ExodusReader");
  this->currentServer->GetProxyManager()->RegisterProxy("paraq", "source1", source);
  source->Delete();
  vtkSMStringVectorProperty::SafeDownCast(source->GetProperty("FileName"))->SetElement(0, File.ascii());
  vtkSMStringVectorProperty::SafeDownCast(source->GetProperty("FilePrefix"))->SetElement(0, File.ascii());
  vtkSMStringVectorProperty::SafeDownCast(source->GetProperty("FilePattern"))->SetElement(0, "%s");
  source->UpdateVTKObjects();
  
  pqAddPart(currentServer, vtkSMSourceProxy::SafeDownCast(source));

  // Create a render window ...  
  vtkRenderWindow* const render_window = this->currentServer->GetRenderModule()->GetRenderWindow();
  this->window->SetRenderWindow(render_window);
  this->window->setWindowTitle("ParaQ Client");
  this->window->update();

  pqRenderViewProxy* proxy = pqRenderViewProxy::New();
  proxy->SetRenderWindow(this->window);
  vtkPVGenericRenderWindowInteractor* iren = 
       vtkPVGenericRenderWindowInteractor::SafeDownCast(this->currentServer->GetRenderModule()->GetInteractor());
  iren->SetPVRenderView(proxy);
  proxy->Delete();
  iren->Enable();

  //render_window->SetWindowName("ParaQ Client");
  //render_window->SetPosition(500, 500);
  //render_window->SetSize(640, 480);
  //render_window->Render();

  pqResetCamera(currentServer->GetRenderModule());
  pqRedrawCamera(currentServer->GetRenderModule());
}

void pqMainWindow::onDebugHierarchy()
{
  pqDumpHierarchy(cerr, *this);
}

void pqMainWindow::onTestsRun()
{
  pqRunRegressionTests();
  pqRunRegressionTests(*this);
}

