/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqAboutDialog.h"
#include "pqCommandDispatcher.h"
#include "pqCommandDispatcherManager.h"
#include "pqConfig.h"
#include "pqConnect.h"
#include "pqFileDialog.h"
#include "pqLocalFileDialogModel.h"
#include "pqMainWindow.h"
#include "pqObjectInspector.h"
#include "pqObjectInspectorDelegate.h"
#include "pqParts.h"
#include "pqRefreshToolbar.h"
#include "pqRenderViewProxy.h"
#include "pqServer.h"
#include "pqServerBrowser.h"
#include "pqServerFileDialogModel.h"
#include "pqSetData.h"
#include "pqSetName.h"
#include "pqSMAdaptor.h"
#include "pqPipelineData.h"

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

#ifdef PARAQ_BUILD_TESTING
#include <pqEventPlayer.h>
#include <pqEventPlayerXML.h>
#include <pqRecordEventsDialog.h>
#endif // PARAQ_BUILD_TESTING

pqMainWindow::pqMainWindow() :
  CurrentServer(0),
  RefreshToolbar(0),
  PropertyToolbar(0),
  Window(0),
  ServerDisconnectAction(0),
  Inspector(0),
  InspectorDelegate(0),
  InspectorDock(0),
  InspectorView(0)
{
  this->setObjectName("mainWindow");
  this->setWindowTitle(QByteArray("ParaQ Client") + QByteArray(" ") + QByteArray(QT_CLIENT_VERSION));

  this->menuBar() << pqSetName("menuBar");

  // File menu.
  QMenu* const fileMenu = this->menuBar()->addMenu(tr("File"))
    << pqSetName("fileMenu");
  
  fileMenu->addAction(tr("New..."))
    << pqSetName("New")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileNew()));
    
  fileMenu->addAction(tr("Open..."))
    << pqSetName("Open")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileOpen()));
    
  fileMenu->addAction(tr("Save Server State..."))
    << pqSetName("SaveServerState")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileSaveServerState()));
    
  fileMenu->addAction(tr("Quit"))
    << pqSetName("Quit")
    << pqConnect(SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));

  // Server menu.
  QMenu* const serverMenu = this->menuBar()->addMenu(tr("Server"))
    << pqSetName("serverMenu");
  
  serverMenu->addAction(tr("Connect..."))
    << pqSetName("Connect")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onServerConnect()));

  this->ServerDisconnectAction = serverMenu->addAction(tr("Disconnect"))
    << pqSetName("Disconnect")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onServerDisconnect()));    

  // Sources & Filters menus.
  this->SourcesMenu = this->menuBar()->addMenu(tr("Sources"))
    << pqSetName("sourcesMenu")
    << pqConnect(SIGNAL(triggered(QAction*)), this, SLOT(onCreateSource(QAction*)));
    
  this->FiltersMenu = this->menuBar()->addMenu(tr("Filters"))
    << pqSetName("filtersMenu")
    << pqConnect(SIGNAL(triggered(QAction*)), this, SLOT(onCreateFilter(QAction*)));
  
  QObject::connect(this, SIGNAL(serverChanged()), SLOT(onUpdateSourcesFiltersMenu()));

#ifdef PARAQ_BUILD_TESTING
  // Test menu.
  QMenu* const testsMenu = this->menuBar()->addMenu(tr("Tests"))
    << pqSetName("testsMenu");
  
  testsMenu->addAction(tr("Record"))
    << pqSetName("Record")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onRecordTest()));
    
  testsMenu->addAction(tr("Play"))
    << pqSetName("Play")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onPlayTest()));
    
#endif // PARAQ_BUILD_TESTING
  
  // Help menu.
  QMenu* const helpMenu = this->menuBar()->addMenu(tr("Help"))
    << pqSetName("helpMenu");
  
  helpMenu->addAction(QString(tr("About %1 %2")).arg("ParaQ").arg(QT_CLIENT_VERSION))
    << pqSetName("About")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onHelpAbout()));
 
  // Setup the refresh toolbar.
  QObject::connect(&pqCommandDispatcherManager::Instance(), SIGNAL(updateWindows()), this, SLOT(onUpdateWindows()));
  this->RefreshToolbar = new pqRefreshToolbar(this);
  this->addToolBar(this->RefreshToolbar);

  // Create the object inspector model.
  this->Inspector = new pqObjectInspector(this);
  if(this->Inspector)
    this->Inspector->setObjectName("Inspector");

  this->InspectorDelegate = new pqObjectInspectorDelegate(this);

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
      if(this->InspectorDelegate)
        this->InspectorView->setItemDelegate(this->InspectorDelegate);
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

    vtkRenderWindow* const rw = Server->renderModule()->GetRenderWindow();
    this->Window->SetRenderWindow(rw);
    this->Window->update();

    pqRenderViewProxy* proxy = pqRenderViewProxy::New();
    proxy->SetRenderModule(Server->renderModule());
    vtkPVGenericRenderWindowInteractor* interactor = vtkPVGenericRenderWindowInteractor::SafeDownCast(Server->renderModule()->GetInteractor());
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

  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(this->CurrentServer->processModule()), tr("Open File:"), this, "fileOpenDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileOpen(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileOpen(const QStringList& Files)
{
  for(int i = 0; i != Files.size(); ++i)
    {
    QString file = Files[i];
    
    vtkSMProxy* source = this->CurrentServer->pipelineData()->newSMProxy("sources", "ExodusReader");
    this->CurrentServer->proxyManager()->RegisterProxy("paraq", "source1", source);
    source->Delete();
    Adaptor->setProperty(source->GetProperty("FileName"), file);
    Adaptor->setProperty(source->GetProperty("FilePrefix"), file);
    Adaptor->setProperty(source->GetProperty("FilePattern"), "%s");
    source->UpdateVTKObjects();
    
    pqAddPart(this->CurrentServer, vtkSMSourceProxy::SafeDownCast(source));
    if(this->Inspector)
      this->Inspector->setProxy(this->Adaptor, source);
    }

  this->CurrentServer->renderModule()->ResetCamera();
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

  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(this->CurrentServer->processModule()), tr("Open Server State File:"), this, "fileOpenDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(pnFileOpenServerState(const QStringList&)));
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
    this->CurrentServer->proxyManager()->SaveState("test", &file, 0);
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

void pqMainWindow::onUpdateWindows()
{
  if(this->CurrentServer)
    this->CurrentServer->renderModule()->StillRender();
}

void pqMainWindow::onUpdateSourcesFiltersMenu()
{
  this->FiltersMenu->clear();
  this->SourcesMenu->clear();

  if(this->CurrentServer)
    {
    vtkSMProxyManager* manager = this->CurrentServer->proxyManager();
    manager->InstantiateGroupPrototypes("filters");
    int numFilters = manager->GetNumberOfProxies("filters_prototypes");
    for(int i=0; i<numFilters; i++)
      {
      const char* proxyName = manager->GetProxyName("filters_prototypes",i);
      this->FiltersMenu->addAction(proxyName) << pqSetName(proxyName) << pqSetData(proxyName);
      }

#if 1
    // hard code sources
    this->SourcesMenu->addAction("2D Glyph") << pqSetName("2D Glyph") << pqSetData("GlyphSource2D");
    this->SourcesMenu->addAction("3D Text") << pqSetName("3D Text") << pqSetData("VectorText");
    this->SourcesMenu->addAction("Arrow") << pqSetName("Arrow") << pqSetData("ArrowSource");
    this->SourcesMenu->addAction("Axes") << pqSetName("Axes") << pqSetData("Axes");
    this->SourcesMenu->addAction("Box") << pqSetName("Box") << pqSetData("CubeSource");
    this->SourcesMenu->addAction("Cone") << pqSetName("Cone") << pqSetData("ConeSource");
    this->SourcesMenu->addAction("Cylinder") << pqSetName("Cylinder") << pqSetData("CylinderSource");
    this->SourcesMenu->addAction("Hierarchical Fractal") << pqSetName("Hierarchical Fractal") << pqSetData("HierarchicalFractal");
    this->SourcesMenu->addAction("Line") << pqSetName("Line") << pqSetData("LineSource");
    this->SourcesMenu->addAction("Mandelbrot") << pqSetName("Mandelbrot") << pqSetData("ImageMandelbrotSource");
    this->SourcesMenu->addAction("Plane") << pqSetName("Plane") << pqSetData("PlaneSource");
    this->SourcesMenu->addAction("Sphere") << pqSetName("Sphere") << pqSetData("SphereSource");
    this->SourcesMenu->addAction("Superquadric") << pqSetName("Superquadric") << pqSetData("SuperquadricSource");
    this->SourcesMenu->addAction("Wavelet") << pqSetName("Wavelet") << pqSetData("RTAnalyticSource");
#else
    manager->InstantiateGroupPrototypes("sources");
    int numSources = manager->GetNumberOfProxies("sources_prototypes");
    for(int i=0; i<numSources; i++)
      {
      const char* proxyName = manager->GetProxyName("sources_prototypes",i);
      this->SourcesMenu->addAction(proxyName) << pqSetName(proxyName) << pqSetData(proxyName);
      }
#endif
    }
}

void pqMainWindow::onCreateSource(QAction* action)
{
  if(!action)
    return;

  QByteArray sourceName = action->data().toString().toAscii();

  vtkSMProxy* source = this->CurrentServer->pipelineData()->newSMProxy("sources", sourceName);
  
  //TEMP
  if(this->Inspector)
    this->Inspector->setProxy(this->Adaptor, source);
  pqAddPart(this->CurrentServer, vtkSMSourceProxy::SafeDownCast(source));
  this->CurrentServer->renderModule()->ResetCamera();
  this->Window->update();
}

void pqMainWindow::onCreateFilter(QAction* action)
{
  if(!action)
    return;
  
  QByteArray filterName = action->data().toString().toAscii();

  vtkSMSourceProxy* cp = this->CurrentServer->pipelineData()->currentProxy();
  vtkSMProxy* source = this->CurrentServer->pipelineData()->newSMProxy("filters", filterName);
  vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(source);
  this->CurrentServer->pipelineData()->addInput(sp, cp);
  //TEMP
  if(this->Inspector)
    this->Inspector->setProxy(this->Adaptor, sp);
  pqAddPart(this->CurrentServer, sp);
  this->CurrentServer->renderModule()->ResetCamera();
  this->Window->update();
}

void pqMainWindow::onHelpAbout()
{
  pqAboutDialog* const dialog = new pqAboutDialog(this);
  dialog->show();
}

void pqMainWindow::onRecordTest()
{
#ifdef PARAQ_BUILD_TESTING

  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Record Test:"), this, "fileSaveDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onRecordTest(const QStringList&)));
  file_dialog->show();
  
#endif // PARAQ_BUILD_TESTING
}

void pqMainWindow::onRecordTest(const QStringList& Files)
{
#ifdef PARAQ_BUILD_TESTING

  for(int i = 0; i != Files.size(); ++i)
    {
    pqRecordEventsDialog* const dialog = new pqRecordEventsDialog(Files[i], this);
    dialog->show();
    }
  
#endif // PARAQ_BUILD_TESTING
}

void pqMainWindow::onPlayTest()
{
#ifdef PARAQ_BUILD_TESTING

  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Record Test:"), this, "fileSaveDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onPlayTest(const QStringList&)));
  file_dialog->show();
  
#endif // PARAQ_BUILD_TESTING
}

void pqMainWindow::onPlayTest(const QStringList& Files)
{
#ifdef PARAQ_BUILD_TESTING

  pqEventPlayer player(*this);
  player.addDefaultWidgetEventPlayers();

  for(int i = 0; i != Files.size(); ++i)
    {
      pqEventPlayerXML xml_player;
      xml_player.playXML(player, Files[i]);
    }

#endif // PARAQ_BUILD_TESTING
}

